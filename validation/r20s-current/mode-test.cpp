#include <cassert>
#include <cstdio>
#include "../../src/rr_user_control.h"
int main(){
 using namespace control_rr;
 assert(RRUserPresetLiveSwitchable(RRPresetE,RRPresetF));
 assert(RRUserPresetLiveSwitchable(RRPresetF,RRPresetE));
 assert(!RRUserPresetLiveSwitchable(RRPresetF,RRPresetK));
 assert(!RRUserPresetLiveSwitchable(RRPresetK,RRPresetE));
 const auto g=RRUserPresetSelectionGeneration();
 RRUserSetPresetValue(RRPresetE); assert(RRUserPresetValue()==RRPresetE); assert(RRUserPresetSelectionGeneration()==g+1);
 RRUserSetPresetValue(RRPresetF); assert(RRUserPresetValue()==RRPresetF); assert(RRUserPresetSelectionGeneration()==g+2);
 RRUserSetSkinMode(RRSkinMode::Responsivity); assert(RRUserSkinResponsivity());
 RRUserSetSkinMode(RRSkinMode::Off); assert(!RRUserSkinResponsivity());
 RRUserSetMode(RRUserMode::Partial); assert(RRUserPartialRequested()&&!RRUserRequested());
 RRUserSetMode(RRUserMode::Full); assert(RRUserRequested()&&!RRUserPartialRequested());
 std::puts("PASS r20s user policy: E/F live switch, K/L/M restart-only, skin responsivity toggle, PARTIAL diagnostic mode");
}
