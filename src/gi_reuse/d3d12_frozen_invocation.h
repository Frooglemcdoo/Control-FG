#pragma once
#include "d3d12_frozen_producer.h"
#include "d3d12_frozen_completion.h"
#include "d3d12_frozen_pipeline_bundle.h"
#include "d3d12_native_gi_textures.h"
#include "d3d12_native_gi_heap_lease.h"
#include "d3d12_native_hit_snapshot.h"
#include "frozen_allocation_budget.h"
namespace gi_reuse {
// The whole owner placed in D3D12SplitOwnerRetirement. The live host supplies
// native scene ownership and actual queue/recording evidence. This object must
// NOT be destroyed/rearmed merely because the producer or render scope ended.
class D3D12FrozenInvocation {
 using Ptr=Microsoft::WRL::ComPtr<D3D12FrozenPipelineBundle>;
 // Declared first so the reservation is released AFTER all GPU resources.
 FrozenAllocationReservation allocationReservation_;
 D3D12FrozenWorkingSet work_;
 D3D12GIRayConstantSnapshot constants_;
 D3D12GISbtSnapshot tables_;
 D3D12FrozenRaygenRecords records_;
 D3D12NativeGITextures textures_;
 D3D12NativeGIHeapLease nativeHeaps_;
 D3D12NativeHitSnapshot hitSources_;
 Microsoft::WRL::ComPtr<ID3D12Resource> output_;
 Microsoft::WRL::ComPtr<ID3D12Device> device_;
 Ptr pipelines_;
 D3D12FrozenProducer::Inputs producer_;
 ComputeBindingSnapshot savedRay_,savedShade_;
 ID3D12CommandList* originalProducer_=nullptr; // Identity only; router owns its recording seal.
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
  NativeGIRecording recording{};
  // Captured from the matching original native writer, before scheduling/reuse.
  const NativeGITable* nativeHitTable=nullptr;
  const std::vector<PTNativeBinding::Binding>* nativeBindings=nullptr;
  const std::vector<D3D12NativeHitSnapshot::Source>* hitSources=nullptr;
  UINT64 writerVersion=0;
  D3D12_DISPATCH_RAYS_DESC dispatch{};
  D3D12FrozenProducer::Inputs ray{};
  UINT64 budget=0;
  std::shared_ptr<FrozenAllocationBudget> memoryBudget;
  float guideFraction=.5f;
 };
 // CPU-only committed allocation preflight, including retained allocations
 // while growing retired buffers. Zero means invalid/unrepresentable. The host
 // must reserve this peak against active+idle owners BEFORE calling Prepare.
 UINT64 RequiredPeakBytes(const Preparation& p)const noexcept {
  if(!p.device||!p.nativeSbt||!p.nativeBindings||!p.hitSources||!p.nativeHitTable||!p.budget)return 0;
  try {
   const auto layout=D3D12FrozenWorkingSet::Describe(p.ray.width,p.ray.height,p.ray.slots);
   if(!layout||(device_&&(device_.Get()!=p.device||!work_.Matches(p.ray.width,p.ray.height,p.ray.slots))))return 0;
   std::vector<NativeHitSnapshotPlan::Allocation> allocations;allocations.reserve(p.hitSources->size());
   for(const auto& source:*p.hitSources)allocations.push_back({source.token,source.storage.address,source.storage.bytes});
   NativeHitSnapshotPlan plan;if(!plan.Build(*p.nativeBindings,std::move(allocations),p.budget))return 0;
   const auto base=p.nativeSbt->GetGPUVirtualAddress();const auto sourceDesc=p.nativeSbt->GetDesc();
   if(sourceDesc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!base||
      p.dispatch.RayGenerationShaderRecord.StartAddress!=base)return 0;
   UINT64 end=32;
   for(const auto table:{p.dispatch.MissShaderTable,p.dispatch.HitGroupTable}){
    if(table.StartAddress<base)return 0;const auto offset=table.StartAddress-base;
    if(offset>sourceDesc.Width||table.SizeInBytes>sourceDesc.Width-offset)return 0;
    end=(std::max)(end,offset+table.SizeInBytes);
   }
   if(end>16ull*1024*1024||p.nativeHitTable->bytes.empty())return 0;
   UINT64 peak=0;
   const auto add=[&](UINT64 width,D3D12_RESOURCE_FLAGS flags,ID3D12Resource* retained){
    if(!width)return false;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=width;desc.Height=1;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;desc.Flags=flags;
    if(retained){const auto old=retained->GetDesc();
     if(old.Dimension!=desc.Dimension||old.Flags!=flags)return false;
     if(old.Width>=width)desc=old;
     else {const auto bytes=Allocation(p.device,retained);if(!bytes||bytes>p.budget-peak)return false;peak+=bytes;}
    }
    const auto bytes=p.device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
    if(!bytes||bytes==UINT64_MAX||bytes>p.budget-peak)return false;peak+=bytes;return true;
   };
   for(unsigned n=0;n<4;++n)if(!add(layout.bytes[n],D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      work_.Get(static_cast<D3D12FrozenWorkingSet::Buffer>(n))))return 0;
   if(!add(D3D12GIRayConstantSnapshot::Bytes,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,constants_.Storage())||
      !add((end+63)&~UINT64(63),D3D12_RESOURCE_FLAG_NONE,tables_.Storage())||
      !add(D3D12FrozenRaygenRecords::Bytes,D3D12_RESOURCE_FLAG_NONE,records_.Storage())||
      !add(plan.Bytes(),D3D12_RESOURCE_FLAG_NONE,hitSources_.Storage())||
      !add(p.nativeHitTable->bytes.size(),D3D12_RESOURCE_FLAG_NONE,tables_.HitUploadStorage()))return 0;
   return peak;
  }catch(...){return 0;}
 }
 UINT64 RetainedAllocationBytes()const noexcept {
  UINT64 bytes=work_.AllocationBytes();
  for(auto* resource:{constants_.Storage(),tables_.Storage(),records_.Storage(),hitSources_.Storage(),tables_.HitUploadStorage()}){
   if(!resource)continue;const auto allocation=Allocation(device_.Get(),resource);
   if(!allocation||allocation>UINT64_MAX-bytes)return UINT64_MAX;bytes+=allocation;
  }
  return bytes;
 }
 D3D12FrozenInvocation()=default;
 D3D12FrozenInvocation(const D3D12FrozenInvocation&)=delete;
 D3D12FrozenInvocation& operator=(const D3D12FrozenInvocation&)=delete;
 HRESULT Prepare(const Preparation& p)noexcept {
  if(prepared_||producerAttempted_||(device_&&!rearmed_)||!p.device||!p.pipelines||!p.nativeOwner||!p.nativeSbt||
     !p.budget||!p.nativeHitTable||!p.nativeBindings||!p.hitSources||!p.writerVersion||
     p.nativeHitTable->pipeline!=reinterpret_cast<UINT64>(p.ray.ray.rays.Get())||
     p.nativeHitTable->version!=p.writerVersion||p.recording.device!=p.device||p.recording.heaps.frame!=p.ray.frame||
     p.recording.heaps.deviceEpoch!=p.ray.epoch||!p.ray.nativeSingleRayAbi||!p.ray.sceneAndDescriptorLease||
     !p.ray.ray.Ready()||!p.ray.ray.rays||p.ray.ray.root.Get()!=p.pipelines->constants.NativeRoot()||
     p.dispatch.Width!=p.ray.width||p.dispatch.Height!=p.ray.height||p.dispatch.Depth!=1)return E_INVALIDARG;
  const auto peak=RequiredPeakBytes(p);
  if(!peak||!allocationReservation_.Peak(p.memoryBudget,peak))return E_OUTOFMEMORY;
  struct Settle {D3D12FrozenInvocation* owner;~Settle(){
   owner->allocationReservation_.Settle(owner->RetainedAllocationBytes());
  }} settle{this};
  // Keep a failed partial allocation private. Its owner may be destroyed since
  // Prepare never records commands. It cannot be accidentally prepared again.
  if(device_&&(device_.Get()!=p.device||pipelines_.Get()!=p.pipelines||
     !work_.Matches(p.ray.width,p.ray.height,p.ray.slots)))return E_INVALIDARG;
  const bool reused=rearmed_;rearmed_=false;device_=p.device;pipelines_=p.pipelines;
  if(!nativeHeaps_.Acquire(p.recording,p.ray.ray))return E_INVALIDARG;
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
  // Preserve source bytes before either half of the split estimator. Building
  // the private table changes ONLY local source addresses in validated even rows.
  // Allocation/CPU failures still happen before the first GPU command.
  const auto retainedUpload=tables_.HitUploadAllocationBytes();
  if(retainedUpload>p.budget-bytes_)return E_OUTOFMEMORY;
  hr=hitSources_.Prepare(p.device,*p.nativeBindings,*p.hitSources,p.budget-bytes_-retainedUpload);if(FAILED(hr))return hr;
  if(hitSources_.AllocationBytes()>p.budget-bytes_)return E_OUTOFMEMORY;
  bytes_+=hitSources_.AllocationBytes();
  try {
   const auto& original=*p.nativeHitTable;const auto& bindings=*p.nativeBindings;
   if(bindings.empty()||bindings.size()>65536||original.bytes.size()!=bindings.size()*192||
      original.active.size()!=bindings.size()*2||hitSources_.Bindings().size()!=bindings.size())return E_INVALIDARG;
   auto rebased=original;
   for(std::size_t i=0;i<bindings.size();++i){auto* row=rebased.bytes.data()+i*192;
    if(original.active[i*2]!=1||original.active[i*2+1]!=0||
       std::memcmp(row+32,reinterpret_cast<const unsigned char*>(&bindings[i])+8,56))return E_INVALIDARG;
    const auto& b=hitSources_.Bindings()[i];std::memcpy(row+32,&b.indices,8);std::memcpy(row+40,&b.attributes,8);
   }
   hr=tables_.ReplaceHitTable(p.device,rebased,reinterpret_cast<UINT64>(p.ray.ray.rays.Get()),p.writerVersion,p.budget-bytes_);
   if(FAILED(hr))return hr;
  }catch(...){return E_OUTOFMEMORY;}
  if(tables_.HitUploadAllocationBytes()>p.budget-bytes_)return E_OUTOFMEMORY;
  bytes_+=tables_.HitUploadAllocationBytes();
  if(!textures_.AcquireAtRay(p.nativeOwner,p.device,p.ray.width,p.ray.height))return E_INVALIDARG;
  if(!allocationReservation_.Settle(RetainedAllocationBytes()))return E_OUTOFMEMORY;
  originalProducer_=p.recording.list;producer_=p.ray;prepared_=true;return S_OK;
 }
 // Caller must first Take this WHOLE owner from the retirement pool. GPU
 // completion or the producer Reset alone is not sufficient. Changed extents,
 // native root/pipelines or device require a fresh owner after retirement.
 bool RearmAfterRetirement()noexcept {
  if(!prepared_||!producerRecorded_||rearmed_)return false;
  if(!hitSources_.RearmAfterRetirement()||!constants_.RearmAfterRetirement()||!tables_.RearmAfterRetirement()||!records_.RearmAfterRetirement())return false;
  originalProducer_=nullptr;nativeHeaps_={};textures_.ResetAfterRetirement();output_.Reset();savedRay_={};savedShade_={};producer_={};bytes_=0;
  prepared_=producerAttempted_=producerRecorded_=consumerAttempted_=false;rearmed_=true;return true;
 }
 bool ReusableFor(const Preparation& p)const noexcept {
  return rearmed_&&device_.Get()==p.device&&pipelines_.Get()==p.pipelines&&
   work_.Matches(p.ray.width,p.ray.height,p.ray.slots);
 }
 ID3D12CommandList* OriginalProducer()const noexcept{return originalProducer_;}
 ID3D12CommandQueue* NativeQueue()const noexcept{return nativeHeaps_.Queue();}
 UINT64 AllocationBytes()const noexcept{return prepared_?bytes_:0;}
 bool AcceptProducer(ID3D12GraphicsCommandList4* list,D3D12FrozenProducer::Trace trace)const noexcept {
  return prepared_&&!producerAttempted_&&list==originalProducer_&&hitSources_.CanRecord(list)&&nativeHeaps_.Validate(producer_.frame,nativeHeaps_.Queue())&&D3D12FrozenProducer::Accept(list,producer_,work_,constants_,tables_,records_,trace);
 }
 D3D12FrozenProducer::Result RecordProducer(ID3D12GraphicsCommandList4* list,D3D12FrozenProducer::Trace trace)noexcept {
  if(!AcceptProducer(list,trace))return D3D12FrozenProducer::Result::RejectedBeforeCommands;
  producerAttempted_=true;
  if(!hitSources_.Record(list))return D3D12FrozenProducer::Result::FailedAfterCommands;
  const auto result=D3D12FrozenProducer::Record(list,producer_,work_,constants_,tables_,records_,trace,savedRay_);
  producerRecorded_=result==D3D12FrozenProducer::Result::Recorded;
  return result==D3D12FrozenProducer::Result::RejectedBeforeCommands?
   D3D12FrozenProducer::Result::FailedAfterCommands:result;
 }
 struct Consumption {
  const void* nativeOwner=nullptr;
  const void* outputContainer=nullptr;
  bool producerSubmitted=false,sceneAndDescriptorLease=false;
  ID3D12CommandQueue* queue=nullptr;
  UINT frame=0,epoch=0;
  ComputeBindingSnapshot shade;
  GraphicsTableState graphics;
 };
 D3D12FrozenCompletion::Result RecordConsumer(ID3D12GraphicsCommandList4* list,const Consumption& p,
                                             D3D12FrozenCompletion::Trace trace)noexcept {
  if(!producerRecorded_||consumerAttempted_||!p.producerSubmitted||!p.sceneAndDescriptorLease||
     p.frame!=producer_.frame||p.epoch!=producer_.epoch||
     !nativeHeaps_.Validate(p.frame,p.queue))return D3D12FrozenCompletion::Result::RejectedBeforeCommands;
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
