#pragma once
// Included after rrNativeClampApi and the native sample observation are defined.
// Read-only CPU provider evidence, never proof of GPU constant consumption.
#ifdef CONTROL_RR_TEMPORAL_PROBE
static void RRTemporalProbe(unsigned long long frame,unsigned samples) noexcept {
 static unsigned attempts=0,records=0;
 static unsigned long long lastFrame=~0ull;
 if(attempts>=1024||samples!=4||!rrNativeFrame.Selected()||frame==lastFrame)return;
 lastFrame=frame;++attempts;
 const DWORD saved=GetLastError();
 const char* reason="resolver";
 std::uint32_t temporal=0,cut=0,current=0;
 bool valid=false;
 struct Slot {std::int32_t id,tag;};
 __try {
  auto& api=rrNativeClampApi;
  if(api.ready&&api.resolve){
   Slot temporalSlot{-1,0},cutSlot{-1,0},currentSlot{-1,0};
   api.resolve(&temporalSlot,"g_uTemporalFrame");
   api.resolve(&cutSlot,"g_uCameraCut");
   api.resolve(&currentSlot,"g_uCurrentFrame");
   control_rr_native_clamp::Entry first{},second{};
   std::uint32_t again=0,cutAgain=0,currentAgain=0;
   reason="provider_read";
   if(api.ReadCurrent(temporalSlot.id,temporal,&first)&&api.ReadCurrent(cutSlot.id,cut)&&cut<=1&&api.ReadCurrent(currentSlot.id,current)){
    reason="provider_changed";
    valid=api.ReadCurrent(temporalSlot.id,again,&second)&&again==temporal&&
     !std::memcmp(&first,&second,sizeof(first))&&api.ReadCurrent(cutSlot.id,cutAgain)&&cutAgain==cut&&
     api.ReadCurrent(currentSlot.id,currentAgain)&&currentAgain==current;
    if(valid)reason="ready";
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){reason="provider_exception";valid=false;}
 if(valid)++records;
 native_lighting::Trace("rr_temporal_probe build=R193 frame=%llu samples=%u attempt=%u valid=%u temporal=%u camera_cut=%u current=%u stage=pre_reflection_filter cpu_provider=1 gpu_confirmed=0 reason=%s",
  frame,samples,attempts,unsigned(valid),temporal,cut,current,reason);
 if(attempts==1024)native_lighting::Trace("rr_temporal_probe_end attempts=%u records=%u automatic_limit=1",attempts,records);
 SetLastError(saved);
}
#endif
