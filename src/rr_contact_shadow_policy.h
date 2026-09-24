#pragma once

// r20u: Control's RT contact-shadow path calls DeferredLightFiltering::filterColorNoise
// with a temporal enable flag plus spatial filter size/step.  FULL RR should see the
// noisy ray-traced shadow signal, not a temporally accumulated/spatially blurred one.
// Keep the native function call itself so Control's output/binding contract remains
// intact, but neutralize its denoising knobs.  OFF/PARTIAL remain exact pass-through.
namespace control_rr {
struct RRContactShadowFilterPolicy {
    bool temporal{};
    int spatialSize{};
    int spatialStep{};
    bool neutralized{};
};

inline RRContactShadowFilterPolicy ContactShadowFilterPolicy(
    bool fullRR, bool contactShadowEnabled, bool temporal, int spatialSize, int spatialStep) noexcept {
    RRContactShadowFilterPolicy out{temporal, spatialSize, spatialStep, false};
    if (fullRR && contactShadowEnabled) {
        out.temporal = false;
        out.spatialSize = 0;
        // Keep a non-zero step.  The native shader still runs as an identity-style
        // pass so downstream resource ownership/output routing is preserved.
        out.spatialStep = 1;
        out.neutralized = true;
    }
    return out;
}
}
