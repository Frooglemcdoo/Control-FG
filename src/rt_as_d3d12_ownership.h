#pragma once
#include "rt_as_ownership.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <functional>

namespace rt_as_ownership {
// D3D12 adapter for retention only. It neither builds ASes nor enables a game
// replacement. Caller must provide complete binding/producer coverage separately.
class D3D12Ownership {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 struct Resources {std::vector<Ptr<ID3D12Resource>> values;};
 struct Queue {
  std::mutex mutex;Ptr<IUnknown> identity;Ptr<ID3D12CommandQueue> native;Ptr<ID3D12Fence> fence;
  Id id=0,reserved=0,completed=0;
 };
 struct State {Ledger ledger;std::mutex mutex;std::map<IUnknown*,std::shared_ptr<Queue>> queues;Id nextQueue=1;};
 std::unique_ptr<State> state_{new State};
 std::shared_ptr<Queue> GetQueue(ID3D12CommandQueue* native){
  if(!native)throw std::invalid_argument("null queue");
  Ptr<IUnknown> identity;if(FAILED(native->QueryInterface(IID_PPV_ARGS(&identity))))throw std::runtime_error("queue identity");
  std::lock_guard<std::mutex> guard(state_->mutex);
  auto found=state_->queues.find(identity.Get());if(found!=state_->queues.end())return found->second;
  if(state_->queues.size()>=16)throw std::runtime_error("ownership queue capacity");
  Ptr<ID3D12Device> device;if(FAILED(native->GetDevice(IID_PPV_ARGS(&device))))throw std::runtime_error("queue device");
  auto q=std::make_shared<Queue>();q->identity=identity;q->native=native;q->id=state_->nextQueue++;
  if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&q->fence))))throw std::runtime_error("ownership fence");
  state_->queues.emplace(identity.Get(),q);return q;
 }
public:
 using Signal=HRESULT(*)(ID3D12CommandQueue*,ID3D12Fence*,UINT64);
 static HRESULT NativeSignal(ID3D12CommandQueue* q,ID3D12Fence* fence,UINT64 value){return q->Signal(fence,value);}
 ~D3D12Ownership(){
  // Failed signals/device removal provide no release proof. Preserve the ledger,
  // COM owners and timelines through process teardown instead of freeing in-use
  // resources. New submissions stop at the capacity limit below.
  if(state_&&state_->ledger.Pending())(void)state_.release();
 }
 Key Register(ID3D12Resource* native,ID3D12Resource* replacement,
              const rt_as_build_route::Coverage& coverage,const std::vector<ID3D12Resource*>& dependencies={}){
  if(!native||native->GetDesc().Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)throw std::invalid_argument("native AS allocation");
  if(replacement&&replacement->GetDesc().Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)throw std::invalid_argument("replacement AS allocation");
  auto refs=std::make_shared<Resources>();refs->values.emplace_back(native);
  if(replacement)refs->values.emplace_back(replacement);
  for(auto* resource:dependencies){if(!resource)throw std::invalid_argument("missing AS dependency");refs->values.emplace_back(resource);}
  return state_->ledger.Register(native->GetGPUVirtualAddress(),native->GetDesc().Width,refs,replacement!=nullptr,
   coverage,replacement?replacement->GetGPUVirtualAddress():0);
 }
 Ledger& Contracts(){return state_->ledger;}
 // All reference acquisition and bookkeeping allocations occur before Execute.
 // A per-queue mutex protects our private fence ordering; no global queue lock
 // spans Execute. This proves lifetime, not dependency ordering between queues.
 void Execute(ID3D12CommandQueue* queue,const std::vector<Id>& recordings,const std::function<void()>& nativeExecute,Signal signal=&NativeSignal){
  if(!nativeExecute||!signal)throw std::invalid_argument("submission callback");
  auto q=GetQueue(queue);std::lock_guard<std::mutex> guard(q->mutex);
  if(state_->ledger.Pending()>=128||q->reserved>=UINT64_MAX-1)throw std::runtime_error("ownership submission capacity");
  const auto value=++q->reserved;const auto ticket=state_->ledger.PrepareSubmit(recordings,q->id,q->id,value);
  state_->ledger.Executing(ticket);
  try{nativeExecute();}catch(...){state_->ledger.Quarantine(ticket);throw;}
  if(FAILED(signal(queue,q->fence.Get(),value))){state_->ledger.Quarantine(ticket);throw std::runtime_error("ownership signal failed; resources quarantined");}
  state_->ledger.Submitted(ticket);
 }
 void Poll(){
  std::vector<std::shared_ptr<Queue>> queues;
  {std::lock_guard<std::mutex> guard(state_->mutex);for(const auto& item:state_->queues)queues.push_back(item.second);}
  for(const auto& q:queues){std::lock_guard<std::mutex> guard(q->mutex);const auto value=q->fence->GetCompletedValue();
   if(value==UINT64_MAX)continue; // Device removal is not successful completion.
   if(value>q->completed){state_->ledger.Complete(q->id,q->id,value);q->completed=value;}
  }
 }
};
}
