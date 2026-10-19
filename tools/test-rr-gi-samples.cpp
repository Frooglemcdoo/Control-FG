#include <initializer_list>
#include "../src/rr_gi_sample_control.h"
#include "../src/rr_frame_coordinator.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 for(unsigned requested: {0u,1u,2u,3u,4u,6u,8u,16u,0xffffffffu}){
  const bool accepted=requested==2||requested==4||requested==6||requested==8;
  assert(NormalizeGISampleRequest(requested)==(accepted?requested:0u));
  for(unsigned n: {0u,1u,2u,4u,16u,0xffffffffu}){
   assert(GISampleComparison(n,requested,false)==n);
   assert(GISampleComparison(n,requested,true)==(accepted&&n==1?requested:n));
  }
 }
 GISampleFrameSelection latch;
 RRFrameCoordinator frame;
 for(unsigned f=1;f<=7;++f){
  const unsigned next[]={4,4,6,8,2,0,4};
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
 const unsigned before=latch.requested;
 assert(!latch.Latch(6,false)&&latch.requested==before);
 RRFrameCoordinator off;
 assert(!off.Begin({1,3840,2160,3840,2160,0},false,false));
 assert(!off.ResetForSignalChange());
 assert(!off.Reset());
 puts("PASS: GI 2/4/6/8; invalid/native/off preservation; frame snapshot; one history reset per change; late reset rejected");
}
