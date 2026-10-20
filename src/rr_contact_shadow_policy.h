#pragma once

namespace control_rr {
struct RRContactShadowFilterPolicy {
    bool temporal{};
    int spatialSize{};
    int spatialStep{};
    bool neutralized{};
};

// R50 experiment: retain native contact-shadow filtering before RR. The old
// full-RR policy removed both temporal accumulation and spatial filtering.
// Preserving the caller's parameters tests whether that raw visibility noise
// contributes to the stationary lighting speckles. Reflection/GI stay separate.
inline RRContactShadowFilterPolicy ContactShadowFilterPolicy(
    bool /*fullRR*/, bool /*contactShadowEnabled*/, bool temporal,
    int spatialSize, int spatialStep) noexcept {
    return {temporal, spatialSize, spatialStep, false};
}
}
