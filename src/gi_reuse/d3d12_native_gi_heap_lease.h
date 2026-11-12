#pragma once
#include "d3d12_native_gi_recording.h"
#include "d3d12_compute_bindings.h"
namespace gi_reuse {
// Retains the ORIGINAL recording's queue, native fence and actual bound heaps.
// Validate reads saved native records, never the consumer/submit thread's TLS.
// This is a descriptor-storage lease only: native scene writer exclusion and
// submission before the native frame retirement cut remain host obligations.
class D3D12NativeGIHeapLease {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 GIRecordedHeapIdentity identity_;
 Ptr<ID3D12Device> device_;
 Ptr<ID3D12CommandQueue> queue_;
 Ptr<ID3D12Fence> fence_;
 std::array<Ptr<ID3D12DescriptorHeap>,2> heaps_;
 template<class T>static bool SameDevice(T* child,ID3D12Device* expected)noexcept {
  Ptr<ID3D12Device> actual;
  return child&&SUCCEEDED(child->GetDevice(IID_PPV_ARGS(&actual)))&&actual.Get()==expected;
 }
public:
 bool Acquire(const NativeGIRecording& source,const ComputeBindingSnapshot& bound)noexcept {
  if(identity_.Saved()||!source.device||!source.queue||!source.list||!bound.Ready())return false;
  auto* fence=reinterpret_cast<ID3D12Fence*>(source.heaps.deviceFenceObject);
  if(!SameDevice(source.queue,source.device)||!SameDevice(fence,source.device)||
     source.queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
  std::array<UINT64,2> objects{};
  for(unsigned i=0;i<2;++i){
   if(!SameDevice(bound.heaps[i].Get(),source.device))return false;
   const auto desc=bound.heaps[i]->GetDesc();
   if(desc.Type!=(i?D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)||
      !(desc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))return false;
   objects[i]=reinterpret_cast<UINT64>(bound.heaps[i].Get());
  }
  // Never trust a fabricated/default completed=0 from a borrowed parser result.
  auto sample=source.heaps;sample.completed=fence->GetCompletedValue();
  GIRecordedHeapIdentity candidate;
  if(!candidate.Capture(sample,objects))return false;
  GIRecordedHeapIdentity::Sample reread;
  if(!ReadNativeGIRecordedHeaps(sample,sample.frame,fence->GetCompletedValue(),&reread)||
     !candidate.ValidateSaved(reread,objects))return false;
  // Publish only after all identity, epoch, device and completion checks pass.
  device_=source.device;queue_=source.queue;fence_=fence;heaps_=bound.heaps;identity_=candidate;
  return true;
 }
 bool Validate(UINT64 frame,ID3D12CommandQueue* queue)const noexcept {
  const auto* saved=identity_.Saved();if(!saved||queue!=queue_.Get())return false;
  GIRecordedHeapIdentity::Sample now;
  const std::array<UINT64,2> objects{reinterpret_cast<UINT64>(heaps_[0].Get()),reinterpret_cast<UINT64>(heaps_[1].Get())};
  return ReadNativeGIRecordedHeaps(*saved,frame,fence_->GetCompletedValue(),&now)&&identity_.ValidateSaved(now,objects);
 }
 ID3D12CommandQueue* Queue()const noexcept{return queue_.Get();}
 ID3D12Device* Device()const noexcept{return device_.Get();}
 UINT64 Epoch()const noexcept{return identity_.Saved()?identity_.Saved()->deviceEpoch:0;}
};
}
