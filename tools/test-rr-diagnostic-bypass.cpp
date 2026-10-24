#include <cwchar>
#include "../src/rr_user_control.h"
#include "../src/native_lighting_live_state.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace control_rr;
    using namespace native_lighting;
    for (bool requested : {false,true}) {
        RRUserRequest(requested);
        for (Mode mode : {Mode::RoomBoth,Mode::RoomAuthoredClip,Mode::RoomAuthoredSpatial,Mode::RoomAuthoredRaw,Mode::RoomBlocker,Mode::RoomHitX,Mode::RoomHitY,Mode::RoomHitZ,Mode::RtSpotVisibilityRaw,Mode::RtSpotLightIds,Mode::RoomBoth}) {
            RRSetDiagnosticBypass(IsSignalDiagnostic(mode));
            assert(RRUserRequested()==requested);
            assert(RRUserRuntimeWorkRequested()==(requested&&!IsSignalDiagnostic(mode)));
        }
        RRUserSetRuntimePaused(true);
        RRSetDiagnosticBypass(false);
        assert(!RRUserRuntimeWorkRequested());
        RRUserSetRuntimePaused(false);
    }
    std::cout<<"PASS diagnostic bypass preserves saved preference and independent resize pause\n";
}
