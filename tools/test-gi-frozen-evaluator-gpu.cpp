#include "gpu_test_device.h"
#include "../src/gi_reuse/frozen_guide_training.h"
#include <array>
using namespace gi_reuse;
int main()try{
 Device d(false);constexpr UINT width=18,height=3,count=width/2*height;
 auto cpu=[&](UINT slot){auto h=d.views->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(slot)*d.descriptorSize;return h;};
 auto gpu=[&](UINT slot){auto h=d.views->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(slot)*d.descriptorSize;return h;};
 auto texture=[&](UINT slot,DXGI_FORMAT format,bool array,bool write,const std::vector<unsigned char>& data){
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
  desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=format;if(write)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  ComPtr<ID3D12Resource> out;Check(d.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,write?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&out)));
  if(write){D3D12_UNORDERED_ACCESS_VIEW_DESC view{};view.Format=format;view.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;d.device->CreateUnorderedAccessView(out.Get(),nullptr,&view,cpu(slot));}
  else{
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;d.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
   const UINT pixelBytes=format==DXGI_FORMAT_R32_UINT?4:16;std::vector<unsigned char> uploadBytes(static_cast<size_t>(bytes));
   for(UINT y=0;y<height;++y)memcpy(uploadBytes.data()+y*footprint.Footprint.RowPitch,data.data()+y*width*pixelBytes,width*pixelBytes);
   auto upload=d.Upload(uploadBytes);D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
   to.pResource=out.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;d.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
   d.Transition(out.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
   D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=format;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   if(array){view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;view.Texture2DArray.MipLevels=1;view.Texture2DArray.ArraySize=1;}
   else{view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Texture2D.MipLevels=1;}d.device->CreateShaderResourceView(out.Get(),&view,cpu(slot));
  }
  d.retained.push_back(out);return out;
 };
 D3D12_SHADER_RESOURCE_VIEW_DESC nullView{};nullView.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;nullView.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
 nullView.Texture2D.MipLevels=1;nullView.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 for(UINT i=0;i<27;++i)d.device->CreateShaderResourceView(nullptr,&nullView,cpu(i));
 std::vector<unsigned char> surface(width*height*16),depth=surface,clip=surface,materials(width*height*4);
 for(UINT i=0;i<width*height;++i){float value=1;memcpy(depth.data()+i*16,&value,4);value=.5f;memcpy(clip.data()+i*16,&value,4);UINT mat=(i%width)%3==0?65535u:0u;memcpy(materials.data()+i*4,&mat,4);}
 texture(0,DXGI_FORMAT_R32G32B32A32_FLOAT,false,false,surface);texture(2,DXGI_FORMAT_R32G32B32A32_FLOAT,false,false,depth);
 texture(3,DXGI_FORMAT_R32G32B32A32_FLOAT,false,false,clip);texture(24,DXGI_FORMAT_R32_UINT,true,false,materials);
 const auto color=texture(27,DXGI_FORMAT_R32G32B32A32_FLOAT,false,true,{}),distance=texture(28,DXGI_FORMAT_R32_FLOAT,false,true,{});
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,27,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,0}};
 D3D12_ROOT_PARAMETER parameters[8]{};
 for(UINT i=0;i<2;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[i].DescriptorTable={1,ranges+i};}
 parameters[2].ParameterType=parameters[3].ParameterType=parameters[5].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;
 parameters[2].Descriptor={1,0};parameters[3].Descriptor={4,0};parameters[5].Descriptor={1,18};
 parameters[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[4].Constants={0,18,8};
 parameters[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[6].Descriptor={0,18};
 parameters[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;parameters[7].Descriptor={0,18};
 D3D12_ROOT_SIGNATURE_DESC rd{8,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-frozen-evaluator-fixture.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
 ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 std::vector<float> lighting(128),system(320);for(UINT i=0;i<4;++i)lighting[17*4+i]=1;
 system[6*4]=system[7*4+1]=system[8*4+2]=1;for(UINT i=0;i<4;++i)system[(22+i)*4+i]=1;
 system[1*4+2]=1.0f/width;system[1*4+3]=1.0f/height;system[54*4+2]=-1;const UINT sampleFrame=211;memcpy(&system[69*4+2],&sampleFrame,4);
 std::vector<UINT> constants(64);constants[2]=1;
 auto light=d.Upload(lighting),sys=d.Upload(system),gi=d.Upload(constants);
 std::vector<float> weight(count);const float cycle[]={0,.5f,1,1.125f};for(UINT i=0;i<count;++i)weight[i]=cycle[i%4];auto weights=d.Upload(weight);
 const auto training=d.Buffer((count+1)*sizeof(GITrainingRecord),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 std::vector<unsigned char> sentinel((count+1)*sizeof(GITrainingRecord),0xcd);auto initial=d.Upload(sentinel);d.list->CopyResource(training.Get(),initial.Get());d.Transition(training.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootDescriptorTable(0,gpu(0));d.list->SetComputeRootDescriptorTable(1,gpu(27));
 d.list->SetComputeRootConstantBufferView(2,light->GetGPUVirtualAddress());d.list->SetComputeRootConstantBufferView(3,gi->GetGPUVirtualAddress());d.list->SetComputeRootConstantBufferView(5,sys->GetGPUVirtualAddress());
 d.list->SetComputeRootUnorderedAccessView(6,training->GetGPUVirtualAddress());d.list->SetComputeRootShaderResourceView(7,weights->GetGPUVirtualAddress());
 for(UINT phase=0;phase<2;++phase){UINT control[]={width,height,100,7,phase,phase,1,0};d.list->SetComputeRoot32BitConstants(4,8,control,0);d.list->Dispatch(2,1,1);d.Uav(color.Get());d.Uav(distance.Get());d.Uav(training.Get());}
 struct ReadTexture{ComPtr<ID3D12Resource> data;D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;};
 auto readTexture=[&](ID3D12Resource* source){ReadTexture r{};const auto desc=source->GetDesc();UINT64 bytes=0;d.device->GetCopyableFootprints(&desc,0,1,0,&r.footprint,nullptr,nullptr,&bytes);r.data=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  d.Transition(source,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=source;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  to.pResource=r.data.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=r.footprint;d.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);return r;};
 auto colorRead=readTexture(color.Get()),distanceRead=readTexture(distance.Get());auto trainingRead=d.Buffer(sentinel.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(training.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyResource(trainingRead.Get(),training.Get());d.Submit();
 unsigned char* colors=nullptr;unsigned char* distances=nullptr;GITrainingRecord* records=nullptr;
 Check(colorRead.data->Map(0,nullptr,reinterpret_cast<void**>(&colors)));Check(distanceRead.data->Map(0,nullptr,reinterpret_cast<void**>(&distances)));Check(trainingRead->Map(0,nullptr,reinterpret_cast<void**>(&records)));
 for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){
  const bool valid=x%7!=0,miss=x%3==0;const UINT index=y*(width/2)+x/2;const float factor=(x&1)?weight[index]:1;
  const auto* c=reinterpret_cast<const float*>(colors+y*colorRead.footprint.Footprint.RowPitch+x*16);
  const float hit=*reinterpret_cast<const float*>(distances+y*distanceRead.footprint.Footprint.RowPitch+x*4);
  const float values[]={miss?10.f:2.f,miss?21.f:6.f,miss?44.f:12.f};
  for(UINT channel=0;channel<3;++channel)Require(std::abs(c[channel]-(valid?values[channel]*factor:0))<1e-5f,"weighted hit/miss color or column publication wrong");
  Require(c[3]==(valid?1.f:0.f),"receiver alpha changed");Require(hit==(valid?(miss?5.f:2.5f):65504.f),"weight changed native hit distance");
  if(!(x&1)){
   const auto& r=records[index];Require(r.key.valid==UINT(valid),"training validity wrong");
   if(valid){Require(std::abs(GIDot(r.direction,r.direction)-1)<1e-5f,"training direction nonunit");const float energy=values[0]*.2126f+values[1]*.7152f+values[2]*.0722f;Require(std::abs(r.energy-energy)<1e-5f,"training omitted hit/miss energy");}
  }
 }
 Require(!memcmp(records+count,sentinel.data()+count*sizeof(GITrainingRecord),sizeof(GITrainingRecord)),"padded dispatch overwrote training guard");
 colorRead.data->Unmap(0,nullptr);distanceRead.data->Unmap(0,nullptr);trainingRead->Unmap(0,nullptr);
 std::cout<<"PASS generated evaluator GPU fixture: disjoint/padded coverage, training writes, all hit/miss radiance weighted, zero contribution retained, native distance/alpha. Lighting helper is stubbed; no gameplay parity claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
