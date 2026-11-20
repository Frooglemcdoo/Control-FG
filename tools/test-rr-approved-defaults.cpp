#include "../src/rr_user_control.h"
#include "../src/rr_gi_sample_control.h"
#include "../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 static_assert(!control_reblur::Enabled,"Restored RR must not enable REBLUR");
 assert(control_rr_clamp::requestedMode==control_rr_clamp::Mode::Override60);
 for(unsigned rays:{0u,4u,10u}){
  GISampleSelect(rays);RRUserRequest(true);
  assert(RRUserRequested()&&gi_reuse::ownedRequested&&GISampleRequested()==rays);
  RRUserRequest(false);assert(!gi_reuse::ownedRequested&&GISampleRequested()==rays);
  RRUserRequest(true);assert(gi_reuse::ownedRequested&&GISampleRequested()==rays);
 }
 control_rr_clamp::requestedMode=control_rr_clamp::Mode::Native;
 RRUserRequest(false);RRUserRequest(true);
 assert(control_rr_clamp::requestedMode==control_rr_clamp::Mode::Native);
 puts("PASS RR-owned GI without NRD; native/4/10-ray choices survive RR toggles; default 60% clamp and explicit override preserved");
}
