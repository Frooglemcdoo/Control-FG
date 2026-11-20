#include <initializer_list>
#include "../src/rr_gi_sample_control.h"
#include "../src/rr_frame_coordinator.h"
#include <cassert>
#include "../src/native_lighting_live_state.h"
#include <cstdio>
static void TestFilterRecovery(){
 using namespace control_rr;
 struct History {unsigned calls=0;bool success=true;bool ResetBeforeFilter() noexcept {++calls;return success;}};
 for(unsigned route=0;route<3;++route){ // REBLUR off, REBLUR+RR, native RR
  RRFrameCoordinator frame;History history;
  assert(frame.Begin({1,3840,2160,2560,1440,0},true,true));
  assert(frame.FeatureResult(true,true));
  assert(ResetHistoryBeforeFilter(frame,history)&&history.calls==0);
  assert(frame.BeginLighting(1)&&frame.BeginEvaluation(1,true,true)&&frame.EvaluationResult(true));
  assert(frame.Begin({2,3840,2160,2560,1440,0},true,true));
  assert(frame.FeatureResult(true,true)); // menu skips lighting/evaluation
  const bool requested=route!=0;
  assert(!frame.Begin({3,3840,2160,2560,1440,0},requested,true));
  assert(frame.Recovering()&&frame.FeatureResult(true,false)&&frame.NeedsHistoryReset());
  assert(ResetHistoryBeforeFilter(frame,history)&&history.calls==1);
  assert(ResetHistoryBeforeFilter(frame,history)&&history.calls==1); // once, before either filter route
  assert(frame.CompleteSR()&&!frame.Stopped());
  assert(frame.Begin({4,3840,2160,2560,1440,0},true,true));
  assert(frame.FeatureResult(true,true)&&frame.BeginLighting(4)&&frame.BeginEvaluation(4,true,true)&&frame.EvaluationResult(true));
  assert(!frame.Begin({5,3840,2160,1920,1080,0},true,true));
  assert(frame.FeatureResult(true,false)&&ResetHistoryBeforeFilter(frame,history)&&history.calls==2);
  assert(frame.CompleteSR()&&!frame.Stopped());
 }
 // Failure must remain terminal; no fake acknowledgement or automatic unstop.
 RRFrameCoordinator failure;History history;history.success=false;
 assert(failure.Begin({1,1920,1080,1920,1080,0},true,true));
 assert(failure.FeatureResult(true,true)&&failure.BeginLighting(1)&&failure.BeginEvaluation(1,true,true)&&failure.EvaluationResult(true));
 assert(!failure.Begin({2,1920,1080,1920,1080,0},false,true));
 assert(failure.FeatureResult(true,false));
 assert(!ResetHistoryBeforeFilter(failure,history)&&failure.Stopped()&&failure.NeedsHistoryReset());
}
int main(){
 TestFilterRecovery();

 using namespace control_rr;
 assert(GISampleRequested()==0); // R392 startup preserves the native count.
 for(unsigned requested: {0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u,14u,15u,16u,17u,32u,0xffffffffu}){
  const bool accepted=requested>=2&&requested<=16;
  assert(NormalizeGISampleRequest(requested)==(accepted?requested:0u));
  for(unsigned n: {0u,1u,2u,4u,16u,0xffffffffu}){
   assert(GISampleComparison(n,requested,false)==n);
   assert(GISampleComparison(n,requested,true)==(accepted&&n>=1&&n<=16?requested:n));
  }
 }
 unsigned cycle=0;
 for(unsigned expected=2;expected<=16;++expected){
  cycle=NextGISampleRequest(cycle);assert(cycle==expected);
 }
 assert(NextGISampleRequest(cycle)==2);
 for(unsigned pos=0;pos<=15;++pos){const unsigned rays=GISampleSliderValue(pos);assert(rays==(pos?pos+1:0));assert(GISampleSliderPosition(rays)==pos);}
 assert(GISampleSliderValue(16)==0);
 GISampleFrameSelection latch;
 RRFrameCoordinator frame;
 for(unsigned f=1;f<=7;++f){
  const unsigned next[]={2,2,3,4,2,0,3};
  GISampleSelect(next[f-1]);
  assert(frame.Begin({f,3840,2160,3840,2160,0},true,true));
  const bool changed=latch.Latch(GISampleRequested(),true);
  assert(changed==(f!=2));
  if(changed)assert(frame.ResetForSignalChange());
  assert(frame.FeatureResult(true,true));
  assert(frame.Reset()==(f!=2));
  const unsigned latched=latch.requested;
  GISampleSelect(8); // UI click during this frame must not change its count.
  assert(latch.requested==latched);
  assert(!frame.ResetForSignalChange()); // Too late to mutate this frame.
  assert(frame.BeginLighting(f));
  assert(frame.BeginEvaluation(f,true,true));
  assert(frame.EvaluationResult(true));
 }
 GISampleSelect(6);
 using native_lighting::Mode;
 for(auto mode:{Mode::RtSpotGi2,Mode::RtSpotGi8,Mode::RtSpotGi12,Mode::RtSpotGi16,Mode::RtSpotGiMenu,Mode::RtSpotWorldAlbedo}){
  const auto request=native_lighting::GiSampleRequest(mode,GISampleRequested());
  assert(latch.Latch(request,true));assert(!latch.Latch(request,true));
  assert(GISampleRequested()==6); // Diagnostics never persist their override.
  assert(GISampleComparison(8,latch.requested,true)==(request?request:8));
 }
 const unsigned before=latch.requested;
 assert(!latch.Latch(6,false)&&latch.requested==before);
 RRFrameCoordinator off;
 assert(!off.Begin({1,3840,2160,3840,2160,0},false,false));
 assert(!off.ResetForSignalChange());
 assert(!off.Reset());
 puts("PASS: GI integer counts 2 through 16; native=2 regression; invalid/native/off preservation; frame snapshot; one history reset per change; late reset rejected");
}
