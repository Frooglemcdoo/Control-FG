#include <cassert>
#include <cstdio>
#include "../../src/rr_user_control.h"
int main(){
 using namespace control_rr;
 RRUserSetMode(RRUserMode::Off);
 assert(!RRUserRuntimeWorkRequested());
 RRUserSetMode(RRUserMode::Partial);
 assert(RRUserPartialRequested());
 assert(!RRUserRuntimeWorkRequested());
 RRUserSetMode(RRUserMode::Full);
 assert(RRUserRequested());
 assert(RRUserRuntimeWorkRequested());
 RRUserSetMode(RRUserMode::Off);
 assert(!RRUserRuntimeWorkRequested());
 assert(RRUserPresetLiveSwitchable(RRPresetE,RRPresetF));
 assert(RRUserPresetLiveSwitchable(RRPresetF,RRPresetE));
 assert(!RRUserPresetLiveSwitchable(RRPresetF,RRPresetK));
 std::puts("PASS r20t policy: recurring RR auxiliary work only in FULL; PARTIAL/OFF excluded; E/F live switch preserved");
}
