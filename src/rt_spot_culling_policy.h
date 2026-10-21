#pragma once
#include <cmath>

namespace native_lighting::spot_culling {
// Expand the camera frustum far enough to retain nearby off-screen casters.
// Keep the native far plane and respect the engine's expansion control range.
inline constexpr float kMinimumExpansion = 60.0f;
inline constexpr float kMaximumExpansion = 200.0f;
inline bool ValidExpansion(float value) noexcept {
    return std::isfinite(value) && value >= 0.0f;
}
inline float ExpandedView(float native, float farDistance) noexcept {
    if (!ValidExpansion(native) || !std::isfinite(farDistance) || farDistance <= 0.0f)
        return native;
    const float bounded = farDistance < kMinimumExpansion ? kMinimumExpansion :
        farDistance > kMaximumExpansion ? kMaximumExpansion : farDistance;
    return native > bounded ? native : bounded;
}
} // namespace native_lighting::spot_culling
