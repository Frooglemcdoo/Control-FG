#pragma once
#include "nrd_diffuse_session.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <map>
#include <string>

// Dedicated D3D12 execution backend. Caller supplies exact resource states,
// restores its own bindings, and retires a recording only after both GPU
// completion and command-list Reset. No native resources are guessed here.
namespace control_nrd {
class D3D12Backend {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 struct Texture {Ptr<ID3D12Resource> resource;D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;};
 Ptr<ID3D12Device> device;Ptr<ID3D12RootSignature> root;
 std::vector<Ptr<ID3D12PipelineState>> pipelines;
 std::vector<Texture> permanent,transient;
 UINT increment=0,srvCount=0,uavCount=0,constantStride=0,maxSets=0;
 std::uint16_t width=0,height=0;
 bool poisoned=false;
 static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("NRD D3D12 operation failed");}
 static DXGI_FORMAT Format(nrd::Format f){switch(f){
#define F(n,d) case nrd::Format::n:return DXGI_FORMAT_##d
 F(R8_UNORM,R8_UNORM);F(R8_UINT,R8_UINT);F(RG8_UNORM,R8G8_UNORM);F(RGBA8_UNORM,R8G8B8A8_UNORM);
 F(R16_SFLOAT,R16_FLOAT);F(R16_UNORM,R16_UNORM);F(R16_UINT,R16_UINT);
 F(RG16_SFLOAT,R16G16_FLOAT);F(RG16_UNORM,R16G16_UNORM);F(RG16_UINT,R16G16_UINT);
 F(RGBA16_SFLOAT,R16G16B16A16_FLOAT);F(RGBA16_UNORM,R16G16B16A16_UNORM);
 F(R32_SFLOAT,R32_FLOAT);F(R32_UINT,R32_UINT);F(RG32_SFLOAT,R32G32_FLOAT);F(RG32_UINT,R32G32_UINT);
 F(RGBA32_SFLOAT,R32G32B32A32_FLOAT);F(RGBA32_UINT,R32G32B32A32_UINT);
 F(R10_G10_B10_A2_UNORM,R10G10B10A2_UNORM);F(R11_G11_B10_UFLOAT,R11G11B10_FLOAT);
#undef F
 default:throw std::runtime_error("Unsupported NRD pool format");}}
 Texture MakeTexture(const nrd::TextureDesc& t){
  if(!t.downsampleFactor)throw std::runtime_error("Invalid NRD pool extent");
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  d.Width=(width+t.downsampleFactor-1)/t.downsampleFactor;d.Height=(height+t.downsampleFactor-1)/t.downsampleFactor;
  d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Format=Format(t.format);d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;h.CreationNodeMask=h.VisibleNodeMask=1;
  Texture out;Check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,out.state,nullptr,IID_PPV_ARGS(&out.resource)));return out;
 }
 static void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES& state,D3D12_RESOURCE_STATES next){
  D3D12_RESOURCE_BARRIER b{};
  if(state!=next){b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,state,next};list->ResourceBarrier(1,&b);state=next;}
  else if(next==D3D12_RESOURCE_STATE_UNORDERED_ACCESS){b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=resource;list->ResourceBarrier(1,&b);}
 }
public:
 struct Arena {ID3D12DescriptorHeap* heap=nullptr;UINT first=0,count=0;};
 struct External {ID3D12Resource* resource=nullptr;D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;};
 using Inputs=std::map<nrd::ResourceType,External>;
 struct Recording {
  Ptr<ID3D12DescriptorHeap> heap;Ptr<ID3D12Resource> constants;
  // Retains external resources referenced by the immutable descriptor table.
  std::vector<Ptr<ID3D12Resource>> retained;
  Ptr<ID3D12RootSignature> root;
  std::vector<Ptr<ID3D12PipelineState>> pipelines;
  void* mapped=nullptr;
  UINT first=0;bool borrowed=false;
  ~Recording(){if(mapped)constants->Unmap(0,nullptr);}
  Recording()=default;Recording(const Recording&)=delete;Recording&operator=(const Recording&)=delete;
 };
 D3D12Backend(ID3D12Device* d,const nrd::InstanceDesc& desc,std::uint16_t w,std::uint16_t h):device(d),width(w),height(h){
  if(!d||!w||!h)throw std::runtime_error("NRD device or extent missing");
  increment=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  srvCount=(std::max)(1u,desc.descriptorPoolDesc.perSetTexturesMaxNum);uavCount=(std::max)(1u,desc.descriptorPoolDesc.perSetStorageTexturesMaxNum);
  constantStride=(desc.constantBufferMaxDataSize+255)&~255u;
  maxSets=desc.descriptorPoolDesc.setsMaxNum;
  D3D12_DESCRIPTOR_RANGE ranges[2]{};
  ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srvCount,desc.resourcesBaseRegisterIndex,desc.resourcesSpaceIndex,0};
  ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,uavCount,desc.resourcesBaseRegisterIndex,desc.resourcesSpaceIndex,0};
  D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[0].Descriptor={desc.constantBufferRegisterIndex,desc.constantBufferAndSamplersSpaceIndex};
  for(unsigned i=0;i<2;++i){params[i+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i+1].DescriptorTable={1,&ranges[i]};}
  std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(desc.samplersNum);
  for(unsigned i=0;i<desc.samplersNum;++i){auto&s=samplers[i];
   s.Filter=desc.samplers[i]==nrd::Sampler::NEAREST_CLAMP?D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
   s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
   s.ShaderRegister=desc.samplersBaseRegisterIndex+i;s.RegisterSpace=desc.constantBufferAndSamplersSpaceIndex;
  }
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;rd.NumStaticSamplers=UINT(samplers.size());rd.pStaticSamplers=samplers.data();
  Ptr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
  Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
  pipelines.resize(desc.pipelinesNum);
  for(unsigned i=0;i<desc.pipelinesNum;++i){const auto&p=desc.pipelines[i];auto shader=p.computeShaderDXIL.size?p.computeShaderDXIL:p.computeShaderDXBC;
   if(!shader.bytecode||!shader.size)throw std::runtime_error("NRD GPU shaders are not embedded; CPU-only library cannot render");
   D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS={shader.bytecode,SIZE_T(shader.size)};
   Check(device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipelines[i])));
  }
  for(unsigned i=0;i<desc.permanentPoolSize;++i)permanent.push_back(MakeTexture(desc.permanentPool[i]));
  for(unsigned i=0;i<desc.transientPoolSize;++i)transient.push_back(MakeTexture(desc.transientPool[i]));
 }
 // Use one owner/queue and record only after the previous session was submitted.
 // This class deliberately does not signal fences or decide native list lifetime.
 UINT MaxDescriptors()const noexcept{return maxSets*(srvCount+uavCount);}
 void AllocateRecording(const std::vector<Dispatch>& plan,const Inputs& inputs,Recording& recording,Arena arena={nullptr,0,0}){
  if(poisoned||plan.empty()||recording.heap)throw std::runtime_error("Invalid NRD recording");
  // Complete validation before touching the command list.
  for(const auto&d:plan){
   if(d.pipeline>=pipelines.size()||!d.width||!d.height||d.constants.size()>constantStride)throw std::runtime_error("Invalid NRD dispatch");
   unsigned reads=0,writes=0;
   for(const auto&r:d.resources){
    if(r.descriptorType==nrd::DescriptorType::TEXTURE)++reads;else if(r.descriptorType==nrd::DescriptorType::STORAGE_TEXTURE)++writes;else throw std::runtime_error("Invalid NRD descriptor type");
    if(r.type==nrd::ResourceType::PERMANENT_POOL){if(r.indexInPool>=permanent.size())throw std::runtime_error("NRD permanent index");}
    else if(r.type==nrd::ResourceType::TRANSIENT_POOL){if(r.indexInPool>=transient.size())throw std::runtime_error("NRD transient index");}
    else {auto it=inputs.find(r.type);if(it==inputs.end()||!it->second.resource)throw std::runtime_error("Required NRD input missing");}
   }
   if(reads>srvCount||writes>uavCount)throw std::runtime_error("NRD descriptor bounds");
  }
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=UINT(plan.size())*(srvCount+uavCount);hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if(arena.heap){
   const auto d=arena.heap->GetDesc();Ptr<ID3D12Device> owner;
   if(d.Type!=hd.Type||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||arena.first>d.NumDescriptors||arena.count>d.NumDescriptors-arena.first||arena.count<hd.NumDescriptors||
      FAILED(arena.heap->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device.Get())throw std::runtime_error("Invalid native NRD descriptor arena");
   recording.heap=arena.heap;recording.first=arena.first;recording.borrowed=true;
  }else Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&recording.heap)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=UINT64(constantStride)*plan.size();bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&recording.constants)));
  D3D12_RANGE empty{};Check(recording.constants->Map(0,&empty,&recording.mapped));
  for(const auto&kv:inputs)recording.retained.emplace_back(kv.second.resource);
  for(const auto&t:permanent)recording.retained.push_back(t.resource);
  for(const auto&t:transient)recording.retained.push_back(t.resource);
  recording.root=root;recording.pipelines=pipelines;
 }
 // AllocateRecording must finish before the enclosing transaction records any
 // commands. The plan/inputs remain immutable between allocation and dispatch.
 void RecordPrepared(ID3D12GraphicsCommandList* list,const std::vector<Dispatch>& plan,Inputs& inputs,Recording& recording){
  if(poisoned||!list||!recording.heap||!recording.constants||!recording.mapped)throw std::runtime_error("NRD recording was not prepared");
  auto cpu=recording.heap->GetCPUDescriptorHandleForHeapStart();auto gpu=recording.heap->GetGPUDescriptorHandleForHeapStart();
  cpu.ptr+=SIZE_T(recording.first)*increment;gpu.ptr+=UINT64(recording.first)*increment;
  auto ch=[&](UINT i){return D3D12_CPU_DESCRIPTOR_HANDLE{cpu.ptr+SIZE_T(i)*increment};};
  auto gh=[&](UINT i){return D3D12_GPU_DESCRIPTOR_HANDLE{gpu.ptr+UINT64(i)*increment};};
  auto*heap=recording.heap.Get();if(!recording.borrowed)list->SetDescriptorHeaps(1,&heap);list->SetComputeRootSignature(root.Get());
  for(UINT i=0;i<plan.size();++i){const auto&d=plan[i];const UINT base=i*(srvCount+uavCount);
   // Deterministic null descriptors for unused entries of the shared layout.
   D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
   D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=srv.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
   for(UINT j=0;j<srvCount;++j)device->CreateShaderResourceView(nullptr,&srv,ch(base+j));
   for(UINT j=0;j<uavCount;++j)device->CreateUnorderedAccessView(nullptr,nullptr,&uav,ch(base+srvCount+j));
   UINT read=0,write=0;
   for(const auto&r:d.resources){ID3D12Resource* resource;D3D12_RESOURCE_STATES* state;
    if(r.type==nrd::ResourceType::PERMANENT_POOL||r.type==nrd::ResourceType::TRANSIENT_POOL){auto&t=(r.type==nrd::ResourceType::PERMANENT_POOL?permanent:transient).at(r.indexInPool);resource=t.resource.Get();state=&t.state;}
    else{auto&t=inputs.at(r.type);resource=t.resource;state=&t.state;}
    const bool storage=r.descriptorType==nrd::DescriptorType::STORAGE_TEXTURE;
    Barrier(list,resource,*state,storage?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if(storage){uav.Format=resource->GetDesc().Format;device->CreateUnorderedAccessView(resource,nullptr,&uav,ch(base+srvCount+write++));}
    else{srv.Format=resource->GetDesc().Format;device->CreateShaderResourceView(resource,&srv,ch(base+read++));}
   }
   if(!d.constants.empty())std::memcpy(static_cast<std::uint8_t*>(recording.mapped)+SIZE_T(i)*constantStride,d.constants.data(),d.constants.size());
   list->SetPipelineState(pipelines[d.pipeline].Get());list->SetComputeRootConstantBufferView(0,recording.constants->GetGPUVirtualAddress()+UINT64(i)*constantStride);
   list->SetComputeRootDescriptorTable(1,gh(base));list->SetComputeRootDescriptorTable(2,gh(base+srvCount));list->Dispatch(d.width,d.height,1);
  }
 }
 // Once commands or tracked states may have changed, any exception/cancellation
 // requires recreating the backend after retiring its recordings. Clearing NRD
 // history alone does not restore a cancelled D3D12 resource transition.
 void AbandonAfterReset()noexcept{poisoned=true;}
 void Record(ID3D12GraphicsCommandList* list,const std::vector<Dispatch>& plan,Inputs& inputs,Recording& recording){
  if(poisoned)throw std::runtime_error("NRD backend requires recreation after abandoned recording");
  try{AllocateRecording(plan,inputs,recording);RecordPrepared(list,plan,inputs,recording);}catch(...){poisoned=true;throw;}
 }

};
}
