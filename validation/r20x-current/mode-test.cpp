#include <cassert>
#include "../../src/rr_broad_diffuse_policy.h"
#include "../../src/rr_user_control.h"
int main(){
  using control_rr::BroadDiffusePolicy;
  assert(!BroadDiffusePolicy(false,false,4u|16u).bypass);
  assert(!BroadDiffusePolicy(true,true,4u|16u).bypass);
  assert(BroadDiffusePolicy(true,false,4u).bypass);
  assert(BroadDiffusePolicy(true,false,16u).bypass);

  assert(!control_rr::RRUserRuntimePaused());
  control_rr::RRUserSetRuntimePauseReason(control_rr::RRRuntimePauseResize,true);
  assert(control_rr::RRUserRuntimePaused());
  assert(control_rr::RRUserRuntimePauseReasons()==control_rr::RRRuntimePauseResize);
  control_rr::RRUserSetPresetTransitionPaused(true);
  assert((control_rr::RRUserRuntimePauseReasons()&control_rr::RRRuntimePausePreset)!=0);
  control_rr::RRUserSetRuntimePaused(false); // clear resize only
  assert(control_rr::RRUserRuntimePaused()); // preset pause must survive
  assert(control_rr::RRUserRuntimePauseReasons()==control_rr::RRRuntimePausePreset);
  control_rr::RRUserSetPresetOptionsWindow(true);
  assert(control_rr::RRUserPresetOptionsWindow());
  control_rr::RRUserSetPresetTransitionPaused(false);
  assert(!control_rr::RRUserRuntimePaused());
  control_rr::RRUserSetPresetOptionsWindow(false);

  const auto generation=control_rr::RRUserPresetSelectionGeneration();
  control_rr::RRUserSetPresetValue(control_rr::RRPresetE);
  assert(control_rr::RRUserPresetValue()==control_rr::RRPresetE);
  assert(control_rr::RRUserPresetSelectionGeneration()==generation+1);
  control_rr::RRUserSetPresetValue(control_rr::RRPresetE);
  assert(control_rr::RRUserPresetSelectionGeneration()==generation+1);
  control_rr::RRUserSetPresetValue(control_rr::RRPresetF);
  assert(control_rr::RRUserPresetSelectionGeneration()==generation+2);
  return 0;
}
