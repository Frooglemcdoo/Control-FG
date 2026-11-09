#include <cassert>
#include "../../src/rr_user_control.h"
int main(){
 using namespace control_rr;
 assert(RRUserModeValue()==RRUserMode::Off);
 RRUserSetMode(RRUserMode::Partial); assert(RRUserPartialRequested()); assert(!RRUserRequested()); assert(RRUserAnyRequested());
 RRUserSetMode(RRUserMode::Full); assert(RRUserRequested()); assert(!RRUserPartialRequested());
 RRUserSetMode(RRUserMode::Off); assert(!RRUserAnyRequested());
 RRUserSetPresetValue(RRPresetE); RRUserMarkPresetRestartRequired(true); assert(RRUserPresetNeedsRestart());
 RRUserConfirmPresetCreate(RRPresetE); assert(!RRUserPresetNeedsRestart()); assert(RRUserConfirmedPresetValue()==RRPresetE); assert(RRUserPresetCreateGeneration()==1);
 return 0;
}
