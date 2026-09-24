#pragma once
#include "rr_user_control.h"
#include "rr_reflection_windows_access.h"
#include "rr_reflection_present.h"
#include "rr_distance_input.h"
// Own reflection copies consumed by the distance/RR path. All native/device
// identities are hash-locked; allocations remain bounded and fence-retired.
struct RRReflectionOwner {
 ID3D12Device* device=nullptr;
 ID3D12CommandQueue* queue=nullptr;
 ID3D12Fence* fence=nullptr;
 control_rr_reflection::D3D12Api api{};
 control_rr_reflection::D3D12Backend backend;
 control_rr_reflection::Capture<control_rr_reflection::D3D12Backend> capture;
 control_rr_reflection::PresentLedger presents;
 control_rr_reflection::Shape requested[2]{},allocated[2]{};
 RRDistanceInput distanceInputs[3]{};
 unsigned long long epoch=0,recorded=0,retired=0,submitted=0,skips=0;
 DWORD recordingThread=0;
 bool preparing=false,prepared=false,stopped=false;
 RRReflectionOwner():backend(api),capture(backend){}
};
static SRWLOCK rrReflectionLock=SRWLOCK_INIT;
static RRReflectionOwner* rrReflection=nullptr; // retained through exit on uncertainty
static control_rr_reflection::WindowsAccess rrReflectionAccess;
static unsigned long long rrReflectionCalls=0,rrReflectionRejected=0;
static unsigned rrReflectionBindingDiagnostics=0;
static void (*rrReflectionSubmissionObserver)(control_rr::Lease,std::uint64_t)=nullptr;
// Bounded failure evidence only; these borrowed reads never authorize a copy.
static void RRReflectionBindingDetails(void* owner) noexcept {
 if(rrReflectionBindingDiagnostics++>=4)return;
 __try {
  auto a=rrReflectionAccess.Callbacks();
  for(unsigned lane=0;lane<2;++lane){
   std::uintptr_t container=0,native=0,shader=0,resource=0,tracker=0,device=0;
   unsigned flags=0,state=0;unsigned char manual=0;control_rr_reflection::Shape shape{};
   const bool ownerRead=control_rr_reflection::Read(a,reinterpret_cast<std::uintptr_t>(owner),lane?0x10:0,container);
   if(ownerRead&&container)native=a.containerNative(a.context,container);
   shader=a.shaderNative(a.context,reinterpret_cast<std::uintptr_t>(verifiedRenderer)+(lane?control_rr_reflection::PositionShaderRva:control_rr_reflection::MaterialShaderRva));
   const bool fields=native&&control_rr_reflection::Read(a,native,0x38,flags)&&control_rr_reflection::Read(a,native,0x68,manual)&&
    control_rr_reflection::Read(a,native,0x88,resource)&&control_rr_reflection::Read(a,native,0x50,tracker);
   if(!tracker&&native)tracker=native+0x40;
   const bool stateRead=tracker&&control_rr_reflection::Read(a,tracker,0x20,state);
   const bool described=resource&&a.describe(a.context,resource,shape,device);
   Log("RR_REFLECTION_BINDING_DETAIL lane=%u owner_read=%u fields_read=%u shader_match=%u flags=0x%X manual=%u state_read=%u state=0x%X described=%u format=%u width=%llu height=%u layers=%u mips=%u samples=%u resource_flags=0x%X stage=reflection_capture",
    lane,unsigned(ownerRead),unsigned(fields),unsigned(native&&native==shader),flags,unsigned(manual),unsigned(stateRead),state,unsigned(described),shape.format,
    static_cast<unsigned long long>(shape.width),shape.height,shape.layers,shape.mips,shape.samples,shape.flags);
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){Log("RR_REFLECTION_BINDING_DETAIL fault=0x%08lX",GetExceptionCode());}
}
struct RRReflectionReadContext {void* commandContext=nullptr;};
static bool RRReflectionCurrent(void* user,control_rr_reflection::Context& out) noexcept {
 out={};RRGuideInputSnapshot native{};CameraSnapshot camera{};
 DWORD fault=0;const char* reason="unavailable";ID3D12Device* device=nullptr;bool good=false;
 __try {
  // Native submit uses Remedy's ThreadID enum, not the Windows thread ID.
  auto* d3d=reinterpret_cast<unsigned char*>(verifiedD3d);
  using NativeThreadFn=int (*)();
  auto nativeThread=*reinterpret_cast<NativeThreadFn*>(d3d+0x5D5D8);
  if(!nativeThread||nativeThread()!=*reinterpret_cast<const int*>(d3d+0xF6C34))return false;
  auto* expected=static_cast<RRReflectionReadContext*>(user);
  if(RRGuideReadRendererContext(&native,&reason)&&ReadCamera(&camera,&fault,&reason)&&
     camera.engineFrame==native.engineFrame&&camera.view&&
     (!expected||!expected->commandContext||expected->commandContext==native.commandContext)&&
     SUCCEEDED(native.commandList->GetDevice(IID_PPV_ARGS(&device)))&&device){
   out={native.engineFrame,reinterpret_cast<std::uintptr_t>(native.commandList),
    reinterpret_cast<std::uintptr_t>(native.queue),reinterpret_cast<std::uintptr_t>(device),
    reinterpret_cast<std::uintptr_t>(camera.view)};good=true;
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){good=false;}
 if(device){__try {device->Release();}__except(EXCEPTION_EXECUTE_HANDLER){good=false;}}
 if(!good)out={};return good;
}
static void RRReflectionSkip(const char* reason) noexcept {
 const auto n=++rrReflectionRejected;
 if(rrReflection)++rrReflection->skips;
 if(n<=8||(n&(n-1))==0)Log("RR_REFLECTION_SKIP count=%llu reason=%s stage=reflection_capture",n,reason);
}
static void RRReflectionStop(const char* reason) noexcept {
 if(rrReflection)rrReflection->stopped=true;
 Log("RR_REFLECTION_STOP reason=%s resources_retained=1 stage=reflection_capture",reason);
}
static DWORD WINAPI RRReflectionPrepare(void* argument) noexcept {
 auto* c=static_cast<RRReflectionOwner*>(argument);
 AcquireSRWLockExclusive(&rrReflectionLock);
 __try {
  if(c->stopped||!c->capture.Idle()||c->capture.Stopped())__leave;
  if(!c->fence&&FAILED(c->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c->fence)))){
   control_rr::RRUserCaptureFailure(control_rr::RRWaitReason::CaptureAllocation);
   RRReflectionStop("fence_allocation_failed");__leave;
  }
  if(c->prepared&&!c->backend.Dispose(c->capture.Idle())){RRReflectionStop("resize_not_idle");__leave;}
  c->prepared=false;
  c->api.createFactory=realDxgi?reinterpret_cast<control_rr_reflection::D3D12Api::CreateFactory>(
   GetProcAddress(realDxgi,"CreateDXGIFactory1")):nullptr;
  if(!c->backend.Prepare(reinterpret_cast<std::uintptr_t>(c->device),reinterpret_cast<std::uintptr_t>(c->queue),
    reinterpret_cast<std::uintptr_t>(c->fence),c->requested[0],c->requested[1])){
   const auto& report=c->backend.LastPrepare();
   const auto* reason=control_rr_reflection::PrepareFailureName(report.failure);
   Log("RR_REFLECTION_PREPARE_FAILED reason=%s width=%llu height=%u layers=%u material_bytes=%llu position_bytes=%llu required_bytes=%llu budget_bytes=%llu os_budget_bytes=%llu process_usage_bytes=%llu reserve_bytes=%llu budget_hr=0x%08lX budget_exception=0x%08lX slots=3 failed_resource=%u create_hr=0x%08lX create_exception=0x%08lX stage=reflection_capture",
    reason,static_cast<unsigned long long>(c->requested[0].width),c->requested[0].height,c->requested[0].layers,
    static_cast<unsigned long long>(report.resourceBytes[0]),static_cast<unsigned long long>(report.resourceBytes[1]),
    static_cast<unsigned long long>(report.requiredBytes),static_cast<unsigned long long>(report.budgetBytes),
    static_cast<unsigned long long>(report.memory.budget),static_cast<unsigned long long>(report.memory.usage),
    static_cast<unsigned long long>(report.memory.Reserve()),static_cast<unsigned long>(c->api.lastBudgetResult),c->api.lastBudgetException,
    report.failedResource,static_cast<unsigned long>(c->api.lastCreateResult),c->api.lastCreateException);
   control_rr::RRUserCaptureFailure(report.failure==control_rr_reflection::PrepareFailure::Budget?
    control_rr::RRWaitReason::CaptureBudget:report.failure==control_rr_reflection::PrepareFailure::BudgetQuery?
    control_rr::RRWaitReason::CaptureBudgetQuery:control_rr::RRWaitReason::CaptureAllocation);
   RRReflectionStop(reason);__leave;
  }
  if(!c->capture.Bind(c->backend.Key())||c->epoch==UINT64_MAX){RRReflectionStop("capture_bind_failed");__leave;}
  c->allocated[0]=c->requested[0];c->allocated[1]=c->requested[1];++c->epoch;c->prepared=true;
  control_rr::RRUserCaptureFailure(control_rr::RRWaitReason::Guides);
  const auto& admission=c->backend.LastPrepare();
  Log("RR_REFLECTION_MEMORY os_budget_bytes=%llu process_usage_bytes=%llu reserve_bytes=%llu allowed_bytes=%llu required_bytes=%llu policy=dxgi_local_headroom_minus_ten_percent",
   static_cast<unsigned long long>(admission.memory.budget),static_cast<unsigned long long>(admission.memory.usage),
   static_cast<unsigned long long>(admission.memory.Reserve()),static_cast<unsigned long long>(admission.budgetBytes),
   static_cast<unsigned long long>(admission.requiredBytes));
  Log("RR_REFLECTION_PREPARED width=%llu height=%u layers=%u bytes=%llu slots=3 epoch=%llu stage=reflection_capture",
   static_cast<unsigned long long>(c->allocated[0].width),c->allocated[0].height,c->allocated[0].layers,
   static_cast<unsigned long long>(c->backend.AllocationBytes()),c->epoch);
 } __except(EXCEPTION_EXECUTE_HANDLER){RRReflectionStop("prepare_exception");}
 c->preparing=false;ReleaseSRWLockExclusive(&rrReflectionLock);return 0;
}
static void RRReflectionQueuePrepare(RRReflectionOwner* c,const control_rr_reflection::Snapshot& s) noexcept {
 if(c->preparing||!c->capture.Idle())return;
 c->requested[0]=s.material.shape;c->requested[1]=s.position.shape;c->preparing=true;
 Log("RR_REFLECTION_PREPARE_REQUEST width=%llu height=%u layers=%u budget_policy=dxgi_local_headroom stage=reflection_capture",
  static_cast<unsigned long long>(s.material.shape.width),s.material.shape.height,s.material.shape.layers);
 if(!QueueUserWorkItem(&RRReflectionPrepare,c,WT_EXECUTEDEFAULT)){c->preparing=false;RRReflectionStop("prepare_queue_failed");}
}
// Keep constructor-failure cleanup outside the SEH function. Even a nothrow
// allocation can need C++ unwinding when its constructor is potentially throwing.
__declspec(noinline) static RRReflectionOwner* RRReflectionCreateOwner() noexcept {
 try {return new(std::nothrow) RRReflectionOwner;}
 catch(...) {return nullptr;}
}
// Called synchronously after the original reflection-only binding helper.
static void RRReflectionAfterBind(void* hitOwner) noexcept {
 const DWORD saved=GetLastError();
 if(!TryAcquireSRWLockExclusive(&rrReflectionLock)){SetLastError(saved);return;}
 void* first=nullptr;void* second=nullptr;void* recording=nullptr;
 bool lock1=false,lock2=false,lock3=false;
 RRReflectionReadContext reader{};
 __try {
  ++rrReflectionCalls;
  if(rrReflection&&(rrReflection->stopped||rrReflection->preparing))__leave;
  RRGuideInputSnapshot native{};const char* reason="unavailable";
  if(!RRGuideReadRendererContext(&native,&reason)){RRReflectionSkip(reason);__leave;}
  reader.commandContext=native.commandContext;
  auto access=rrReflectionAccess;access.user=&reader;access.current=&RRReflectionCurrent;
  control_rr_reflection::Snapshot before{},snapshot{};
  if(!control_rr_reflection::ReadBound(access.Callbacks(),reinterpret_cast<std::uintptr_t>(verifiedRenderer),
    control_rr_reflection::BindCallRva,reinterpret_cast<std::uintptr_t>(hitOwner),before)){
   RRReflectionSkip("binding_snapshot_rejected");RRReflectionBindingDetails(hitOwner);__leave;
  }
  first=reinterpret_cast<void*>(before.material.tracker);second=reinterpret_cast<void*>(before.position.tracker);
  if(first==second){RRReflectionSkip("aliased_trackers");__leave;}
  if(reinterpret_cast<std::uintptr_t>(first)>reinterpret_cast<std::uintptr_t>(second)){void* t=first;first=second;second=t;}
  recording=native.recordingLock;
  lock1=RRGuideTryAcquireNativeLock(first);if(!lock1){RRReflectionSkip("material_lock_busy");__leave;}
  lock2=RRGuideTryAcquireNativeLock(second);if(!lock2){RRReflectionSkip("position_lock_busy");__leave;}
  lock3=RRGuideTryAcquireNativeLock(recording);if(!lock3){RRReflectionSkip("recording_lock_busy");__leave;}
  RRGuideInputSnapshot again{};
  if(!RRGuideReadRendererContext(&again,&reason)||again.commandContext!=native.commandContext||
    again.recordingLock!=native.recordingLock||again.presentToken!=native.presentToken||
    !control_rr_reflection::ReadBound(access.Callbacks(),reinterpret_cast<std::uintptr_t>(verifiedRenderer),
     control_rr_reflection::BindCallRva,reinterpret_cast<std::uintptr_t>(hitOwner),snapshot)||
    !control_rr_reflection::Same(before.context,snapshot.context)||
    !control_rr_reflection::Same(before.material,snapshot.material)||!control_rr_reflection::Same(before.position,snapshot.position)){
   RRReflectionSkip("locked_snapshot_changed");__leave;
  }
  if(!rrReflection){
   auto* c=RRReflectionCreateOwner();if(!c)__leave;
   // Publish before any reference acquisition: SEH faults must retain partial ownership.
   rrReflection=c;
   c->recordingThread=GetCurrentThreadId();
   if(FAILED(native.commandList->GetDevice(IID_PPV_ARGS(&c->device)))||!c->device){RRReflectionStop("device_acquire_failed");__leave;}
   c->queue=native.queue;c->queue->AddRef();
  }
  auto* c=rrReflection;
  if(reinterpret_cast<std::uintptr_t>(c->device)!=snapshot.context.device||reinterpret_cast<std::uintptr_t>(c->queue)!=snapshot.context.queue){
   RRReflectionSkip("device_or_queue_changed");__leave;
  }
  if(!c->prepared||!control_rr_reflection::Same(c->allocated[0],snapshot.material.shape)||
     !control_rr_reflection::Same(c->allocated[1],snapshot.position.shape)){
   RRReflectionQueuePrepare(c,snapshot);__leave;
  }
  c->api.user=&reader;c->api.current=&RRReflectionCurrent;
  c->api.commandCount=reinterpret_cast<std::uint64_t*>(static_cast<unsigned char*>(native.commandContext)+8);
  if(!c->backend.SetRecording(snapshot.context)){RRReflectionSkip("recording_context_changed");__leave;}
  control_rr::Lease lease{};
  const auto perf=RRPerfBegin(native,RRPerfStage::ReflectionCopy);
  if(!c->capture.Record(snapshot,c->epoch,c->backend.Destinations(),lease)){
   RRPerfEnd(perf,false);
   if(c->capture.Stopped()||c->backend.Poisoned())RRReflectionStop("copy_recording_failed");
   else RRReflectionSkip("slots_busy_or_duplicate_frame");
   __leave;
  }
  RRPerfEnd(perf);
  if(!c->presents.Record(lease,native.presentToken)){RRReflectionStop("present_stamp_failed");__leave;}
  // Keep the exact producer's constants/depth identity with the same lease.
  // Provider equality is checked on this native recording thread, before GI.
  const char* distanceReason="unknown";
  const bool distanceInput=RRDistanceReadInput(snapshot,c->distanceInputs[lease.index],&distanceReason);
  control_rr::RRUserDistanceInput(distanceInput,distanceReason);
  if(!distanceInput&&(c->recorded<4||(c->recorded%240)==0))
   Log("RR_DISTANCE_INPUT_REJECT frame=%llu reason=%s",static_cast<unsigned long long>(snapshot.context.frame),distanceReason);
  ++c->recorded;
  if(c->recorded<=4||(c->recorded%240)==0)Log("RR_REFLECTION_COPIED frame=%llu present=%llu slot=%u serial=%llu width=%llu height=%u layers=%u stage=reflection_capture",
   static_cast<unsigned long long>(snapshot.context.frame),native.presentToken,unsigned(lease.index),
   static_cast<unsigned long long>(lease.serial),static_cast<unsigned long long>(snapshot.material.shape.width),snapshot.material.shape.height,snapshot.material.shape.layers);
 } __except(EXCEPTION_EXECUTE_HANDLER){RRReflectionStop("binding_or_copy_exception");}
 // Callbacks refer only to this synchronous stack frame, never to the worker.
 if(rrReflection){rrReflection->api.user=nullptr;rrReflection->api.current=nullptr;rrReflection->api.commandCount=nullptr;}
 __try {
  if(lock3)RRGuideReleaseNativeLock(recording);
  if(lock2)RRGuideReleaseNativeLock(second);
  if(lock1)RRGuideReleaseNativeLock(first);
 } __except(EXCEPTION_EXECUTE_HANDLER){RRReflectionStop("native_lock_release_exception");}
 ReleaseSRWLockExclusive(&rrReflectionLock);SetLastError(saved);
}
static void RRReflectionAfterPresent(unsigned long long returnedPresent) noexcept {
 const DWORD saved=GetLastError();
 if(!TryAcquireSRWLockExclusive(&rrReflectionLock)){SetLastError(saved);return;}
 __try {
  auto* c=rrReflection;
  if(c&&c->recordingThread!=GetCurrentThreadId())__leave;
  if(c&&c->prepared&&!c->preparing&&!c->stopped){
   unsigned submittedBefore=0;for(const auto& s:c->capture.Inspect())if(s.state==control_rr::SlotState::Submitted)++submittedBefore;
   if(!c->capture.Collect()){RRReflectionStop("fence_completion_invalid");__leave;}
   unsigned submittedAfter=0;for(const auto& s:c->capture.Inspect())if(s.state==control_rr::SlotState::Submitted)++submittedAfter;
   c->retired+=submittedBefore-submittedAfter;
   for(std::size_t i=0;i<3;++i){const auto state=c->capture.Inspect()[i];const control_rr::Lease lease{i,state.serial};
    if(state.state!=control_rr::SlotState::Ready||!c->presents.Covered(lease,returnedPresent))continue;
    // This hook is after originalPresent's renderer-list submission. Only copies
    // recorded with an older Present token on its primary Direct list qualify.
    if(!c->capture.SubmitCaptured(lease)){RRReflectionStop("fence_signal_failed");break;}
    if(rrReflectionSubmissionObserver)rrReflectionSubmissionObserver(lease,c->capture.Inspect()[i].fenceValue);
    ++c->submitted;
   }
  }
  if(returnedPresent%120==0)Log("RR_REFLECTION_STATUS present=%llu calls=%llu rejected=%llu recorded=%llu submitted=%llu retired=%llu stopped=%u stage=reflection_capture",
   returnedPresent,rrReflectionCalls,rrReflectionRejected,c?c->recorded:0,c?c->submitted:0,c?c->retired:0,c?unsigned(c->stopped):0);
 } __except(EXCEPTION_EXECUTE_HANDLER){RRReflectionStop("retirement_exception");}
 ReleaseSRWLockExclusive(&rrReflectionLock);SetLastError(saved);
}
