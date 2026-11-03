#pragma once
#include "gi_native_scene_owners.h"
#include <deque>
namespace GINativeSceneQueues {
using Microsoft::WRL::ComPtr;
using namespace GINativeSceneOwners;
static std::recursive_mutex mutex;
static gi_reuse::GILiveSceneCatalog catalog;
struct Queue {
 ComPtr<IUnknown> identity;ComPtr<ID3D12CommandQueue> native;ComPtr<ID3D12Fence> completed;
 UINT64 id=0,next=0,completedStamp=0;
};
struct Fence {ComPtr<IUnknown> identity;UINT64 id=0;};
struct Flight {
 UINT64 queue=0,value=0,stamp=0;bool signaled=false;
 std::vector<ComPtr<Recording>> recordings;
 // Future renderer admission leases belong here, before native Execute.
};
static std::vector<Queue> queues;
static std::vector<Fence> fences;
static std::deque<Flight> flights;
static UINT64 serial=0;
static std::atomic<UINT64> submits{0},marks{0},checkpoints{0};
static bool Active() noexcept {return Enabled()&&ready.load();}
static Queue* FindQueue(ID3D12CommandQueue* input){
 if(!input)return nullptr;ComPtr<IUnknown> identity;if(FAILED(input->QueryInterface(IID_PPV_ARGS(&identity))))return nullptr;
 for(auto& q:queues)if(q.identity.Get()==identity.Get())return &q;
 if(queues.size()>=16)return nullptr;Queue next;next.identity=identity;next.native=input;next.id=++serial;
 ComPtr<ID3D12Device> device;
 if(FAILED(input->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&next.completed))))return nullptr;
 queues.push_back(std::move(next));return &queues.back();
}
static UINT64 FenceID(ID3D12Fence* input){
 if(!input)return 0;ComPtr<IUnknown> identity;if(FAILED(input->QueryInterface(IID_PPV_ARGS(&identity))))return 0;
 for(const auto& f:fences)if(f.identity.Get()==identity.Get())return f.id;
 if(fences.size()>=256)return 0;const auto id=++serial;fences.push_back({identity,id});return id;
}
static void Poll(){
 for(auto it=flights.begin();it!=flights.end();){
  auto q=std::find_if(queues.begin(),queues.end(),[&](const Queue& entry){return entry.id==it->queue;});
  const auto done=q==queues.end()?0:q->completed->GetCompletedValue();
  if(it->signaled&&done!=UINT64_MAX&&done>=it->value){q->completedStamp=(std::max)(q->completedStamp,it->stamp);it=flights.erase(it);}else ++it;
 }
 // Rebase only on already-completed queue tails; no frame-time CPU/GPU wait.
 if(flights.empty()&&catalog.Healthy()){
  std::map<std::uint64_t,std::uint64_t> completed;
  for(const auto& q:queues)completed.emplace(q.id,q.completedStamp);
  if(catalog.Checkpoint(completed,false))++checkpoints;
 }
}
static std::shared_ptr<const gi_reuse::GISceneBuild> Payload(const Event& input){
 auto output=std::make_shared<gi_reuse::GISceneBuild>();output->build=input.build;output->geometry=input.geometry;
 if(input.instances){output->instances={input.instances->address,input.instances->token,1,
   std::shared_ptr<const std::vector<unsigned char>>(input.instances,&input.instances->bytes)};
  output->bottoms.reserve(input.bottoms.size());for(const auto& ref:input.bottoms)output->bottoms.push_back({ref.address,ref.token,0});}
 return output;
}
// Paired callbacks bracket the exact native call, including independent queues.
// All failure paths preserve forwarding. The recursive lock also covers nested
// observer signals; those calls are suppressed by the native hook's depth guard.
static UINT64 Before(ID3D12CommandQueue* input,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Active())return 0;mutex.lock();const DWORD error=GetLastError();
 try {
  Poll();auto* q=FindQueue(input);
  if(!q||!catalog.Healthy()||count>256||(count&&!lists)||flights.size()>=256){catalog.Invalidate("live_submission_preflight");SetLastError(error);return 1;}
  Flight flight;flight.queue=q->id;flight.value=++q->next;
  if(!flight.value||flight.value==UINT64_MAX){catalog.Invalidate("live_fence_overflow");SetLastError(error);return 1;}
  if(input->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_COPY){
   std::set<UINT64> unique;
   for(UINT i=0;i<count;++i){auto recording=ReadRecording(lists[i],false);
    if(!recording||!recording->complete||recording->submitted||!unique.insert(recording->generation).second){catalog.Invalidate("live_missing_or_replayed_recording");break;}
    flight.recordings.push_back(std::move(recording));
   }
   if(catalog.Healthy())for(const auto& recording:flight.recordings){recording->submitted=true;
    for(const auto& event:recording->events){
     if(event.writer){if(!catalog.Mark(q->id))break;++marks;}
     else if(!catalog.Submit(q->id,recording->generation,Payload(event)))break;
    }
   }
  }
  // Even an empty submission has a tail marker, so completion never authorizes
  // history retirement past an unobserved native queue operation.
  if(catalog.Healthy())catalog.Mark(q->id);
  flight.stamp=catalog.TailStamp(q->id);flights.push_back(std::move(flight));++submits;
 }catch(...){catalog.Invalidate("live_submission_exception");}
 SetLastError(error);return 1;
}
static void After(ID3D12CommandQueue* input,UINT64 locked) noexcept {
 if(!locked)return;const DWORD error=GetLastError();
 try {auto* q=FindQueue(input);
  if(q&&!flights.empty()&&flights.back().queue==q->id&&!flights.back().signaled){
   auto& flight=flights.back();if(SUCCEEDED(input->Signal(q->completed.Get(),flight.value)))flight.signaled=true;
   else catalog.Invalidate("live_completion_signal_failed");
  }
 }catch(...){catalog.Invalidate("live_completion_exception");}
 SetLastError(error);mutex.unlock();
}
static bool BeforeFence() noexcept {if(!Active())return false;mutex.lock();return true;}
static void AfterFence(ID3D12CommandQueue* input,ID3D12Fence* fence,UINT64 value,bool signal,HRESULT result,bool locked) noexcept {
 if(!locked)return;const DWORD error=GetLastError();
 try {auto* q=FindQueue(input);const auto id=FenceID(fence);
  if(!q||!id)catalog.Invalidate("live_fence_identity");
  else if(signal)catalog.Signal(q->id,id,value,SUCCEEDED(result));else catalog.Wait(q->id,id,value,SUCCEEDED(result));
 }catch(...){catalog.Invalidate("live_fence_exception");}
 SetLastError(error);mutex.unlock();
}
static void CpuSignal(ID3D12Fence* input) noexcept {
 if(!Active())return;const DWORD error=GetLastError();std::lock_guard<std::recursive_mutex> guard(mutex);
 ComPtr<IUnknown> identity;
 if(input&&SUCCEEDED(input->QueryInterface(IID_PPV_ARGS(&identity))))for(const auto& f:fences)if(f.identity.Get()==identity.Get()){catalog.Invalidate("live_cpu_signal_on_observed_fence");break;}
 SetLastError(error);
}
}
