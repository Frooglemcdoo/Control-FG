#include "../../src/rr_frame_coordinator.h"
#include "../../src/rr_user_control.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
using namespace control_rr;
static void sr(RRFrameCoordinator& c,RRFrameKey k,bool requested,bool ready){
 assert(!c.Begin(k,requested,ready));assert(!c.Stopped());assert(c.FeatureResult(true,false));
 if(c.NeedsHistoryReset())assert(c.HistoryResetResult(true));
 assert(c.CompleteSR(true,true));
}
static void rr(RRFrameCoordinator& c,RRFrameKey k){
 assert(c.Begin(k,true,true));assert(c.FeatureResult(true,true));assert(c.Reset());assert(!c.Resizing());
 assert(c.BeginLighting(k.frame));assert(c.BeginEvaluation(k.frame,true,true));assert(c.EvaluationResult(true));
}
int main(){
 // Reproduce r20j: warm 1440p with toggle OFF, change to 4K, then request RR.
 RRFrameCoordinator c;RRFrameKey k{1,3840,2160,2560,1440};sr(c,k,false,true);
 ++k.frame;k.width=3840;k.height=2160;sr(c,k,false,true);assert(c.Resizing());
 ++k.frame;sr(c,k,true,false);++k.frame;rr(c,k);
 // Repeated render and output changes while RR is active. Old ready=true
 // cannot select RR on the transition frame, even if the new extent matches
 // a previously used allocation. Warmup and history are independently gated.
 for(unsigned n=0;n<1000;++n){
  ++k.frame;k.width=n%2?3840:2560;k.height=n%2?2160:1440;
  sr(c,k,true,true);assert(c.Resizing()&&!c.Recovering());
  for(unsigned j=0;j<4;++j){++k.frame;sr(c,k,true,false);}
  ++k.frame;rr(c,k);
 }
 ++k.frame;k.outputWidth=7680;k.outputHeight=4320;sr(c,k,true,true);++k.frame;rr(c,k);
 ++k.frame;k.width=7680;k.height=4320;sr(c,k,true,true);++k.frame;rr(c,k);
 // Menu may apply several choices without lighting or SR evaluation in between.
 ++k.frame;k.width=2560;k.height=1440;
 assert(!c.Begin(k,true,true));assert(c.FeatureResult(true,false));
 ++k.frame;k.width=3840;k.height=2160;
 assert(!c.Begin(k,true,true));assert(c.FeatureResult(true,false));assert(c.Recovering());
 assert(c.HistoryResetResult(true));assert(c.CompleteSR());++k.frame;rr(c,k);
 // Resizing cannot recover partially recorded RR lighting or evaluation faults.
 for(unsigned stage=0;stage<2;++stage){
  RRFrameCoordinator bad;RRFrameKey a{1,3840,2160,2560,1440};
  assert(bad.Begin(a,true,true));assert(bad.FeatureResult(true,true));assert(bad.BeginLighting(1));
  if(stage)assert(bad.BeginEvaluation(1,true,true));
  ++a.frame;a.width=3840;a.height=2160;assert(!bad.Begin(a,true,true));assert(bad.Stopped());
 }
 RRUserRequest(true);RRUserFrameStatus(true,false,false,false,true);
 assert(rrUserStatus.load()==RRUserStatus::Resizing&&std::wcsstr(RRUserLabel(),L"rebuilding"));
 RRUserCaptureFailure(RRWaitReason::CaptureBudget);RRUserWaitingStatus(true,true,1);
 assert(std::wcsstr(RRUserLabel(),L"available GPU memory"));
 RRUserCaptureFailure(RRWaitReason::Guides);RRUserDistanceInput(true,"ready");RRUserWaitingStatus(true,true,1);
 RRUserFrameStatus(true,false,true,false,true);assert(rrUserStatus.load()==RRUserStatus::Stopped);
 RRUserRequest(false);RRUserFrameStatus(false,false,false,false,true);assert(rrUserStatus.load()==RRUserStatus::Off);
 puts("PASS logged OFF/resize/ON path, 1000 active resolution cycles, 8K/render/output changes, rapid menu choices, history/SR gates, reset, terminal late faults and UI");
}
