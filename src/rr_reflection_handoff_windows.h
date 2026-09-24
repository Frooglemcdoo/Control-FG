#pragma once
// Exact d3d+32991 call: append the CLOSED primary command context to queue+28.
// Native queue locking is already held. Never block waiting for our lock here.
using RRPrimaryAppendFn=std::uintptr_t (*)(void*,void*);
static RRPrimaryAppendFn rrPrimaryAppendOriginal=nullptr;
static RRAlbedoCallPatch rrPrimaryAppendPatch{};
static unsigned long long rrPrimaryQueuedCount=0;
static void RRReflectionObservePrimaryAppend(void* vector,void* argument) noexcept {
 if(!rrReflectionCaptureEnabled||!TryAcquireSRWLockExclusive(&rrReflectionLock))return;
 __try {
  auto* owner=rrReflection;
  if(!owner||owner->stopped||owner->recordingThread!=GetCurrentThreadId())__leave;
  const auto base=reinterpret_cast<std::uintptr_t>(verifiedD3d);
  const auto read=&control_rr_reflection::WindowsAccess::Read;
  std::uintptr_t queue=0,nativeQueue=0,closed=0,list=0;unsigned char closedFlag=0;
  unsigned long long frame=0;DWORD fault=0;
  if(!read(nullptr,base+0x111c38,&queue,8)||!queue||reinterpret_cast<std::uintptr_t>(vector)!=queue+0x28||
   !read(nullptr,queue+0x10,&nativeQueue,8)||nativeQueue!=reinterpret_cast<std::uintptr_t>(owner->queue)||
   !read(nullptr,reinterpret_cast<std::uintptr_t>(argument),&closed,8)||!closed||
   !read(nullptr,closed+0x18,&list,8)||!list||
   !read(nullptr,closed+0x28,&closedFlag,1)||closedFlag!=1||!ReadEngineFrameSafe(&frame,&fault))__leave;
  for(std::size_t i=0;i<3;++i){
   const auto& state=owner->capture.Inspect()[i];const control_rr::Lease lease{i,state.serial};
   const auto& input=owner->distanceInputs[i];
   if(state.state!=control_rr::SlotState::Ready||state.key.frame!=frame||!input.valid||input.context.list!=list||
    input.context.queue!=nativeQueue||!owner->presents.Matches(lease,presentCount.load()))continue;
   if(owner->capture.ObservePrimaryQueued(lease,input.context)){
    const auto count=++rrPrimaryQueuedCount;
    if(count<=4||(count%240)==0)Log("RR_REFLECTION_PRIMARY_QUEUED count=%llu frame=%llu slot=%u serial=%llu list=%p queue=%p",count,frame,unsigned(i),static_cast<unsigned long long>(lease.serial),reinterpret_cast<void*>(list),reinterpret_cast<void*>(nativeQueue));
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){Log("RR_REFLECTION_PRIMARY_OBSERVER_FAULT");}
 ReleaseSRWLockExclusive(&rrReflectionLock);
}
static std::uintptr_t RRReflectionHookPrimaryAppend(void* vector,void* argument) {
 const auto result=rrPrimaryAppendOriginal(vector,argument);
 const DWORD saved=GetLastError();
 RRReflectionObservePrimaryAppend(vector,argument);
 SetLastError(saved);return result;
}
static bool RRReflectionInstallPrimaryAppend(HMODULE d3d) noexcept {
 auto* base=reinterpret_cast<unsigned char*>(d3d);
 // Arguments are queue+0x28 and &closedPacket->context, after native Close.
 const unsigned char prefix[]{0x48,0x8d,0x4e,0x28};
 if(!d3d||d3d!=verifiedD3d||memcmp(base+0x3298d,prefix,sizeof(prefix)))return false;
 rrPrimaryAppendOriginal=reinterpret_cast<RRPrimaryAppendFn>(base+0x24e0);
 if(!RRAlbedoPrepareCallPatch(base+0x32991,reinterpret_cast<void*>(rrPrimaryAppendOriginal),reinterpret_cast<void*>(&RRReflectionHookPrimaryAppend),&rrPrimaryAppendPatch))return false;
 const bool changed=RRAlbedoExchangeCall(&rrPrimaryAppendPatch,true);
 if(changed&&rrPrimaryAppendPatch.writeHealthy){Log("RR_REFLECTION_PRIMARY_HOOK_READY site=0x32991 fifo_handoff=1");return true;}
 if(changed&&!RRAlbedoExchangeCall(&rrPrimaryAppendPatch,false))Log("RR_REFLECTION_PRIMARY_ROLLBACK_FAILED");
 return false;
}
