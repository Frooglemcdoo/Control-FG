#include <initializer_list>
#include "../src/rr_gi_sample_control.h"
#include "../src/rr_frame_coordinator.h"
#include <cassert>
#include "../src/native_lighting_live_state.h"
#include <cstdio>
int main(){
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
