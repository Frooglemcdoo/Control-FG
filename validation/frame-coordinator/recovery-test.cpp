#include "../../src/rr_frame_coordinator.h"
#include "../../src/rr_user_control.h"
#include <cassert>
#include <cwchar>
#include <cstdio>
using namespace control_rr;
static RRFrameKey key(std::uint64_t frame){return {frame,3840,2160,2560,1440};}
static void rr(RRFrameCoordinator& c,std::uint64_t frame,bool reset){
 assert(c.Begin(key(frame),true,true));assert(c.FeatureResult(true,true));assert(c.Reset()==reset);
 assert(c.BeginLighting(frame));assert(c.BeginEvaluation(frame,true,true));assert(c.EvaluationResult(true));
}
static void pause(RRFrameCoordinator& c,std::uint64_t frame){
 assert(c.Begin(key(frame),true,true));assert(c.FeatureResult(true,true));
 assert(!c.Begin(key(frame+1),true,true));assert(c.Recovering()&&!c.Stopped());
 assert(c.FeatureResult(true,false));
}
int main(){
 // Replay r20f's observed cadence: RR until 6531, no lighting/AA until 6893.
 // The old coordinator latched Stopped when Ready was abandoned at 6532.
 RRFrameCoordinator trace;
 for(std::uint64_t frame=2244;frame<=6531;++frame)rr(trace,frame,frame==2244);
 assert(trace.Begin(key(6532),true,true));assert(trace.FeatureResult(true,true));
 for(std::uint64_t frame=6533;frame<6893;++frame){
  assert(!trace.Begin(key(frame),true,true));assert(trace.Recovering()&&!trace.Stopped());
  assert(trace.FeatureResult(true,false));assert(trace.NeedsHistoryReset());
 }
 assert(!trace.Begin(key(6893),true,true));assert(trace.FeatureResult(true,false));
 assert(trace.HistoryResetResult(true));assert(trace.Recovering());
 assert(trace.CompleteSR(true));assert(!trace.Recovering()&&!trace.Stopped());
 rr(trace,6894,true);rr(trace,6895,false);
 // Repeated gaps, unavailable guides and user OFF stay in SR until a complete
 // native frame. Neither time nor an OFF/ON click clears a terminal fault.
 RRFrameCoordinator cycles;std::uint64_t f=1;
 for(unsigned n=0;n<1000;++n){
  rr(cycles,f++,true);pause(cycles,f);f+=2;
  for(unsigned gap=0;gap<7;++gap){
   assert(!cycles.Begin(key(f++),gap!=3,gap%2==0));assert(cycles.FeatureResult(true,false));
   assert(cycles.Recovering()&&!cycles.Stopped());
  }
  assert(cycles.HistoryResetResult(true));assert(cycles.CompleteSR(true));
  assert(!cycles.Recovering()&&!cycles.Stopped());
 }
 // No lighting yet means no modified native history, but still requires SR.
 {RRFrameCoordinator c;pause(c,1);assert(!c.NeedsHistoryReset());assert(c.CompleteSR());rr(c,3,true);}
 // Restoring history alone cannot authorize RR. Wait for successful SR too.
 {RRFrameCoordinator c;rr(c,1,true);pause(c,2);assert(c.HistoryResetResult(true));
  assert(!c.Begin(key(4),true,true));assert(c.FeatureResult(true,false));assert(c.Recovering());
  assert(c.CompleteSR());rr(c,5,true);}
 for(unsigned fault=0;fault<6;++fault){
  RRFrameCoordinator c;rr(c,1,true);pause(c,2);
  if(fault==0)assert(!c.CompleteSR());
  if(fault==1)assert(!c.HistoryResetResult(false));
  if(fault==2){assert(c.HistoryResetResult(true));assert(!c.CompleteSR(false));}
  if(fault==3){c.Fail("late_recording_failure");}
  if(fault==4)assert(!c.Begin(key(3),true,true));
  if(fault==5){assert(c.HistoryResetResult(true));assert(!c.CompleteSR(true,false));}
  assert(c.Stopped()&&!c.Recovering());assert(!c.Begin(key(5),true,true));
 }
 for(unsigned stage=0;stage<3;++stage){
  RRFrameCoordinator c;assert(c.Begin(key(1),true,true));
  if(stage){assert(c.FeatureResult(true,true));assert(c.BeginLighting(1));}
  if(stage==2)assert(c.BeginEvaluation(1,true,true));
  assert(!c.Begin(key(2),true,true));assert(c.Stopped()&&!c.Recovering());
 }
 RRUserRequest(true);RRUserFrameStatus(true,false,false,true);
 assert(rrUserStatus.load()==RRUserStatus::Recovering);
 assert(std::wcsstr(RRUserLabel(),L"Paused"));
 RRUserRequest(false);RRUserFrameStatus(false,false,false,true);
 assert(rrUserStatus.load()==RRUserStatus::Off);
 RRUserRequest(true);RRUserFrameStatus(true,false,true,true);
 assert(rrUserStatus.load()==RRUserStatus::Stopped);
 puts("PASS r20f pause/resume cadence, 1000 interruption cycles, SR/history recovery gates, toggle, reset on resumed RR, and committed/frame faults stay terminal");
}
