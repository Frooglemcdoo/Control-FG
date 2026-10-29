#pragma once
// Execute the shipped bytecode over synthetic textures, not a substitute shader.
#include <array>
struct HistoryTexture {ComPtr<ID3D12Resource> resource;D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};};
static float PatternValue(float value,UINT pattern,int x,int y) {
 x=std::max(0,std::min(15,x));y=std::max(0,std::min(7,y));
 if(!pattern)return value;
 const bool bright=pattern==1?(x==8&&y==4):pattern==2?((x==8||x==9)&&y==4):pattern==3?x==8:x>=8;
 return bright?value:.2f;
}
static HistoryTexture HistoryInput(Device& gpu,float base,float filtered,UINT index,UINT mips=1,UINT pattern=0) {
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=16;desc.Height=8;
 desc.DepthOrArraySize=1;desc.MipLevels=static_cast<UINT16>(mips);desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;
 HistoryTexture t;Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&t.resource)));
 std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(mips);UINT64 bytes=0;
 gpu.device->GetCopyableFootprints(&desc,0,mips,0,layouts.data(),nullptr,nullptr,&bytes);
 std::vector<unsigned char> data(static_cast<size_t>(bytes));
 for(UINT mip=0;mip<mips;++mip){auto& f=layouts[mip];
  for(UINT y=0;y<f.Footprint.Height;++y)for(UINT x=0;x<f.Footprint.Width*4;++x){
   float v=mip?filtered:PatternValue(base,pattern,x/4,y);std::memcpy(data.data()+f.Offset+y*f.Footprint.RowPitch+x*sizeof(float),&v,sizeof(v));
  }
 }
 auto upload=gpu.Upload(data);
 for(UINT mip=0;mip<mips;++mip){D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=t.resource.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=mip;
  D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=layouts[mip];gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
 gpu.Transition(t.resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=desc.Format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
 view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=mips;
 auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=static_cast<SIZE_T>(index)*gpu.descriptorSize;
 gpu.device->CreateShaderResourceView(t.resource.Get(),&view,cpu);gpu.retained.push_back(t.resource);return t;
}
static void RunHistoryPixels(Device& gpu,ID3D12RootSignature* root,ID3D12PipelineState* adaptive,ID3D12PipelineState* native,ID3D12PipelineState* noHistory,ID3D12PipelineState* outlier) {
 struct Case {const char* name;float current,filtered,history,depth;bool cut,native;float expected;bool noHistory=false;bool outlier=false;UINT pattern=0;};
 const Case cases[]={
  {"stable history",.2f,.2f,.22f,.5f,false,false,.214f},
  {"bright trail",.2f,.2f,2.f,.5f,false,false,.2357f},
  {"dark trail",.8f,.8f,.02f,.5f,false,false,.6593f},
  {"current firefly",10.f,.2f,.2f,.5f,false,false,.2f},
  {"depth rejection",.2f,.2f,2.f,.7f,false,false,.2f},
  {"camera cut",.2f,.2f,2.f,.5f,true,false,.2f},
  {"native bright trail control",.2f,.2f,2.f,.5f,false,true,1.46f},
  {"dark stable noise",.01f,.01f,.012f,.5f,false,false,.0114f},
  {"zero history bright trail",.2f,.2f,2.f,.5f,false,false,.2f,true},
  {"zero history dark trail",.8f,.8f,.02f,.5f,false,false,.8f,true},
  {"isolated bright sample",10.f,.5f,100.f,.5f,false,false,0,true,true,1},
  {"adjacent bright pair",10.f,.5f,100.f,.5f,false,false,0,true,true,2},
  {"thin bright line",10.f,.5f,100.f,.5f,false,false,0,true,true,3},
  {"bright region edge",10.f,.5f,100.f,.5f,false,false,0,true,true,4},
  {"uniform bright radiance",10.f,10.f,100.f,.5f,false,false,0,true,true,0},
  {"black radiance",0.f,0.f,100.f,.5f,false,false,0,true,true,0},
 };
 D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};heapDesc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
 heapDesc.NumDescriptors=128;heapDesc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 auto previousViews=gpu.views; // Keep previously recorded heap state alive through Submit.
 gpu.views.Reset();Check(gpu.device->CreateDescriptorHeap(&heapDesc,IID_PPV_ARGS(&gpu.views)));
 ID3D12DescriptorHeap* heaps[]={gpu.views.Get()};gpu.list->SetDescriptorHeaps(1,heaps);
 std::vector<ComPtr<ID3D12Resource>> outputs;std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
 for(UINT i=0;i<sizeof(cases)/sizeof(cases[0]);++i){auto c=cases[i];UINT first=6*i;
  HistoryInput(gpu,.5f,.5f,first);HistoryInput(gpu,c.depth,c.depth,first+1);HistoryInput(gpu,0,0,first+2);
  HistoryInput(gpu,c.current,c.filtered,first+3,4,c.pattern);HistoryInput(gpu,c.history,c.history,first+4);
  std::array<float,192> sys{};sys[0]=16;sys[1]=8;sys[2]=1.f/16;sys[3]=1.f/8;
  sys[4]=16;sys[5]=8;sys[6]=1.f/16;sys[7]=1.f/8;
  auto cb0=gpu.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
  std::array<UINT,64> filter{};filter[6]=c.cut?1:0;
  auto cb1=gpu.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=16;desc.Height=8;
  desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  ComPtr<ID3D12Resource> output;Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&output)));gpu.retained.push_back(output);
  D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=desc.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=static_cast<SIZE_T>(first+5)*gpu.descriptorSize;gpu.device->CreateUnorderedAccessView(output.Get(),nullptr,&uav,cpu);
  auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=static_cast<UINT64>(first)*gpu.descriptorSize;auto target=srv;target.ptr+=5*gpu.descriptorSize;
  gpu.list->SetComputeRootSignature(root);gpu.list->SetComputeRootConstantBufferView(0,cb0->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(1,cb1->GetGPUVirtualAddress());
  gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,target);
  gpu.list->SetPipelineState(c.outlier?outlier:c.noHistory?noHistory:c.native?native:adaptive);gpu.list->Dispatch(2,1,1);
  gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;gpu.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
  auto readback=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;
  D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=output.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  outputs.push_back(readback);layouts.push_back(footprint);
 }
 gpu.Submit();
 for(size_t i=0;i<outputs.size();++i){void* mapped=nullptr;Check(outputs[i]->Map(0,nullptr,&mapped));auto f=layouts[i];
  for(UINT y=0;y<8;++y)for(UINT x=0;x<16;++x){auto* pixel=reinterpret_cast<const float*>(static_cast<const unsigned char*>(mapped)+f.Offset+y*f.Footprint.RowPitch)+4*x;
   float expected=cases[i].expected;
   if(cases[i].outlier){auto c=cases[i];auto value=[&](int sx,int sy){return PatternValue(c.current,c.pattern,sx,sy);};
    float neighbors[]={value(int(x)-1,y),value(int(x)+1,y),value(x,int(y)-1),value(x,int(y)+1)};
    std::sort(neighbors,neighbors+4);
    expected=std::min(std::min(value(x,y),c.filtered),neighbors[2]*1.5f+.002f);
   }
   for(UINT ch=0;ch<3;++ch)Require(std::isfinite(pixel[ch])&&std::abs(pixel[ch]-expected)<.0001f,cases[i].name);
  }
  outputs[i]->Unmap(0,nullptr);std::cout<<"PASS R214 GPU pixels: "<<cases[i].name<<'\n';
 }
}
