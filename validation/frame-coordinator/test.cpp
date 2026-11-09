#include "../../src/rr_frame_coordinator.h"
#include "../../src/rr_user_control.h"
#include "../../src/rr_indirect_call.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <initializer_list>
using namespace control_rr;
static RRFrameKey key(std::uint64_t frame){return {frame,3840,2160,2560,1440};}
int main(){
 RRFrameCoordinator c;
 assert(!c.Begin(key(1),true,false));assert(c.FeatureResult(true,false));c.CompleteSR();assert(!c.Stopped());
 assert(c.Begin(key(2),true,true));assert(c.FeatureResult(true,true));assert(c.Reset());
 assert(c.BeginLighting(2));assert(c.BeginEvaluation(2,true,true));assert(c.EvaluationResult(true));
 assert(c.Begin(key(3),true,true));assert(c.FeatureResult(true,true));assert(!c.Reset());
 assert(c.BeginLighting(3));assert(!c.BeginEvaluation(3,false,true));assert(c.Stopped());
 assert(!c.Begin(key(4),true,true));assert(c.FeatureResult(true,false));assert(c.NeedsHistoryReset());
 assert(c.HistoryResetResult(true));assert(c.Reset());c.CompleteSR();assert(!c.NeedsHistoryReset());
 for(int fault=0;fault<8;++fault){
  RRFrameCoordinator f;assert(f.Begin(key(1),true,true));
  if(fault==0){assert(!f.FeatureResult(true,false));}
  else if(fault==1){assert(!f.FeatureResult(false,true));}
  else {assert(f.FeatureResult(true,true));
   if(fault==2){assert(!f.BeginLighting(2));}
   else if(fault==3){assert(!f.BeginEvaluation(1,true,true));}
   else {assert(f.BeginLighting(1));
    if(fault==4){assert(!f.BeginEvaluation(1,true,false));}
    else if(fault==5){assert(!f.BeginLighting(1));}
    else if(fault==6){assert(!f.Begin(key(2),true,true));}
    else {assert(f.BeginEvaluation(1,true,true));assert(!f.EvaluationResult(false));}
   }
  }
  assert(f.Stopped());assert(!f.Begin(key(3),true,true));
 }
 {RRFrameCoordinator f;assert(f.Begin(key(1),true,true));assert(f.FeatureResult(true,true));assert(f.BeginLighting(1));assert(f.BeginEvaluation(1,true,true));assert(f.EvaluationResult(true));auto resized=key(2);resized.width=1920;assert(!f.Begin(resized,true,true));assert(!f.Stopped()&&f.Resizing()&&f.Recovering());assert(f.FeatureResult(true,false));assert(f.HistoryResetResult(true));assert(f.CompleteSR());}
 {RRFrameCoordinator f;assert(f.Begin(key(1),true,true));assert(f.FeatureResult(true,true));assert(f.BeginLighting(1));assert(f.BeginEvaluation(1,true,true));assert(f.EvaluationResult(true));
  assert(!f.Begin(key(2),true,false));assert(f.FeatureResult(true,false));assert(f.HistoryResetResult(true));f.CompleteSR();
  assert(f.Begin(key(3),true,true));assert(f.FeatureResult(true,true));assert(f.Reset());}
 // UI request changes must not alter an already committed frame.
 assert(!RRUserRequested());RRFrameCoordinator toggle;
 assert(!toggle.Begin(key(10),RRUserRequested(),true));assert(toggle.FeatureResult(true,false));toggle.CompleteSR();
 RRUserRequest(true);assert(toggle.Begin(key(11),RRUserRequested(),true));assert(toggle.FeatureResult(true,true));
 RRUserFrameStatus(true,true,false);assert(rrUserStatus.load()==RRUserStatus::Waiting);
 assert(toggle.BeginLighting(11));RRUserRequest(false);assert(toggle.Selected());
 assert(toggle.BeginEvaluation(11,true,true));assert(toggle.EvaluationResult(true));RRUserPublish(RRUserStatus::Active);
 assert(!toggle.Begin(key(12),RRUserRequested(),true));assert(toggle.FeatureResult(true,false));
 assert(toggle.NeedsHistoryReset());assert(toggle.HistoryResetResult(true));toggle.CompleteSR();RRUserFrameStatus(false,false,false);
 assert(rrUserStatus.load()==RRUserStatus::Off);
 RRUserRequest(true);assert(toggle.Begin(key(13),RRUserRequested(),true));assert(toggle.FeatureResult(true,true));assert(toggle.Reset());
 toggle.Fail();RRUserFrameStatus(true,true,toggle.Stopped());assert(rrUserStatus.load()==RRUserStatus::Stopped);
 RRUserRequest(false);RRUserRequest(true);assert(!toggle.Begin(key(14),RRUserRequested(),true));
 IndirectCall6 original{{0xff,0x15,0x10,0,0,0}},patch{};std::uintptr_t slot=0;
 assert(DecodeIndirectCall(original,0x1000,slot)&&slot==0x1016);
 assert(ReplaceIndirectCall(original,0x1000,slot,0x2000,patch));assert(patch.bytes[0]==0xe8&&patch.bytes[5]==0x90);
 std::int32_t delta=0;std::memcpy(&delta,patch.bytes+1,4);assert(0x1005+delta==0x2000);
 assert(!ReplaceIndirectCall(original,0x1000,slot+1,0x2000,patch));
 assert(!ReplaceIndirectCall(original,0x1000,slot,UINTPTR_MAX,patch));
 original.bytes[0]=0xe8;assert(!DecodeIndirectCall(original,0x1000,slot));
 RRUserPublish(RRUserStatus::Waiting);RRUserRequest(true);
 RRUserWaitingStatus(true,false,0x2b);assert(rrUserWaitReason.load()==RRWaitReason::OtherRT);
 RRUserWaitingStatus(true,false,0);assert(rrUserWaitReason.load()==RRWaitReason::EnableReflections);
 RRUserWaitingStatus(false,true,1);assert(rrUserWaitReason.load()==RRWaitReason::Unsupported);
 RRUserDistanceInput(false,"clip_to_view_provider");RRUserWaitingStatus(true,true,1);
 assert(rrUserWaitReason.load()==RRWaitReason::Matrix);
 RRUserDistanceInput(true,"ready");RRUserWaitingStatus(true,true,1);
 assert(rrUserWaitReason.load()==RRWaitReason::Guides);
 for(auto reason:{RRWaitReason::CaptureBudget,RRWaitReason::CaptureBudgetQuery,RRWaitReason::CaptureAllocation}){
  RRUserCaptureFailure(reason);
  // The main render thread repeatedly publishes generic distance failures
  // after the allocation worker stopped. Keep the specific preparation cause.
  for(unsigned i=0;i<1000;++i){
   RRUserDistanceDispatch(false,"reflection_unavailable");RRUserWaitingStatus(true,true,1);
   assert(rrUserWaitReason.load()==reason);
  }
  assert(std::wcsstr(RRUserLabel(),reason==RRWaitReason::CaptureBudget?L"available GPU memory":reason==RRWaitReason::CaptureBudgetQuery?L"budget unavailable":L"allocation failed"));
  RRUserWaitingStatus(true,false,0x2b);assert(rrUserWaitReason.load()==RRWaitReason::OtherRT);
  RRUserWaitingStatus(false,true,1);assert(rrUserWaitReason.load()==RRWaitReason::Unsupported);
  RRUserRequest(false);RRUserFrameStatus(false,false,false);assert(std::wcscmp(RRUserLabel(),L"Off")==0);
  RRUserRequest(true);RRUserFrameStatus(true,false,false);RRUserWaitingStatus(true,true,1);
  assert(rrUserWaitReason.load()==reason);
 }
 RRUserCaptureFailure(RRWaitReason::Guides);RRUserDistanceInput(true,"ready");RRUserWaitingStatus(true,true,1);
 assert(rrUserWaitReason.load()==RRWaitReason::Guides);
 puts("PASS RR/SR sequencing, early rejection, eight terminal faults, history recovery, user toggle and mid-frame request isolation, resolution recovery and indirect-call encoding");
}
