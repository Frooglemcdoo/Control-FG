#pragma once
namespace control_rr {
struct RRBroadDiffusePolicy {
    bool bypass{};
};
// R50 experiment: preserve the two broad DLF calls, including their native
// history and pass count. These are distinct from the dedicated diffuse-GI
// bypass, which remains unchanged. Do not infer denoiser ownership from the
// global sun-shadow/AO enable bits alone.
inline RRBroadDiffusePolicy BroadDiffusePolicy(
    bool /*fullRR*/, bool /*partial*/, unsigned /*rtEffects*/) noexcept {
    return {false};
}
}
