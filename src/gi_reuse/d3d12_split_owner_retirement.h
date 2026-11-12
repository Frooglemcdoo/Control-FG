#pragma once
#include "split_owner_retirement.h"
#include <d3d12.h>
#include <wrl/client.h>
namespace gi_reuse {
// Single native queue, private completion fence, bounded shared invocation owners.
// Initialize outside recording; Before/After/Poll never allocate, wait or flush.
// Host serializes state calls, observes ALL Execute/Reset paths, and mutes its
// own injected fence operations. This is not a scene-content ownership proof.
template<class Owner,unsigned Capacity=8,UINT64 Budget=768ull*1024*1024>
class D3D12SplitOwnerRetirement {
 using Policy=GISplitOwnerRetirement<Owner,Capacity,Budget>;
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 struct State {Policy owners;Ptr<ID3D12CommandQueue> queue;Ptr<ID3D12Fence> fence;UINT64 issued=0;};
 std::unique_ptr<State> state_;
public:
 using Recording=typename Policy::Recording;
 using Batch=typename Policy::Batch;
 using Generation=UINT64 (*)(ID3D12CommandList*)noexcept;
 using Signal=HRESULT (*)(ID3D12CommandQueue*,ID3D12Fence*,UINT64);
 static HRESULT NativeSignal(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 n){return q->Signal(f,n);}
 ~D3D12SplitOwnerRetirement(){if(state_&&state_->owners.RetainedBytes())(void)state_.release();}
 HRESULT Initialize(ID3D12CommandQueue* queue)noexcept {
  if(state_||!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return E_INVALIDARG;
  std::unique_ptr<State> next(new(std::nothrow) State);if(!next)return E_OUTOFMEMORY;
  Ptr<ID3D12Device> device;auto hr=queue->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr))return hr;
  hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&next->fence));if(FAILED(hr))return hr;
  next->queue=queue;state_=std::move(next);return S_OK;
 }
 UINT64 Admit(UINT64 frame,Recording producer,UINT64 bytes,std::unique_ptr<Owner>& owner)noexcept {
  return state_?state_->owners.Admit(frame,producer,bytes,owner):0;
 }
 bool Attach(UINT64 id,UINT64 frame,Recording consumer)noexcept{return state_&&state_->owners.Attach(id,frame,consumer);}
 bool CanRecordConsumer(UINT64 id,UINT64 frame,ID3D12CommandQueue* queue,Recording consumer)noexcept {
  return state_&&queue==state_->queue.Get()&&
   state_->owners.CanRecordConsumer(id,frame,reinterpret_cast<UINT64>(queue),consumer);
 }
 bool ClaimConsumerRecording(UINT64 id,UINT64 frame,ID3D12CommandQueue* queue,Recording consumer)noexcept {
  return state_&&queue==state_->queue.Get()&&
   state_->owners.ClaimConsumerRecording(id,frame,reinterpret_cast<UINT64>(queue),consumer);
 }
 bool Seal(Recording r)noexcept{return state_&&state_->owners.Seal(r);}
 bool CancelUnattached(UINT64 id)noexcept{return state_&&state_->owners.CancelUnattached(id);}
 Batch Before(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists,Generation generation)noexcept {
  if(!state_||!count)return {};
  if(!queue||!lists){state_->owners.QuarantineAll();return {};}
  if(!generation||count>256){
   for(UINT i=0;i<count;++i)state_->owners.QuarantineList(reinterpret_cast<UINT64>(lists[i]));
   return {};
  }
  std::array<Recording,256> executed{};
  for(UINT i=0;i<count;++i){
   if(!lists[i]){state_->owners.QuarantineAll();return {};}
   executed[i].list=reinterpret_cast<UINT64>(lists[i]);
   executed[i].generation=generation(lists[i]);
   if(!executed[i].generation)state_->owners.QuarantineList(executed[i].list);
  }
  auto batch=state_->owners.Before(reinterpret_cast<UINT64>(queue),executed.data(),count);
  if(queue!=state_->queue.Get()){
   // Quarantine a real wrong-queue execution, not unrelated work. No signal
   // on the foreign queue and no inference that its work belongs to GI.
   if(batch.count)state_->owners.After(batch,reinterpret_cast<UINT64>(queue),0,false);
   return {};
  }
  return batch;
 }
 bool After(ID3D12CommandQueue* queue,Batch& batch,Signal signal=&NativeSignal)noexcept {
  if(!state_)return false;
  if(!batch.count)return false; // Unrelated native submissions need no signal.
  const auto value=state_->issued<UINT64_MAX-1?state_->issued+1:0;
  const bool ok=queue==state_->queue.Get()&&value&&signal&&SUCCEEDED(signal(queue,state_->fence.Get(),value));
  if(ok)state_->issued=value;
  return state_->owners.After(batch,reinterpret_cast<UINT64>(queue),value,ok);
 }
 void Poll()noexcept {
  if(state_)state_->owners.Complete(reinterpret_cast<UINT64>(state_->queue.Get()),state_->fence->GetCompletedValue());
 }
 Owner* View(UINT64 id)noexcept{return state_?state_->owners.View(id):nullptr;}
 bool Take(UINT64 id,std::unique_ptr<Owner>& owner)noexcept{return state_&&state_->owners.Take(id,owner);}
 UINT64 RetainedBytes()const noexcept{return state_?state_->owners.RetainedBytes():0;}
 void DeviceStopped()noexcept{if(state_){state_->owners.DeviceStopped();state_.reset();}}
};
}
