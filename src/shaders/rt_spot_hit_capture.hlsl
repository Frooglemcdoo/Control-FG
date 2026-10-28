#include "../rt_spot_capture_layout.h"

// These bindings fit the verified native contact-ray global root. A dedicated
// capture table must bind private buffers; NEVER bind the gameplay UAV here.
RaytracingAccelerationStructure captureScene : register(t6);
StructuredBuffer<CaptureRay> captureRays : register(t8);
ByteAddressBuffer captureStatus : register(t9);
RWStructuredBuffer<CaptureResult> captureResults : register(u2);
cbuffer CaptureLayout : register(b4) {
    uint captureCount;
    uint expectedFrameTag;
    uint hitRecordOffset;
    uint hitGeometryStride;
    uint missRecordIndex;
    uint captureLayoutVersion;
    uint captureEligibility;
    uint captureReserved1;
}

[shader("closesthit")]
void captureClosestHit(inout HitPayload hit, BuiltInTriangleIntersectionAttributes attributes) {
    hit.distance = RayTCurrent();
    hit.instanceIndex = InstanceIndex();
    hit.instanceId = InstanceID();
    hit.primitiveIndex = PrimitiveIndex();
    hit.barycentricU = attributes.barycentrics.x;
    hit.barycentricV = attributes.barycentrics.y;
    hit.hitKind = HitKind();
    hit.geometryIndex = GeometryIndex();
}

[shader("miss")]
void captureMiss(inout HitPayload hit) {
    // An any-hit invocation may have changed distance. Do not infer a miss
    // from it; only the final closest-hit handler sets the triangle hit kind.
    hit.hitKind = 0;
}

// Geometry-only queries deliberately avoid the native hit table and alpha shader.
// Widening a native TraceRay mask could select unrelated hit-table groups.
HitPayload geometryProbe(RayDesc ray, uint mask) {
    HitPayload hit = (HitPayload)0;
    hit.distance = ray.TMax;
    hit.instanceIndex = hit.instanceId = hit.primitiveIndex = hit.geometryIndex = 0xffffffff;
    RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
    query.TraceRayInline(captureScene, RAY_FLAG_NONE, mask, ray);
    while (query.Proceed()) {}
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        hit.distance = query.CommittedRayT();
        hit.instanceIndex = query.CommittedInstanceIndex();
        hit.instanceId = query.CommittedInstanceID();
        hit.primitiveIndex = query.CommittedPrimitiveIndex();
        hit.geometryIndex = query.CommittedGeometryIndex();
        float2 bary = query.CommittedTriangleBarycentrics();
        hit.barycentricU = bary.x; hit.barycentricV = bary.y;
        hit.hitKind = query.CommittedTriangleFrontFace() ? 254 : 255;
    }
    return hit;
}

[shader("raygeneration")]
void captureRayGeneration() {
    if (any(DispatchRaysIndex().yz)) return;
    uint index = DispatchRaysIndex().x;
    uint inputCount, inputStride, outputCount, outputStride;
    captureRays.GetDimensions(inputCount, inputStride);
    captureResults.GetDimensions(outputCount, outputStride);
    if (outputStride != CaptureResultBytes || index >= outputCount || index >= CaptureMaxRecords) return;
    CaptureResult result = (CaptureResult)0;
    captureResults[index] = result;
    if (captureEligibility == 1) {
        if (outputCount < CaptureMaxRecords * 3) return;
        captureResults[index + CaptureMaxRecords] = result;
        captureResults[index + CaptureMaxRecords * 2] = result;
    }
    uint statusBytes; captureStatus.GetDimensions(statusBytes);
    if (statusBytes < 16 || captureStatus.Load(0) != 0 || captureStatus.Load(12) != 0) return;
    // One bounded batch; invalid input yields an explicit invalid record.
    if (captureCount > CaptureMaxRecords || index >= captureCount || index >= inputCount ||
        inputStride != CaptureRayBytes || captureLayoutVersion != CaptureLayoutVersion || expectedFrameTag == 0 ||
        hitRecordOffset != 0 || hitGeometryStride != 2 || missRecordIndex != 0 ||
        captureEligibility > 1 || captureReserved1 != 0) return;
    CaptureRay input = captureRays[index];
    if (input.frameTag != expectedFrameTag || input.reserved != 0 ||
        !all(isfinite(input.origin)) || !all(isfinite(input.direction)) ||
        !isfinite(input.maxDistance) || input.maxDistance <= 0) return;
    float lengthSquared = dot(input.direction, input.direction);
    if (abs(lengthSquared - 1) > 0.0001) return;

    HitPayload hit = (HitPayload)0;
    hit.distance = input.maxDistance;
    hit.instanceIndex = hit.instanceId = hit.primitiveIndex = hit.geometryIndex = 0xffffffff;
    RayDesc ray;
    ray.Origin = input.origin;
    ray.Direction = input.direction;
    ray.TMin = 0;
    ray.TMax = input.maxDistance;
    // Same flags/mask/table indexing as R97's centered spotlight ray.
    // Closest accepted hit is required: never accept the first candidate.
    TraceRay(captureScene, RAY_FLAG_NONE, 1, hitRecordOffset,
             hitGeometryStride, missRecordIndex, ray, hit);
    if (!hit.hitKind) {
        hit = (HitPayload)0;
        hit.distance = input.maxDistance;
        hit.instanceIndex = hit.instanceId = hit.primitiveIndex = hit.geometryIndex = 0xffffffff;
    }
    result.hit = hit;
    result.origin = input.origin;
    result.maxDistance = input.maxDistance;
    result.direction = input.direction;
    result.lightId = input.lightId;
    result.pixelX = input.pixelX;
    result.pixelY = input.pixelY;
    result.frameTag = input.frameTag;
    result.valid = 1;
    captureResults[index] = result;
    // Only investigate native misses. The displayed shadow path is untouched.
    if (captureEligibility == 1 && !hit.hitKind) {
        result.hit = geometryProbe(ray, 1);
        captureResults[index + CaptureMaxRecords] = result;
        result.hit = geometryProbe(ray, 255);
        captureResults[index + CaptureMaxRecords * 2] = result;
    }
}
