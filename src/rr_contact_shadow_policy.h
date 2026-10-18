#pragma once
// R13: preserve Control's contact-shadow reconstruction before composition.
// RR still reconstructs the final lighting; this restores only the native
// shadow visibility filter, without changing material guides or GI/reflections.
namespace control_rr {
struct RRContactShadowFilterPolicy {
    bool temporal{};
    int spatialSize{};
    int spatialStep{};
    bool neutralized{};
};
inline RRContactShadowFilterPolicy ContactShadowFilterPolicy(
    bool /*fullRR*/, bool /*contactShadowEnabled*/, bool temporal,
    int spatialSize, int spatialStep) noexcept {
    return {temporal, spatialSize, spatialStep, false};
}
}
