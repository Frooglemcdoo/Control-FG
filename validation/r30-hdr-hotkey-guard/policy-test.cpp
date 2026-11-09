#include <cassert>
#include <iostream>
#include "../../src/fg_hdr_hotkey_policy.h"

int main() {
    using namespace control_fg_hdr_hotkey;
    assert(!ShouldHoldFG(Stage::Idle));
    assert(ShouldHoldFG(Stage::InterceptedWaitOffPresent));
    assert(ShouldHoldFG(Stage::OffQueuedWaitPresentReturn));
    assert(ShouldHoldFG(Stage::OffCommittedWaitKeyRelease));
    assert(ShouldHoldFG(Stage::ReplayedWaitHdrTransition));

    assert(!ReadyToReplay(Stage::Idle));
    assert(ReadyToReplay(Stage::OffCommittedWaitKeyRelease));

    assert(ChooseReplayMode(Stage::OffCommittedWaitKeyRelease, true, true, true) == ReplayMode::None);
    assert(ChooseReplayMode(Stage::OffCommittedWaitKeyRelease, false, true, true) == ReplayMode::BOnlyModifiersStillDown);
    assert(ChooseReplayMode(Stage::OffCommittedWaitKeyRelease, false, false, false) == ReplayMode::FullChordAfterRelease);
    assert(ChooseReplayMode(Stage::OffCommittedWaitKeyRelease, false, true, false) == ReplayMode::None);
    assert(ChooseReplayMode(Stage::OffCommittedWaitKeyRelease, false, false, true) == ReplayMode::None);
    assert(ChooseReplayMode(Stage::ReplayedWaitHdrTransition, false, false, false) == ReplayMode::None);

    assert(!BridgeCanTakeOwnership(Stage::ReplayedWaitHdrTransition, 7, 7));
    assert(BridgeCanTakeOwnership(Stage::ReplayedWaitHdrTransition, 7, 8));
    assert(!BridgeCanTakeOwnership(Stage::OffCommittedWaitKeyRelease, 7, 8));

    std::cout << "PASS r30 HDR hotkey guard policy: FG hold, safe replay timing, and bridge handoff\n";
    return 0;
}
