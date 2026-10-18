#pragma once
static bool RRInputCaptureRecordGuarded(RRInputCaptureJob* c,const RRGuideInputSnapshot* native,
 const RRGuideInputTexture* inputs,DWORD* fault,bool* started,unsigned inputCount=3,bool outputUav=false) noexcept {
 if(!inputCount||inputCount>5)return false;
 void* locks[5]{};for(unsigned i=0;i<inputCount;++i)locks[i]=inputs[i].stateTracker;
 for(unsigned i=0;i<inputCount;++i)for(unsigned j=i+1;j<inputCount;++j)if(reinterpret_cast<uintptr_t>(locks[j])<reinterpret_cast<uintptr_t>(locks[i])){void* t=locks[i];locks[i]=locks[j];locks[j]=t;}
 void* held[5]{};unsigned heldCount=0;bool recordingHeld=false,okay=false;
 __try {
  __try {
   for(unsigned i=0;i<inputCount;++i){if(i&&locks[i]==locks[i-1])continue;if(!locks[i]||!RRGuideTryAcquireNativeLock(locks[i]))__leave;held[heldCount++]=locks[i];}
   bool alreadyHeld=false;for(unsigned i=0;i<heldCount;++i)if(held[i]==native->recordingLock)alreadyHeld=true;
   if(!alreadyHeld){recordingHeld=RRGuideTryAcquireNativeLock(native->recordingLock);if(!recordingHeld)__leave;}
   RRGuideInputSnapshot now{};const char* reason=nullptr;
   if(!RRGuideReadRendererContext(&now,&reason)||now.commandList!=native->commandList||now.commandContext!=native->commandContext||now.queue!=native->queue||now.recordingLock!=native->recordingLock||now.engineFrame!=native->engineFrame||now.presentToken!=native->presentToken)__leave;
   for(unsigned i=0;i<inputCount;++i){RRGuideInputTexture check{};
    if(!RRInputCaptureReadNative(inputs[i].nativeTexture,inputs[i].resource,&check,outputUav)||check.stateTracker!=inputs[i].stateTracker||check.trackedState!=inputs[i].trackedState||check.desc.Format!=inputs[i].desc.Format||check.desc.Width!=inputs[i].desc.Width||check.desc.Height!=inputs[i].desc.Height)__leave;
   }
   okay=RRInputCaptureRecord(c,native,fault,started);
  } __finally {
   if(recordingHeld)RRGuideReleaseNativeLock(native->recordingLock);
   while(heldCount)RRGuideReleaseNativeLock(held[--heldCount]);
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();}
 return okay;
}
