#pragma once
namespace control_rr {
struct RRBroadDiffusePolicy {
    bool bypass{};
};
inline RRBroadDiffusePolicy BroadDiffusePolicy(bool fullRR, bool partial, unsigned rtEffects, bool preserveNative = false) noexcept {
    constexpr unsigned kRTSunShadow = 4u;
    constexpr unsigned kRTAO = 16u;
    return { !preserveNative && fullRR && !partial && ((rtEffects & (kRTSunShadow | kRTAO)) != 0) };
}
}
