#pragma once
#include <cstring>
#include <stdexcept>
#include <string>
#include <cfloat>
static void Require(HRESULT hr){if(FAILED(hr))throw std::runtime_error("GPU fixture HRESULT "+std::to_string(unsigned(hr)));}
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 bytes,D3D12_HEAP_TYPE type){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ComPtr<ID3D12Resource> r;Require(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,type==D3D12_HEAP_TYPE_UPLOAD?D3D12_RESOURCE_STATE_GENERIC_READ:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r)));return r;
}
static ComPtr<ID3D12Resource> Texture(ID3D12Device* d,UINT width,UINT height,UINT layers,DXGI_FORMAT format,bool output){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=width;rd.Height=height;rd.DepthOrArraySize=UINT16(layers);rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Format=format;rd.Flags=output?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;
 ComPtr<ID3D12Resource> r;Require(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r)));return r;
}
static void Transition(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};l->ResourceBarrier(1,&b);}
static std::vector<unsigned> ExecuteFixture(ID3D12Device* device,ID3D12RootSignature* root,ID3D12PipelineState* pso,ID3D12ShaderReflection* reflection,bool parallel,UINT width,UINT height,UINT samples,float clamp){
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};Require(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Require(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Require(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso,IID_PPV_ARGS(&list)));
 auto material=Texture(device,width,height,16,DXGI_FORMAT_R32G32B32A32_UINT,false);
 auto output=Texture(device,width,height,1,DXGI_FORMAT_R32G32B32A32_FLOAT,true);
 auto rd=material->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[16]{};UINT64 total=0;device->GetCopyableFootprints(&rd,0,16,0,fp,nullptr,nullptr,&total);
 auto upload=Buffer(device,total,D3D12_HEAP_TYPE_UPLOAD);void* mapped=nullptr;Require(upload->Map(0,nullptr,&mapped));std::memset(mapped,0,size_t(total));
 for(UINT s=0;s<16;++s){
  for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){
   auto* pixel=reinterpret_cast<unsigned*>(static_cast<char*>(mapped)+fp[s].Offset+y*fp[s].Footprint.RowPitch+x*16);
   // Invalid marker may occur at every position; later valid entries must be ignored.
   pixel[0]=s==(x+3*y)%18?0xfffe:0xffff;
  }
  D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp[s];
  D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=material.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=s;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 }
 upload->Unmap(0,nullptr);Transition(list.Get(),material.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=256;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 ComPtr<ID3D12DescriptorHeap> heap,samplers;Require(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;hd.NumDescriptors=32;Require(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&samplers)));
 ID3D12DescriptorHeap* heaps[]={heap.Get(),samplers.Get()};list->SetDescriptorHeaps(2,heaps);list->SetComputeRootSignature(root);
 UINT resourceOffset=0,samplerOffset=0;std::vector<ComPtr<ID3D12Resource>> constants;
 D3D12_SHADER_DESC shader{};reflection->GetDesc(&shader);
 for(UINT i=0;i<shader.BoundResources;++i){
  D3D12_SHADER_INPUT_BIND_DESC b{};reflection->GetResourceBindingDesc(i,&b);bool sampler=b.Type==D3D_SIT_SAMPLER;auto* h=sampler?samplers.Get():heap.Get();auto inc=device->GetDescriptorHandleIncrementSize(sampler?D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  UINT& offset=sampler?samplerOffset:resourceOffset;auto cpu=h->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(offset)*inc;auto gpu=h->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(offset)*inc;list->SetComputeRootDescriptorTable(i,gpu);
  UINT count=b.BindCount && b.BindCount!=UINT_MAX?b.BindCount:1;
  if(offset+count>(sampler?32u:256u))throw std::runtime_error("fixture descriptor capacity");
  for(UINT j=0;j<count;++j){
   if(sampler){D3D12_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=FLT_MAX;s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;device->CreateSampler(&s,cpu);}
   else if(b.Type==D3D_SIT_CBUFFER){
    auto cb=Buffer(device,65536,D3D12_HEAP_TYPE_UPLOAD);float* f=nullptr;Require(cb->Map(0,nullptr,reinterpret_cast<void**>(&f)));std::memset(f,0,65536);
    if(b.BindPoint==0){f[0]=float(width);f[1]=float(height);}
    if(b.BindPoint==1){f[68]=2;f[69]=1;f[70]=1;f[71]=1;}
    if(b.BindPoint==6){f[0]=clamp;std::memcpy(f+2,&samples,4);}
    cb->Unmap(0,nullptr);D3D12_CONSTANT_BUFFER_VIEW_DESC v{cb->GetGPUVirtualAddress(),65536};device->CreateConstantBufferView(&v,cpu);constants.push_back(cb);
   }else if(b.Type==D3D_SIT_UAV_RWTYPED){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(output.Get(),nullptr,&v,cpu);}
   else{
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;
    if(b.Type==D3D_SIT_STRUCTURED){v.Format=DXGI_FORMAT_UNKNOWN;v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Buffer.NumElements=1;v.Buffer.StructureByteStride=4;}
    else if(b.Dimension==D3D_SRV_DIMENSION_TEXTURE2DARRAY){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=16;}
    else if(b.Dimension==D3D_SRV_DIMENSION_TEXTURE3D){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;v.Texture3D.MipLevels=1;}
    else if(b.Dimension==D3D_SRV_DIMENSION_TEXTURECUBE){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURECUBE;v.TextureCube.MipLevels=1;}
    if(b.ReturnType==D3D_RETURN_TYPE_UINT)v.Format=DXGI_FORMAT_R32G32B32A32_UINT;
    device->CreateShaderResourceView(b.Space==0 && b.BindPoint==26?material.Get():nullptr,&v,cpu);
   }
   cpu.ptr+=inc;
  }offset+=count;
 }
 list->Dispatch((width+7)/8,(height+15)/16,parallel?16:1);
 Transition(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 rd=output->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT readFootprint{};device->GetCopyableFootprints(&rd,0,1,0,&readFootprint,nullptr,nullptr,&total);auto readback=Buffer(device,total,D3D12_HEAP_TYPE_READBACK);
 D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=output.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=readFootprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 Require(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
 ComPtr<ID3D12Fence> fence;Require(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Require(queue->Signal(fence.Get(),1));HANDLE event=CreateEvent(nullptr,FALSE,FALSE,nullptr);Require(fence->SetEventOnCompletion(1,event));DWORD wait=WaitForSingleObject(event,30000);CloseHandle(event);if(wait!=WAIT_OBJECT_0)throw std::runtime_error("fixture GPU timeout");
 Require(readback->Map(0,nullptr,&mapped));std::vector<unsigned> result(width*height*4);
 for(UINT y=0;y<height;++y)std::memcpy(result.data()+y*width*4,static_cast<char*>(mapped)+readFootprint.Offset+y*readFootprint.Footprint.RowPitch,width*16);
 readback->Unmap(0,nullptr);return result;
}
