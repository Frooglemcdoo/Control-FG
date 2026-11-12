#pragma once
#include "d3d12_frozen_producer.h"
#include "d3d12_frozen_completion.h"
#include "d3d12_frozen_pipeline_bundle.h"
#include "d3d12_native_gi_textures.h"
namespace gi_reuse {
// The whole owner placed in D3D12SplitOwnerRetirement. The live host supplies
// native scene ownership and actual queue/recording evidence. This object must
// NOT be destroyed/rearmed merely because the producer or render scope ended.
class D3D12FrozenInvocation {
 using Ptr=Microsoft::WRL::ComPtr<D3D12FrozenPipelineBundle>;
 D3D12FrozenWorkingSet work_;
 D3D12GIRayConstantSnapshot constants_;
 D3D12GISbtSnapshot tables_;
 D3D12FrozenRaygenRecords records_;
 D3D12NativeGITextures textures_;
 Microsoft::WRL::ComPtr<ID3D12Resource> output_;
 Microsoft::WRL::ComPtr<ID3D12Device> device_;
 Ptr pipelines_;
 D3D12FrozenProducer::Inputs producer_;
 ComputeBindingSnapshot savedRay_,savedShade_;
 UINT64 bytes_=0;
 bool prepared_=false,producerAttempted_=false,producerRecorded_=false,consumerAttempted_=false,rearmed_=false;
 static UINT64 Allocation(ID3D12Device* device,ID3D12Resource* resource)noexcept {
  if(!device||!resource)return 0;const auto desc=resource->GetDesc();
  const auto bytes=device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
  return bytes==UINT64_MAX?0:bytes;
 }
public:
 struct Preparation {
  ID3D12Device* device=nullptr;
  D3D12FrozenPipelineBundle* pipelines=nullptr;
  ID3D12Resource* nativeSbt=nullptr;
  const void* nativeOwner=nullptr;
  D3D12_DISPATCH_RAYS_DESC dispatch{};
  D3D12FrozenProducer::Inputs ray{};
  UINT64 budget=0;
  float guideFraction=.5f;
 };
 D3D12FrozenInvocation()=default;
 D3D12FrozenInvocation(const D3D12FrozenInvocation&)=delete;
 D3D12FrozenInvocation& operator=(const D3D12FrozenInvocation&)=delete;
 HRESULT Prepare(const Preparation& p)noexcept {
  if(prepared_||producerAttempted_||(device_&&!rearmed_)||!p.device||!p.pipelines||!p.nativeOwner||!p.nativeSbt||
     !p.budget||!p.ray.nativeSingleRayAbi||!p.ray.sceneAndDescriptorLease||
     !p.ray.ray.Ready()||!p.ray.ray.rays||p.ray.ray.root.Get()!=p.pipelines->constants.NativeRoot()||
     p.dispatch.Width!=p.ray.width||p.dispatch.Height!=p.ray.height||p.dispatch.Depth!=1)return E_INVALIDARG;
  // Keep a failed partial allocation private. Its owner may be destroyed since
  // Prepare never records commands. It cannot be accidentally prepared again.
  if(device_&&(device_.Get()!=p.device||pipelines_.Get()!=p.pipelines||
     !work_.Matches(p.ray.width,p.ray.height,p.ray.slots)))return E_INVALIDARG;
  const bool reused=rearmed_;rearmed_=false;device_=p.device;pipelines_=p.pipelines;
  auto hr=reused?S_OK:work_.Initialize(p.device,D3D12FrozenWorkingSet::Describe(p.ray.width,p.ray.height,p.ray.slots),p.budget);
  if(FAILED(hr))return hr;
  hr=reused?S_OK:constants_.Initialize(p.device,p.pipelines->constants);if(FAILED(hr))return hr;
  hr=tables_.Initialize(p.device,p.nativeSbt,p.dispatch,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);if(FAILED(hr))return hr;
  Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
  hr=p.ray.ray.rays->QueryInterface(IID_PPV_ARGS(&properties));if(FAILED(hr))return hr;
  const auto* identifier=properties->GetShaderIdentifier(D3D12FrozenRaygenAugment::Export);
  if(!identifier)return E_INVALIDARG;
  hr=records_.Initialize(p.device,identifier,work_.Get(D3D12FrozenWorkingSet::Guides)->GetGPUVirtualAddress(),
   work_.Get(D3D12FrozenWorkingSet::Weights)->GetGPUVirtualAddress(),p.ray.width,p.ray.height,
   p.ray.frame,p.ray.epoch,p.ray.slots,p.ray.trainingParity,p.guideFraction);if(FAILED(hr))return hr;
  // Account actual committed storage, including constant/table/raygen snapshots.
  // Native borrowed textures are not new allocations made by this path.
  bytes_=work_.AllocationBytes();
  for(auto* resource:{constants_.Storage(),tables_.Storage(),records_.Resource()}){
   const auto bytes=Allocation(p.device,resource);
   if(!bytes||bytes_>p.budget||bytes>p.budget-bytes_)return E_OUTOFMEMORY;bytes_+=bytes;
  }
  if(!textures_.AcquireAtRay(p.nativeOwner,p.device,p.ray.width,p.ray.height))return E_INVALIDARG;
  producer_=p.ray;prepared_=true;return S_OK;
 }
 // Caller must first Take this WHOLE owner from the retirement pool. GPU
 // completion or the producer Reset alone is not sufficient. Changed extents,
 // native root/pipelines or device require a fresh owner after retirement.
 bool RearmAfterRetirement()noexcept {
  if(!prepared_||!producerRecorded_||rearmed_)return false;
  if(!constants_.RearmAfterRetirement()||!tables_.RearmAfterRetirement()||!records_.RearmAfterRetirement())return false;
  textures_.ResetAfterRetirement();output_.Reset();savedRay_={};savedShade_={};producer_={};bytes_=0;
  prepared_=producerAttempted_=producerRecorded_=consumerAttempted_=false;rearmed_=true;return true;
 }
 UINT64 AllocationBytes()const noexcept{return prepared_?bytes_:0;}
 bool AcceptProducer(ID3D12GraphicsCommandList4* list,D3D12FrozenProducer::Trace trace)const noexcept {
  return prepared_&&!producerAttempted_&&D3D12FrozenProducer::Accept(list,producer_,work_,constants_,tables_,records_,trace);
 }
 D3D12FrozenProducer::Result RecordProducer(ID3D12GraphicsCommandList4* list,D3D12FrozenProducer::Trace trace)noexcept {
  if(!AcceptProducer(list,trace))return D3D12FrozenProducer::Result::RejectedBeforeCommands;
  producerAttempted_=true;
  const auto result=D3D12FrozenProducer::Record(list,producer_,work_,constants_,tables_,records_,trace,savedRay_);
  producerRecorded_=result==D3D12FrozenProducer::Result::Recorded;return result;
 }
 struct Consumption {
  const void* nativeOwner=nullptr;
  const void* outputContainer=nullptr;
  bool producerSubmitted=false,sceneAndDescriptorLease=false;
  UINT frame=0,epoch=0;
  ComputeBindingSnapshot shade;
  GraphicsTableState graphics;
 };
 D3D12FrozenCompletion::Result RecordConsumer(ID3D12GraphicsCommandList4* list,const Consumption& p,
                                             D3D12FrozenCompletion::Trace trace)noexcept {
  if(!producerRecorded_||consumerAttempted_||!p.producerSubmitted||!p.sceneAndDescriptorLease||
     p.frame!=producer_.frame||p.epoch!=producer_.epoch)return D3D12FrozenCompletion::Result::RejectedBeforeCommands;
  // Host already claimed this consumer recording in the retirement pool. Even
  // admission failure cannot permit a second consumer attempt/full native ray.
  consumerAttempted_=true;
  D3D12FrozenCompletion::Inputs input;
  input.width=producer_.width;input.height=producer_.height;input.slots=producer_.slots;
  input.frame=p.frame;input.epoch=p.epoch;input.trainingParity=producer_.trainingParity;
  input.separateProducerSubmitted=p.producerSubmitted;input.sceneAndDescriptorLease=p.sceneAndDescriptorLease;
  savedShade_=p.shade;input.ray=savedRay_;input.shade=savedShade_;input.graphics=p.graphics;
  input.preservedTables=*tables_.Dispatch();input.rayConstants=constants_.Resource()->GetGPUVirtualAddress();
  input.hits=textures_.Hits();
  if(!textures_.ReadAtShade(p.nativeOwner,input.hitStates))return D3D12FrozenCompletion::Result::RejectedBeforeCommands;
  // Missing/changed output can still use the independent uniform-complement
  // recovery and original native GI6 shader. It does not trigger a full ray retry.
  if(textures_.AcquireOutputAtShade(p.outputContainer,device_.Get(),output_))input.output=output_.Get();
  return D3D12FrozenCompletion::RecordOrRecover(list,input,work_,pipelines_->evaluator,pipelines_->guide,records_,trace);
 }
};
}
