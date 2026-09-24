#include <cassert>
#include <iostream>
#include "../../src/fg_hdr_hard_reset_policy.h"
#include "../../src/fg_hdr_transition_policy.h"

using control_fg_hdr_hard_reset::Stage;

int main() {
    using namespace control_fg_hdr_hard_reset;
    assert(!Pending(Stage::Idle));
    assert(Pending(Stage::OffQueuedWaitPresent));
    assert(Pending(Stage::ResourcesFreedWaitBridge));

    assert(!CanFreeAfterPresent(Stage::Idle, true, true));
    assert(!CanFreeAfterPresent(Stage::OffQueuedWaitPresent, false, true));
    assert(!CanFreeAfterPresent(Stage::OffQueuedWaitPresent, true, false));
    assert(CanFreeAfterPresent(Stage::OffQueuedWaitPresent, true, true));

    assert(!CanRearm(Stage::ResourcesFreedWaitBridge, 7, 7, 7, false));
    assert(!CanRearm(Stage::ResourcesFreedWaitBridge, 7, 8, 7, false));
    assert(!CanRearm(Stage::ResourcesFreedWaitBridge, 7, 8, 8, true));
    assert(CanRearm(Stage::ResourcesFreedWaitBridge, 7, 8, 8, false));
    assert(!CanRearm(Stage::Idle, 7, 8, 8, false));

    // r31c regression: Win+Alt+B can change the fresh DXGI output HDR domain
    // without Control issuing ResizeBuffers at all. After the hard reset has
    // remained on the same bridge generation for a grace window, two fresh
    // tagged frames are sufficient to rearm FG in that unchanged render domain.
    assert(!NoBridgeGraceElapsed(129, 100, 100));
    assert(NoBridgeGraceElapsed(130, 100, 100));
    assert(!NoBridgeGraceElapsed(130, 100, 110));
    assert(NoBridgeGraceElapsed(140, 100, 110));
    assert(NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 7, true, true, 129, 100, 100, 0, 0) == 0);
    assert(NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 7, true, true, 130, 100, 100, 0, 0) == 1);
    assert(NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 7, true, true, 131, 100, 100, 130, 1) == 2);
    assert(NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 7, true, false, 131, 100, 100, 130, 1) == 0);
    assert(NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 8, true, true, 131, 100, 100, 130, 1) == 0);
    assert(!CanRearmWithoutBridge(Stage::ResourcesFreedWaitBridge, 7, 7, true, 130, 100, 100, 1));
    assert(CanRearmWithoutBridge(Stage::ResourcesFreedWaitBridge, 7, 7, true, 131, 100, 100, 2));
    assert(!CanRearmWithoutBridge(Stage::ResourcesFreedWaitBridge, 7, 8, true, 131, 100, 100, 2));
    assert(!CanRearmWithoutBridge(Stage::ResourcesFreedWaitBridge, 7, 7, false, 131, 100, 100, 2));

    // r31b regression: if ResizeBuffers/bridge transition already reached the
    // newly observed domain before the fresh-output observer catches up, this is
    // a delayed duplicate and must not start a second hard reset.
    assert(control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(1, 2, false, false)); // HDR -> SDR late observer
    assert(control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(2, 3, true, true));   // SDR -> HDR late observer
    assert(!control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(1, 1, true, false)); // early SDR detection
    assert(!control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(2, 2, false, true)); // early HDR detection
    assert(!control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(1, 2, true, false)); // bridge does not match new domain

    std::cout << "PASS r31c HDR hard-reset policy: off commit, resource free, bridge rearm, display-only no-resize rearm, delayed display duplicate suppression\n";
    return 0;
}
