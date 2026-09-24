#pragma once
// Called with rrReflectionLock held. A newer reflection allocation epoch is
// published only after Capture::Idle proves every old lease fence completed.
static bool RRDistanceCanResize(const RRDistanceOwner* owner,unsigned long long epoch) noexcept {
    const auto state=owner->capturePolicy.state;
    return !owner->preparing&&!owner->stopped&&!owner->exporting&&
        (!owner->lastUseEpoch||epoch>owner->lastUseEpoch)&&
        state!=control_rr::CaptureState::Recording&&state!=control_rr::CaptureState::Recorded&&
        state!=control_rr::CaptureState::Submitted;
}
static void RRDistanceReleaseOutputs(RRDistanceOwner* owner) noexcept {
    for(auto& slot:owner->slots){RRGuideRelease(slot.output);RRGuideRelease(slot.status);RRGuideRelease(slot.heap);}
    for(auto& resource:owner->readback)RRGuideRelease(resource);
    owner->capturePolicy={};owner->captured={};owner->recorded=0;owner->prepared=false;
}
