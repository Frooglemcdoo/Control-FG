// R64 integration prototype; compiled but not dispatched by the game yet.
// Pair the custom closest-hit with a verified native alpha-test any-hit export.
// Never pair it with the native shadow closest-hit: that writes native UAVs.
struct Task {
    float3 receiver; float originBias;
    float3 normal; float endpointBias;
    float3 lightCenter; uint lightId;
    float3 emitterAxisU; uint sampleCount;
    float3 emitterAxisV; uint instanceMask;
};
struct Result { float visibility; uint valid; };
struct HitData { uint visibility; };
RaytracingAccelerationStructure shadowScene : register(t0, space7);
StructuredBuffer<Task> shadowTasks : register(t1, space7);
RWStructuredBuffer<Result> shadowResults : register(u0, space7);

bool ValidTask(Task task) {
    float normalLength = dot(task.normal, task.normal);
    return all(isfinite(task.receiver)) && all(isfinite(task.normal)) &&
        all(isfinite(task.lightCenter)) && all(isfinite(task.emitterAxisU)) &&
        all(isfinite(task.emitterAxisV)) && isfinite(normalLength) &&
        normalLength > 0.5f && normalLength < 1.5f &&
        isfinite(task.originBias) && task.originBias >= 0 &&
        isfinite(task.endpointBias) && task.endpointBias >= 0 &&
        task.sampleCount > 0 && task.sampleCount <= 64 &&
        task.instanceMask > 0 && task.instanceMask <= 255;
}

[shader("closesthit")]
void rtSpotBlocked(inout HitData payload, BuiltInTriangleIntersectionAttributes attributes) {
    payload.visibility = 0;
}
[shader("miss")]
void rtSpotVisible(inout HitData payload) { payload.visibility = 1; }

[shader("raygeneration")]
void rtSpotVisibility() {
    uint index = DispatchRaysIndex().x;
    uint count, stride, outputCount, outputStride;
    shadowTasks.GetDimensions(count, stride);
    shadowResults.GetDimensions(outputCount, outputStride);
    if (index >= outputCount) return;
    Result result; result.visibility = 0; result.valid = 0;
    shadowResults[index] = result;
    if (index >= count || stride != 80 || outputStride != 8) return;
    Task task = shadowTasks[index];
    if (!ValidTask(task)) return;
    float3 normal = normalize(task.normal);
    // Offset toward the light-facing side; no guessed scene-wide bias constant.
    if (dot(normal, task.lightCenter-task.receiver) < 0) normal = -normal;
    float3 origin = task.receiver + normal * task.originBias;
    float visible = 0;
    for (uint sampleIndex = 0; sampleIndex < task.sampleCount; ++sampleIndex) {
        // Deterministic equal-area disk samples. No screen/frame random seed.
        float radius = task.sampleCount == 1 ? 0 : sqrt((float(sampleIndex) + 0.5f) / float(task.sampleCount));
        float angle = float(sampleIndex) * 2.399963229728653f;
        float3 target = task.lightCenter + radius *
            (cos(angle)*task.emitterAxisU + sin(angle)*task.emitterAxisV);
        float3 delta = target-origin;
        float distance = length(delta);
        if (!isfinite(distance) || distance <= task.endpointBias) return;
        RayDesc ray;
        ray.Origin = origin; ray.Direction = delta/distance;
        ray.TMin = 0; ray.TMax = distance-task.endpointBias;
        HitData payload; payload.visibility = 0;
        // The future adapter must build an SBT with one hit record per geometry
        // and the custom miss at slot zero. Alpha tests stay enabled.
        TraceRay(shadowScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                 task.instanceMask, 0, 1, 0, ray, payload);
        visible += float(payload.visibility == 1);
    }
    result.visibility = visible / float(task.sampleCount);
    result.valid = 1;
    shadowResults[index] = result;
}
