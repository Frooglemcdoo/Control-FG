#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace rt_spot_shadow {
inline constexpr unsigned kMaxSamples = 64;
// CPU producer and HLSL structured-buffer layout. All positions are world space.
// Emitter vectors encode the two disk axes, including the emitter radius.
struct Task {
    float receiver[3]; float originBias;
    float normal[3]; float endpointBias;
    float lightCenter[3]; unsigned lightId;
    float emitterAxisU[3]; unsigned sampleCount;
    float emitterAxisV[3]; unsigned instanceMask;
};
static_assert(sizeof(Task) == 80);
static_assert(offsetof(Task, emitterAxisV) == 64);
struct Result { float visibility; unsigned valid; };
static_assert(sizeof(Result) == 8);

inline bool Valid(const Task& task) noexcept {
    const float* vectors[] = {task.receiver, task.normal, task.lightCenter,
                             task.emitterAxisU, task.emitterAxisV};
    for (const auto* vector : vectors)
        for (unsigned i = 0; i < 3; ++i) if (!std::isfinite(vector[i])) return false;
    const float lengthSquared = task.normal[0]*task.normal[0] + task.normal[1]*task.normal[1] + task.normal[2]*task.normal[2];
    return std::isfinite(lengthSquared) && lengthSquared > 0.5f && lengthSquared < 1.5f &&
        std::isfinite(task.originBias) && task.originBias >= 0 &&
        std::isfinite(task.endpointBias) && task.endpointBias >= 0 &&
        task.sampleCount > 0 && task.sampleCount <= kMaxSamples &&
        task.instanceMask > 0 && task.instanceMask <= 255;
}
// Replace one light's visibility, never multiply a combined lighting buffer.
// Invalid or incomplete ray results retain that light's native visibility.
inline float Resolve(float nativeVisibility, Result result) noexcept {
    return result.valid == 1 && std::isfinite(result.visibility) &&
        result.visibility >= 0 && result.visibility <= 1 ? result.visibility : nativeVisibility;
}
} // namespace rt_spot_shadow
