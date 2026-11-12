#pragma once
#include <atomic>
#include <d3d12.h>
// Independent from NRD and diagnostic callbacks. Publish a complete callback
// set at startup and retain it for the process lifetime. This observes Execute;
// it does not establish GPU completion or manufacture a scene lease.
namespace gi_frozen_submission {
struct Callbacks {
 UINT64 (*before)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*) noexcept;
 // submitted=false requires quarantine/cancellation analysis, never retirement.
 void (*after)(ID3D12CommandQueue*,UINT64,bool submitted) noexcept;
};
inline std::atomic<const Callbacks*> callbacks{nullptr};
class Call {
 const Callbacks* observer_=nullptr;
 ID3D12CommandQueue* queue_=nullptr;
 UINT64 token_=0;
 bool submitted_=false;
public:
 Call(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists)noexcept:
  observer_(callbacks.load(std::memory_order_acquire)),queue_(queue){
  if(observer_&&observer_->before&&observer_->after)token_=observer_->before(queue,count,lists);
  else observer_=nullptr;
 }
 Call(const Call&)=delete;Call& operator=(const Call&)=delete;
 ~Call(){Finish();}
 void Submitted()noexcept{submitted_=true;}
 void Finish()noexcept{
  // Capture the same callback set across the native call. Do not reload a new
  // observer for somebody else's token, including teardown/abnormal unwind.
  const auto observer=observer_;observer_=nullptr;
  if(observer)observer->after(queue_,token_,submitted_);
 }
};
}
