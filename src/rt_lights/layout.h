#pragma once
#ifdef __cplusplus
#include <cstdint>
namespace rt_lights {
using uint = std::uint32_t;
struct float3 { float x,y,z; };
#endif
// One task per receiver/light pair. All geometric inputs use world space.
struct ShadowTask {
    float3 receiver; float originBias;
    float3 normal; float endpointBias;
    float3 lightCenter; uint lightId;
    float3 emitterAxisU; uint sampleCount;
    float3 emitterAxisV; uint instanceMask;
    uint receiverId; uint frameTag; uint reserved0; uint reserved1;
};
struct Visibility {
    float value; float nearestBlocker; uint valid; uint lightId;
    uint receiverId; uint frameTag; uint reserved0; uint reserved1;
};
// Native adapter must provide each light's unshadowed contribution, not the
// combined lighting buffer. Identity checks prevent cross-light/history reuse.
struct Contribution {
    uint visibilityIndex; uint receiverId; uint lightId; uint frameTag;
    float3 diffuse; float reserved0;
    float3 specular; float reserved1;
};
struct ReceiverRange { uint first; uint count; uint frameTag; uint reserved; };
struct Lighting { float3 diffuse; uint valid; float3 specular; uint lightCount; };
#ifdef __cplusplus
static_assert(sizeof(ShadowTask)==96);
static_assert(sizeof(Visibility)==32);
static_assert(sizeof(Contribution)==48);
static_assert(sizeof(ReceiverRange)==16);
static_assert(sizeof(Lighting)==32);
} // namespace rt_lights
#endif
