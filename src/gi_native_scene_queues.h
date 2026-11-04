#pragma once
#include "gi_native_scene_owners.h"
#include "gi_native_submission_hooks.h"
#include "gi_owned_pipeline_runtime.h"
#include <deque>
namespace native_lighting {namespace rt_spot_table_audit {inline bool InstallGameQueue(ID3D12CommandQueue*) noexcept;}}
namespace GINativeSceneQueues {
using Microsoft::WRL::ComPtr;
using namespace GINativeSceneOwners;
static std::recursive_mutex mutex;
static gi_reuse::GILiveSceneCatalog catalog;
struct Queue {
 ComPtr<IUnknown> identity;ComPtr<ID3D12CommandQueue> native;ComPtr<ID3D12Fence> completed;
 UINT64 id=0,next=0,completedStamp=0;
};
struct Fence {ComPtr<IUnknown> identity;ComPtr<ID3D12Fence> native;UINT64 id=0,pendingValue=0,observedSignal=0,lastCompleted=0;bool hasCompletion=false;};
struct Flight {
 UINT64 queue=0,value=0,stamp=0;bool signaled=false;
 std::vector<ComPtr<Recording>> recordings;
 ComPtr<ID3D12CommandAllocator> prefixAllocator;
 ComPtr<ID3D12GraphicsCommandList4> prefix;
 ComPtr<ID3D12Fence> admissionHold;
 std::vector<std::pair<std::shared_ptr<Use>,UINT64>> deferred;
 UINT64 holdStarted=0;
 bool holdReleased=false,holdResolved=false;
};
static std::vector<Queue> queues;
static std::vector<ComPtr<IUnknown>> gameQueues;
static std::vector<Fence> fences;
static std::deque<Flight> flights;
static UINT64 serial=0;
static std::atomic<UINT64> submits{0},marks{0},checkpoints{0},compactions{0};
static bool Active() noexcept {return Enabled()&&ready.load();}
static bool IsGameQueue(ID3D12CommandQueue* queue) noexcept {
 ComPtr<IUnknown> identity;if(!queue||FAILED(queue->QueryInterface(IID_PPV_ARGS(&identity))))return false;
 for(const auto& entry:gameQueues)if(entry.Get()==identity.Get())return true;
 return false;
}
static void RegisterGameQueue(ID3D12CommandQueue* queue) noexcept {
 if(!Enabled()||!queue)return;const DWORD error=GetLastError();
 try {std::lock_guard<std::recursive_mutex> guard(mutex);
  [&] {
  ComPtr<IUnknown> identity;if(FAILED(queue->QueryInterface(IID_PPV_ARGS(&identity)))){catalog.Invalidate("game_queue_identity");return;}
  if(IsGameQueue(queue))return;
  if(gameQueues.size()>=16){catalog.Invalidate("game_queue_capacity");return;}
  // Hook the exact interface returned to Control, before its first submission.
  // The existing submitDepth guard suppresses synchronous native forwarding.
  if(!native_lighting::rt_spot_table_audit::InstallGameQueue(queue)){catalog.Invalidate("game_queue_hook");return;}
  gameQueues.push_back(std::move(identity));
  Log("GI_OWNED_GAME_QUEUE revision=R317 registered=1 queue=%p type=%u count=%llu",static_cast<void*>(queue),unsigned(queue->GetDesc().Type),UINT64(gameQueues.size()));
  }();
 }catch(...){catalog.Invalidate("game_queue_registration");}
 SetLastError(error);
}

struct TraceEvent {UINT64 sequence=0,tick=0,queue=0,fence=0,value=0,completed=0;DWORD thread=0;HRESULT result=S_OK;const char* operation="none";};
static std::array<TraceEvent,256> trace;
static UINT64 traceSerial=0;
static void Trace(const char* operation,UINT64 queue,UINT64 fence,UINT64 value,UINT64 completed=0,HRESULT result=S_OK){
 LARGE_INTEGER qpc{};QueryPerformanceCounter(&qpc);const auto n=++traceSerial;
 trace[n%trace.size()]={n,UINT64(qpc.QuadPart),queue,fence,value,completed,GetCurrentThreadId(),result,operation};
}
static void ExplainWait(UINT64 cut,UINT64 writer){
 const auto pending=catalog.PendingWait(cut);static UINT64 reports=0;const auto n=++reports;
 if(n>8&&n%120)return;
 UINT64 actual=0;ID3D12Fence* native=nullptr;
 for(const auto& f:fences)if(f.id==pending.fence){native=f.native.Get();actual=native->GetCompletedValue();break;}
 Log("GI_OWNED_WAIT_DETAIL revision=R318 writer=%llu cut=%llu fence=%llu native_fence=%p wait_value=%llu observed_signal=%llu graph_completed=%llu actual_completed=%llu lane=%llu stamp=%llu count=%llu",
  writer,cut,pending.fence,static_cast<void*>(native),pending.value,pending.signalValue,pending.completed,actual,pending.lane,pending.stamp,n);
 if(n>2)return;
 const auto first=traceSerial>=trace.size()?traceSerial-trace.size()+1:1;
 for(UINT64 i=first;i<=traceSerial;++i){const auto& e=trace[i%trace.size()];
  Log("GI_OWNED_QUEUE_EVENT sequence=%llu event_qpc=%llu event_tid=%lu op=%s queue=%llu fence=%llu value=%llu completed=%llu hr=%08x",e.sequence,e.tick,e.thread,e.operation,e.queue,e.fence,e.value,e.completed,unsigned(e.result));
 }
}

// Nonblocking completion observations resolve only waits whose queue signal
// was not observed. The graph retains all known producer/consumer edges.
static void PollNativeFences(){
 for(auto& fence:fences){if(!fence.pendingValue)continue;const auto completed=fence.native->GetCompletedValue();
  if(completed==UINT64_MAX){catalog.Invalidate("native_fence_device_removed");continue;}
  if(fence.hasCompletion&&completed==fence.lastCompleted)continue;
  fence.lastCompleted=completed;fence.hasCompletion=true;
  const auto resolved=catalog.ObserveFenceCompletion(fence.id,completed);
  if(completed>=fence.pendingValue)fence.pendingValue=0;
  if(resolved){static UINT64 reports=0;const auto n=++reports;
   if(n<=8||n%120==0)Log("GI_OWNED_FENCE_COMPLETION fence=%llu completed=%llu resolved_waits=%llu blocking_wait=0 count=%llu",fence.id,completed,resolved,n);
  }
 }
}
// A recording-time rejection avoids scratch/SBT allocation and tiled command
// recording. It is only a preflight: actual submission must still revalidate.
static bool Preflight(ID3D12CommandQueue* input,const Recording& recording,const Writer& writer) noexcept {
 try {
  // Unsubmitted AS writes can make this list's scene newer than the catalog.
  // Defer these uncommon cases to the original exact submission check.
  std::lock_guard<std::recursive_mutex> guard(mutex);
  // Fatal catalog failure cannot be repaired by a future submission. Reject
  // before the pending-write/producer shortcuts allocate and record GPU work.
  if(!catalog.Healthy()){
   GIOwnedPipeline::active=false;
   static UINT64 blocked=0;if(++blocked<=4||blocked%600==0)
    Log("GI_OWNED_PREFLIGHT revision=R323 rejected=catalog_unhealthy reason=%s writer=%llu scratch_allocated=0 count=%llu",catalog.History().FailureReason(),writer.ticket.serial,blocked);
   return false;
  }
  ComPtr<IUnknown> identity;if(!input||FAILED(input->QueryInterface(IID_PPV_ARGS(&identity))))return false;
  const auto queue=std::find_if(queues.begin(),queues.end(),[&](const Queue& q){return q.identity.Get()==identity.Get();});
  if(queue==queues.end()){static unsigned n=0;if(++n<=4)Log("GI_OWNED_PREFLIGHT_PATH revision=R318 path=unknown_queue queue=%p",static_cast<void*>(input));return true;}
  PollNativeFences();
  if(!catalog.Healthy()){GIOwnedPipeline::active=false;return false;}
  const auto readiness=catalog.Mark(queue->id);
  const auto pending=catalog.PendingWait(readiness);
  {static unsigned n=0;if(++n<=8)Log("GI_OWNED_PREFLIGHT_PATH revision=R318 queue=%llu writer=%llu producer_ordered=%u events=%llu pending_fence=%llu",queue->id,writer.ticket.serial,unsigned(writer.submission.Precedes(queue->id)),UINT64(recording.events.size()),pending.fence);}
  if(pending.fence){static std::atomic<UINT64> deferredWaits{0};const auto n=++deferredWaits;
   if(n<=8||n%120==0)Log("GI_OWNED_PREFLIGHT revision=R318 deferred=queue_wait fence=%llu value=%llu signal_value=%llu completed=%llu writer=%llu admission_required=1 count=%llu",pending.fence,pending.value,pending.signalValue,pending.completed,writer.ticket.serial,n);
   // Recording precedes submission: a real producer may signal this fence
   // before Execute. Keep the recorded pass disarmed; Pass::Admit must prove
   // the exact scene at the actual submission cut before committing controls.
   return true;
  }
  // New AS writes also require validation at their actual submission cut.
  for(const auto& event:recording.events)if(!event.writer&&!event.use)return true;
  // The producer may be queued with the consumer in a later Execute batch.
  if(!writer.submission.Precedes(queue->id))return true;
  gi_reuse::GILiveSceneCatalog::Selection selected;gi_reuse::GILiveSceneCatalog::Diagnostic d;
  const auto cut=catalog.Mark(queue->id);
  bool valid=cut&&catalog.Select(cut,recording.generation,writer.scene.token,writer.scene.address,selected,&d);
  if(valid)for(const auto& geometry:selected.geometry)if(!geometry.frozen||!geometry.frozen->owner){d.stage="unpreserved_geometry";valid=false;break;}
  if(!valid){static std::atomic<UINT64> rejected{0};const auto n=++rejected;
   if(n<=8||n%120==0)Log("GI_OWNED_PREFLIGHT rejected=%s order=%s status=%u token=%llu address=%llx instance=%u missing_instances=%u writer=%llu native_owners_created=%llu native_owners_rejected=%llu retained_bytes=%llu scratch_allocated=0 count=%llu",
    cut?d.stage:"catalog_unhealthy",d.orderReason,unsigned(d.status),d.token,d.address,d.instance,d.missingBottoms,writer.ticket.serial,
    created.load(),GINativeSceneOwners::rejected.load(),owners.RetainedBytes(),n);
  }
  return valid;
 }catch(...){return false;}
}
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
 if(fences.size()>=256)return 0;const auto id=++serial;fences.push_back({identity,input,id});return id;
}
// Called under the observer mutex. Every control/SBT edit below is behind a
// private GPU wait inserted before BOTH the prefix and the original lists.
// Native queue signals remain untouched. Never edit controls after releasing it.
static void ResolveDeferred() noexcept {
 for(auto& flight:flights){if(!flight.admissionHold||flight.holdReleased)continue;
  if(!flight.holdResolved){
   try {
   bool waiting=false;
   if(catalog.Healthy()&&GetTickCount64()-flight.holdStarted<250)
    for(const auto& item:flight.deferred)if(catalog.PendingWait(item.second).fence){waiting=true;break;}
   if(waiting)continue;
   for(const auto& item:flight.deferred){
    if(!catalog.Healthy()||catalog.PendingWait(item.second).fence||!item.first->Admit(catalog,item.second))item.first->Cancel();
   }
   }catch(...){
    catalog.Invalidate("deferred_admission_exception");
    for(const auto& item:flight.deferred)item.first->Cancel();
   }
   flight.holdResolved=true;
  }
  const auto hr=flight.admissionHold->Signal(1);
  if(SUCCEEDED(hr)){
   flight.holdReleased=true;flight.deferred.clear();
   static UINT64 reports=0;if(++reports<=8||reports%120==0)
    Log("GI_OWNED_DEFERRED revision=R319 released=1 queue=%llu elapsed_ms=%llu admitted_total=%llu count=%llu",flight.queue,GetTickCount64()-flight.holdStarted,GIOwnedPipeline::admitted.load(),reports);
  }else {catalog.Invalidate("admission_hold_release_failed");for(const auto& item:flight.deferred)item.first->Cancel();}
 }
}
static bool HasDeferred() noexcept {
 for(const auto& flight:flights)if(flight.admissionHold&&!flight.holdReleased)return true;
 return false;
}
static void Poll(){
 PollNativeFences();ResolveDeferred();
 for(auto it=flights.begin();it!=flights.end();){
  auto q=std::find_if(queues.begin(),queues.end(),[&](const Queue& entry){return entry.id==it->queue;});
  const auto done=q==queues.end()?0:q->completed->GetCompletedValue();
  if(it->signaled&&done!=UINT64_MAX&&done>=it->value){q->completedStamp=(std::max)(q->completedStamp,it->stamp);it=flights.erase(it);}else ++it;
 }
 // Compact between admission cuts, using completed stamps only for retirement.
 // GPU work may remain in flight: its flights own independent resource leases.
 if(catalog.Healthy()&&!HasDeferred()&&submits.load()%32==0){
  std::map<std::uint64_t,std::uint64_t> completed;
  for(const auto& q:queues)completed.emplace(q.id,q.completedStamp);
  if(flights.empty()&&catalog.Checkpoint(completed,false))++checkpoints;
  else if(catalog.Compact(completed))++compactions;
 }
}
static std::shared_ptr<const gi_reuse::GISceneBuild> Payload(const Event& input){
 auto output=std::make_shared<gi_reuse::GISceneBuild>();output->build=input.build;output->geometry=input.geometry;output->frozen=input.frozen;
 if(input.instances){output->instances={input.instances->address,input.instances->token,1,
   std::shared_ptr<const std::vector<unsigned char>>(input.instances,&input.instances->bytes)};
  output->bottoms.reserve(input.bottoms.size());for(const auto& ref:input.bottoms)output->bottoms.push_back({ref.address,ref.token,0});}
 return output;
}
// Paired callbacks bracket the exact native call, including independent queues.
// All failure paths preserve forwarding. The recursive lock also covers nested
// observer signals; those calls are suppressed by the native hook's depth guard.
static void Quarantine(UINT count,ID3D12CommandList*const* lists) noexcept {
 // Submission must still forward. If a fence/flight cannot be established,
 // deliberately retain these owners until process exit rather than freeing
 // storage already referenced by recorded GPU copies on the next Reset.
 if(!lists)return;for(UINT i=0;i<count;++i){auto r=ReadRecording(lists[i],false);if(r)(void)r.Detach();}
}
static UINT64 Before(ID3D12CommandQueue* input,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Active())return 0;GIOwnedMetrics::Scope timing(GIOwnedMetrics::Submission);mutex.lock();const DWORD error=GetLastError();
 if(!IsGameQueue(input)){
  // Internal middleware/transport queues do not define the game's AS order.
  // Never silently ignore a scene write or owned use on an unregistered queue.
  bool scene=false;if(lists)for(UINT i=0;i<count;++i){auto r=ReadRecording(lists[i],false);if(r&&!r->events.empty()){scene=true;break;}}
  if(scene){catalog.Invalidate("scene_on_unregistered_queue");Quarantine(count,lists);GIOwnedPipeline::active=false;
   Log("GI_OWNED_QUEUE_UNREGISTERED revision=R317 scene_events=1 queue=%p count=%u",static_cast<void*>(input),count);}
  SetLastError(error);mutex.unlock();return 0;
 }
 std::vector<std::pair<std::shared_ptr<Use>,UINT64>> pending;
 try {
  Poll();auto* q=FindQueue(input);
  if(!q||count>256||(count&&!lists)||flights.size()>=256){catalog.Invalidate("live_submission_preflight");Quarantine(count,lists);SetLastError(error);return 1;}
  Trace("execute",q->id,0,count);
  Flight flight;flight.queue=q->id;flight.value=++q->next;
  if(!flight.value||flight.value==UINT64_MAX){catalog.Invalidate("live_fence_overflow");Quarantine(count,lists);SetLastError(error);return 1;}
  if(input->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_COPY){
   std::set<UINT64> unique;flight.recordings.reserve(count);
   for(UINT i=0;i<count;++i){auto recording=ReadRecording(lists[i],false);
    if(!recording){catalog.Invalidate("live_missing_recording");continue;}
    const bool duplicate=!unique.insert(recording->generation).second;
    // A complete recording without scene events changes no tracked AS version
    // or owned controls. Its replay is safe; AS/producer/use replay stays refused.
    const bool replay=(recording->submitted||duplicate)&&!recording->events.empty();
    if(!recording->complete||replay){
     const char* reason=!recording->complete?recording->incompleteReason:duplicate?"duplicate_scene_recording":"replayed_scene_recording";
     if(catalog.Healthy())Log("GI_OWNED_RECORDING_FAILURE revision=R323 reason=%s queue=%llu list=%p generation=%llu complete=%u submitted=%u duplicate=%u events=%llu",reason,q->id,static_cast<void*>(lists[i]),recording->generation,unsigned(recording->complete),unsigned(recording->submitted),unsigned(duplicate),UINT64(recording->events.size()));
     catalog.Invalidate(reason);
    }
    // Retain every recording even if an earlier list invalidated admission.
    // GPU lifetime is independent of whether replacement lighting is allowed.
    flight.recordings.push_back(std::move(recording));
   }
   for(const auto& recording:flight.recordings){recording->submitted=true;
    if(catalog.Healthy())for(const auto& event:recording->events){
     if(event.use){
      // Evaluate at this exact list/event position, before later producers in
      // the same Execute batch can publish their receipt. No CPU-time guess.
      if(!event.use->ProducerOrdered(q->id)){event.use->Cancel();continue;}
      const auto cut=catalog.Mark(q->id);if(!cut)break;pending.emplace_back(event.use,cut);
     }
     else if(event.writer){
      if(!event.writer->submission.Submit(q->id,recording->generation)){catalog.Invalidate("writer_submission_identity");break;}
      if(!catalog.Mark(q->id))break;++marks;
     }
     else if(!catalog.Submit(q->id,recording->generation,Payload(event)))break;
    }
   }
  }
  // Create all prefix owners before arming any mapped controls. The prefix
  // initializes each gate once; the recorded pass clears it, including replays.
  bool prefixReady=false,defer=false;
  if(catalog.Healthy()&&!pending.empty()){
   ComPtr<ID3D12Device> device;const auto type=input->GetDesc().Type;
   prefixReady=SUCCEEDED(input->GetDevice(IID_PPV_ARGS(&device)))&&
    SUCCEEDED(device->CreateCommandAllocator(type,IID_PPV_ARGS(&flight.prefixAllocator)))&&
    SUCCEEDED(device->CreateCommandList(0,type,flight.prefixAllocator.Get(),nullptr,IID_PPV_ARGS(&flight.prefix)));
   if(prefixReady){
    for(const auto& item:pending)if(catalog.PendingWait(item.second).fence){defer=true;break;}
    if(defer){
     unsigned held=0;for(const auto& f:flights)if(f.admissionHold&&!f.holdReleased)++held;
     if(held>=8||FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&flight.admissionHold))))prefixReady=false;
     else {flight.deferred=pending;flight.holdStarted=GetTickCount64();}
    }
    for(const auto& item:pending){if(!defer&&!item.first->Admit(catalog,item.second))item.first->Cancel();
     if(!item.first->RecordGate(flight.prefix.Get())){prefixReady=false;break;}}
    if(FAILED(flight.prefix->Close()))prefixReady=false;
   }
  }
  if(!prefixReady){for(const auto& item:pending)item.first->Cancel();flight.admissionHold.Reset();flight.deferred.clear();}
  // Allocation and container insertion happen before enqueuing the private wait,
  // so exceptions cannot strand the GPU behind an unowned fence.
  if(catalog.Healthy())catalog.Mark(q->id);
  flight.stamp=catalog.TailStamp(q->id);flights.push_back(std::move(flight));++submits;
  if(!catalog.Healthy())GIOwnedPipeline::active=false;
  if(submits.load()<=8||submits.load()%600==0)Log("GI_OWNED_QUEUE submissions=%llu healthy=%u reason=%s pending=%llu admitted=%llu completed_checkpoints=%llu compactions=%llu geometry_bytes=%llu",
   submits.load(),unsigned(catalog.Healthy()),catalog.History().FailureReason(),static_cast<UINT64>(pending.size()),GIOwnedPipeline::admitted.load(),checkpoints.load(),compactions.load(),geometryBudget->Held());
  if(submits.load()%600==0)GIOwnedMetrics::Report();
  if(prefixReady){
   auto& submitted=flights.back();
   if(submitted.admissionHold){
    if(FAILED(input->Wait(submitted.admissionHold.Get(),1))){
     for(const auto& item:submitted.deferred)item.first->Cancel();
     submitted.deferred.clear();submitted.admissionHold.Reset();prefixReady=false;
    }else {static UINT64 reports=0;if(++reports<=8||reports%120==0)
     Log("GI_OWNED_DEFERRED revision=R319 held=1 queue=%llu uses=%llu count=%llu",q->id,UINT64(pending.size()),reports);}
   }
   if(prefixReady){ID3D12CommandList* prefix=submitted.prefix.Get();input->ExecuteCommandLists(1,&prefix);}
  }

 }catch(...){for(const auto& item:pending)item.first->Cancel();catalog.Invalidate("live_submission_exception");Quarantine(count,lists);}
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
 ResolveDeferred();
 SetLastError(error);mutex.unlock();
}
static bool BeforeFence() noexcept {if(!Active())return false;mutex.lock();return true;}
static void AfterFence(ID3D12CommandQueue* input,ID3D12Fence* fence,UINT64 value,bool signal,HRESULT result,bool locked) noexcept {
 if(!locked)return;const DWORD error=GetLastError();
 try {
  if(!IsGameQueue(input)){SetLastError(error);mutex.unlock();return;}
  auto* q=FindQueue(input);const auto id=FenceID(fence);
  if(!q||!id)catalog.Invalidate("live_fence_identity");
  else {
   Trace(signal?"signal":"wait",q->id,id,value,fence->GetCompletedValue(),result);
   auto entry=std::find_if(fences.begin(),fences.end(),[&](const Fence& f){return f.id==id;});
   if(signal){catalog.Signal(q->id,id,value,SUCCEEDED(result));
    if(SUCCEEDED(result)){entry->observedSignal=(std::max)(entry->observedSignal,value);if(value>=entry->pendingValue)entry->pendingValue=0;}
   }else {catalog.Wait(q->id,id,value,SUCCEEDED(result));
    // Only unknown signal ranges need polling. Known GPU signals retain their
    // exact producer edges; completed initial values need no blocking wait.
    if(SUCCEEDED(result)&&value>entry->observedSignal){entry->pendingValue=(std::max)(entry->pendingValue,value);entry->hasCompletion=false;PollNativeFences();}
   }
  }
 }catch(...){catalog.Invalidate("live_fence_exception");}
 ResolveDeferred();
 SetLastError(error);mutex.unlock();
}
static void CpuSignal(ID3D12Fence* input) noexcept {
 if(!Active())return;const DWORD error=GetLastError();std::lock_guard<std::recursive_mutex> guard(mutex);
 ComPtr<IUnknown> identity;
 if(input&&SUCCEEDED(input->QueryInterface(IID_PPV_ARGS(&identity))))for(const auto& f:fences)if(f.identity.Get()==identity.Get()){catalog.Invalidate("live_cpu_signal_on_observed_fence");ResolveDeferred();break;}
 SetLastError(error);
}
static const GINativeSubmission::Callbacks callbacks{&Before,&After,&BeforeFence,&AfterFence,&CpuSignal};
[[maybe_unused]] static const bool registered=[] {GINativeSubmission::observer.store(&callbacks);controlGameQueueObserver.store(&RegisterGameQueue);return true;}();

}
