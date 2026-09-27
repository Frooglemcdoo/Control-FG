RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> Output : register(u0);

cbuffer PT1Constants : register(b0)
{
    row_major float4x4 ClipToWorld;
    float4 CameraPosTMin;
    uint2 OutputSize;
    uint Mode;
    uint Pad0;
};

float3 ClipPointToWorld(float2 ndc, float z)
{
    float4 p = mul(float4(ndc, z, 1.0), ClipToWorld);
    float invW = abs(p.w) > 1e-7 ? rcp(p.w) : 1.0;
    return p.xyz * invW;
}

float3 IdColor(uint id)
{
    uint x = id * 1664525u + 1013904223u;
    uint y = x * 1664525u + 1013904223u;
    uint z = y * 1664525u + 1013904223u;
    return float3((x & 255u) / 255.0, (y & 255u) / 255.0, (z & 255u) / 255.0);
}

[numthreads(8,8,1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    if (dispatchId.x >= OutputSize.x || dispatchId.y >= OutputSize.y) return;

    float2 uv = (float2(dispatchId.xy) + 0.5) / float2(OutputSize);
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float3 p0 = ClipPointToWorld(ndc, 0.0);
    float3 p1 = ClipPointToWorld(ndc, 1.0);
    float3 camera = CameraPosTMin.xyz;
    float3 farPoint = dot(p1-camera,p1-camera) > dot(p0-camera,p0-camera) ? p1 : p0;
    float3 direction = normalize(farPoint - camera);

    RayDesc ray;
    ray.Origin = camera;
    ray.Direction = direction;
    ray.TMin = max(CameraPosTMin.w, 0.001);
    ray.TMax = 100000.0;

    RayQuery<RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);
    while (q.Proceed()) {}

    bool hit = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
    float3 value = 0.0;
    if (Mode == 1u) {
        value = hit ? float3(1.0,1.0,1.0) : float3(0.0,0.0,0.0);
    } else if (Mode == 2u) {
        float t = hit ? q.CommittedRayT() : 100000.0;
        float v = hit ? (1.0 - exp(-t * 0.02)) : 0.0;
        value = float3(v,v,v);
    } else {
        value = hit ? IdColor(q.CommittedInstanceID()) : float3(0.0,0.0,0.0);
    }
    Output[dispatchId.xy] = float4(value,1.0);
}
