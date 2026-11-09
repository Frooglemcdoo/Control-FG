#include "gpu_test_device.h"
#include "../src/gi_reuse/producer_ray.h"
#include "../src/gi_reuse/producer_sampling.h"
#include <d3d12sdklayers.h>
#include <array>
using namespace gi_reuse;
struct Batch {uint first,count,frame;float scale;};
int main()try{
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device gpu(false);
 auto cpu=[&](UINT i){auto h=gpu.views->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*gpu.descriptorSize;return h;};
 auto handle=[&](UINT i){auto h=gpu.views->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(i)*gpu.descriptorSize;return h;};
 auto texture=[&](UINT slot,const std::array<float,4>& value,bool integer=false){
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=2;rd.Height=1;rd.DepthOrArraySize=rd.MipLevels=1;
  rd.Format=integer?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R32G32B32A32_FLOAT;rd.SampleDesc.Count=1;
  ComPtr<ID3D12Resource> resource;Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&resource)));
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 size=0;gpu.device->GetCopyableFootprints(&rd,0,1,0,&footprint,nullptr,nullptr,&size);
  std::vector<unsigned char> bytes(static_cast<std::size_t>(size),0);
  if(!integer)for(unsigned p=0;p<2;++p)std::memcpy(bytes.data()+p*16,value.data(),16);
  auto upload=gpu.Upload(bytes);D3D12_TEXTURE_COPY_LOCATION from{},to{};
  from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
  to.pResource=resource.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);gpu.Transition(resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=rd.Format;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  if(integer){view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;view.Texture2DArray.MipLevels=1;view.Texture2DArray.ArraySize=1;}
  else{view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Texture2D.MipLevels=1;}
  gpu.device->CreateShaderResourceView(resource.Get(),&view,cpu(slot));gpu.retained.push_back(resource);
 };
 // Valid null descriptors for unused table slots; all actually-read inputs below
 // have real resources. These are synthetic primary surfaces, not capture parity.
 D3D12_SHADER_RESOURCE_VIEW_DESC nullView{};nullView.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
 nullView.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;nullView.Texture2D.MipLevels=1;nullView.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 for(UINT i=0;i<27;++i)gpu.device->CreateShaderResourceView(nullptr,&nullView,cpu(i));
 texture(0,{0,0,0,0});texture(1,{0,0,0,0});texture(2,{1,0,0,0});texture(3,{.5f,0,0,0});texture(12,{0,0,1,0});texture(24,{0,0,0,0},true);
 auto material=gpu.Upload(std::vector<uint>{0,0});gpu.View(material.Get(),4,8,1,false);
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,27,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,2,0,0}};
 D3D12_ROOT_PARAMETER params[6]{};
 for(UINT i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,ranges+i};}
 const UINT registers[]={0,1,4,5};for(UINT i=2;i<6;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor={registers[i-2],0};}
 D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
 sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;
 sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderRegister=6;sampler.RegisterSpace=1;
 D3D12_ROOT_SIGNATURE_DESC rd{6,params,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto shader=Read("gi-producer-proposals.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC cp{};cp.pRootSignature=root.Get();cp.CS={shader.data(),shader.size()};ComPtr<ID3D12PipelineState> pipeline;
 Check(gpu.device->CreateComputePipelineState(&cp,IID_PPV_ARGS(&pipeline)));
 std::vector<float> system(320,0),lighting(128,0);system[1*4+2]=.5f;system[1*4+3]=1;system[54*4+2]=-1;
 for(UINT base:{2u,14u,22u})for(UINT i=0;i<4;++i)system[(base+i)*4+i]=1;
 for(UINT i=0;i<3;++i)system[(6+i)*4+i]=1;
 for(UINT i=0;i<4;++i)lighting[17*4+i]=1;
 auto sys=gpu.Upload(system),light=gpu.Upload(lighting);
 std::vector<uint> gi(64,0);gi[2]=6;gi[3]=7;auto constants=gpu.Upload(gi);
 gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pipeline.Get());
 gpu.list->SetComputeRootDescriptorTable(0,handle(0));gpu.list->SetComputeRootConstantBufferView(2,sys->GetGPUVirtualAddress());
 gpu.list->SetComputeRootConstantBufferView(3,light->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(4,constants->GetGPUVirtualAddress());
 struct Case {ComPtr<ID3D12Resource> rays,batches,readRays,readBatches;};std::array<Case,4> cases;
 const std::vector<unsigned char> sentinel(13*sizeof(GIProducerRay),0xcd),batchSentinel(3*sizeof(Batch),0xcd);
 for(UINT c=0;c<cases.size();++c){
  auto& item=cases[c];std::vector<uint> controls(64,0);controls[0]=257;controls[1]=11;controls[2]=2;controls[3]=1;controls[5]=2;controls[6]=6;
  const float baseRange=5;std::memcpy(&controls[7],&baseRange,4);
  if(c==1)controls[1]=0;if(c==2)controls[2]=3;if(c==3)controls[6]=5;
  auto control=gpu.Upload(controls);gpu.list->SetComputeRootConstantBufferView(5,control->GetGPUVirtualAddress());
  item.rays=gpu.Buffer(sentinel.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
  item.batches=gpu.Buffer(batchSentinel.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
  auto a=gpu.Upload(sentinel),b=gpu.Upload(batchSentinel);gpu.list->CopyResource(item.rays.Get(),a.Get());gpu.list->CopyResource(item.batches.Get(),b.Get());
  gpu.Transition(item.rays.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);gpu.Transition(item.batches.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  const UINT slot=27+c*2;gpu.View(item.rays.Get(),slot,sizeof(GIProducerRay),13,true);gpu.View(item.batches.Get(),slot+1,sizeof(Batch),3,true);
  gpu.list->SetComputeRootDescriptorTable(1,handle(slot));gpu.list->Dispatch(1,1,1);
  gpu.Transition(item.rays.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.Transition(item.batches.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  item.readRays=gpu.Buffer(sentinel.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  item.readBatches=gpu.Buffer(batchSentinel.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  gpu.list->CopyResource(item.readRays.Get(),item.rays.Get());gpu.list->CopyResource(item.readBatches.Get(),item.batches.Get());
 }
 gpu.Submit();
 for(UINT c=0;c<cases.size();++c){GIProducerRay* rays=nullptr;Batch* batches=nullptr;
  Check(cases[c].readRays->Map(0,nullptr,reinterpret_cast<void**>(&rays)));Check(cases[c].readBatches->Map(0,nullptr,reinterpret_cast<void**>(&batches)));
  for(UINT p=0;p<2;++p){
   if(c==1){Require(!std::memcmp(rays,sentinel.data(),sentinel.size())&&!std::memcmp(batches,batchSentinel.data(),batchSentinel.size()),"Disarmed tile touched scratch");continue;}
   if(c){Require(batches[p].count==0,"Invalid tile published batch");continue;}
   Require(batches[p].first==p*6&&batches[p].count==6&&batches[p].frame==257&&batches[p].scale==1,"Valid tile batch mismatch");
   const float nx=-2.6f/4.38f,nz=2.38f/4.38f;const float wx=p? .5f:-.5f;
   const float viewLength=std::sqrt(wx*wx+2.25f);
   for(UINT r=0;r<6;++r){const auto& value=rays[p*6+r];auto random=GIProducerSample(p,0,257,7,r,6);
    Require(value.proposalVerified==1&&value.sceneRevision==11&&value.frameTag==257&&value.configuredCount==6&&value.rayIndex==r&&value.pixelX==p&&value.pixelY==0,"Producer identity mismatch");
    Require(std::abs(GIDot(value.direction,value.direction)-1)<1e-5f,"Nonunit proposal");
    const float cosine=value.direction.x*nx+value.direction.y*nx+value.direction.z*nz;
    Require(std::abs(cosine-value.cosine)<1e-5f&&std::abs(value.cosine-std::sqrt(random.v))<1e-6f,"Cosine PDF direction mismatch");
    Require(value.rangeRandom==random.range&&value.selectionRandom==random.selection&&value.baseRange==5&&value.tMin==0,"Proposal random/range mismatch");
    const float response=value.cosine*.3183098861837907f;
    Require(std::abs(value.primaryBrdfCos.x-response)<1e-6f&&std::abs(value.primaryBrdfCos.y-response)<1e-6f&&std::abs(value.primaryBrdfCos.z-response)<1e-6f,"Primary BRDF/PDF cancellation mismatch");
    Require(std::abs(value.origin.x-(wx-.001f*wx/viewLength+.002f*nx))<1e-5f&&std::abs(value.origin.y-.002f*nx)<1e-5f&&std::abs(value.origin.z-(.5f-.001f*1.5f/viewLength+.002f*nz))<1e-5f,"Ray origin used wrong native bias");
   }
  }
  Require(!std::memcmp(rays+12,sentinel.data()+12*sizeof(GIProducerRay),sizeof(GIProducerRay)),"Ray guard overwritten");
  Require(!std::memcmp(batches+2,batchSentinel.data()+2*sizeof(Batch),sizeof(Batch)),"Batch guard overwritten");
  cases[c].readRays->Unmap(0,nullptr);cases[c].readBatches->Unmap(0,nullptr);
 }
 ComPtr<ID3D12InfoQueue> info;Check(gpu.device.As(&info));
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<unsigned char> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,message,&size));Require(message->Severity!=D3D12_MESSAGE_SEVERITY_ERROR&&message->Severity!=D3D12_MESSAGE_SEVERITY_CORRUPTION,message->pDescription);}
 std::cout<<"PASS WARP proposal generation: actual receiver reconstruction, native ray bias, complete strata/PDF, independent random dimensions, invalid tiles and output guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
