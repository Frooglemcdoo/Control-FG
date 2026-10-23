#pragma once
#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
namespace rt_spot_capture {
using uint = std::uint32_t;
struct float3 { float x, y, z; };
#endif
static const uint CaptureMaxRecords = 4096;
static const uint CaptureLayoutVersion = 1;
static const uint CaptureRayBytes = 48;
static const uint CaptureResultBytes = 80;
// Diagnostic-only ABI. Never pass this payload to a native 4-byte raygen.
// distance stays first so the native alpha-test any-hit can retain its ABI.
struct HitPayload {
    float distance;
    uint instanceIndex;
    uint instanceId;
    uint primitiveIndex;
    float barycentricU;
    float barycentricV;
    uint hitKind;
    uint geometryIndex;
};
// The adapter supplies the actual biased native origin, not a reconstruction
// with a second bias. All inputs must belong to one dispatch/TLAS generation.
struct CaptureRay {
    float3 origin; float maxDistance;
    float3 direction; uint lightId;
    uint pixelX; uint pixelY; uint frameTag; uint reserved;
};
struct CaptureResult {
    HitPayload hit;
    float3 origin; float maxDistance;
    float3 direction; uint lightId;
    uint pixelX; uint pixelY; uint frameTag; uint valid;
};
#ifdef __cplusplus
static_assert(sizeof(HitPayload) == 32);
static_assert(offsetof(HitPayload, distance) == 0);
static_assert(sizeof(CaptureRay) == CaptureRayBytes);
static_assert(sizeof(CaptureResult) == CaptureResultBytes);
} // namespace rt_spot_capture
#endif
