// Generated accounting boundary only; NOT full material/light replay.
float3 ReplayAccumulate(float3 throughput, float3 radiance, float3 sum) {
    uint4 r3=uint4(asuint(throughput),0), r15=uint4(asuint(radiance),0), r10=uint4(asuint(sum),0);
    r10.xyz=(asuint(mad(asfloat(r3.xyzx),asfloat(r15.xyzx),asfloat(r10.xyzx)))).xyz; // native dword 5029
    return asfloat(r10.xyz);
}
float3 ReplayNormalize(float scale, float3 sum, uint count) {
    uint4 r0=asuint(scale).xxxx, r10=uint4(asuint(sum),0), r1=0;
    uint4 giCB4[1]; giCB4[0]=uint4(0,0,count,0);
    r0.xyz=(asuint((asfloat(r0.xxxx)*asfloat(r10.xyzx)))).xyz; // native dword 5136
    r0.w=(asuint(float4(asint(giCB4[0u].zzzz)))).w; // native dword 5143
    r1.xyz=(asuint((asfloat(r0.xyzx)/asfloat(r0.wwww)))).xyz; // native dword 5150
    return asfloat(r1.xyz);
}
// Validation path only: source-pixel GI accounting, no spatial/temporal reuse.
// Throughput is native and already sampling-weighted. Do not divide by PDF.
ByteAddressBuffer Inputs : register(t0);
RWByteAddressBuffer Outputs : register(u0);
cbuffer Dimensions : register(b0) { uint PixelCount; };
static const uint InputStride = 2080; // 16-byte header, 64 x 32-byte rays, expected RGB
static const uint OutputStride = 48;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= PixelCount) return;
    uint p = id.x * InputStride;
    uint o = id.x * OutputStride;
    Outputs.Store4(o, uint4(0, 0, 0, 0));
    Outputs.Store4(o + 16, uint4(0, 0, 0, 0));
    Outputs.Store4(o + 32, uint4(0, 0, 0, 0));
    uint count = Inputs.Load(p);
    float scale = asfloat(Inputs.Load(p + 4));
    if (count == 0 || count > 64 || !isfinite(scale) || scale < 0) return;
    float3 nativeSum = 0, surfaceSum = 0, fallbackSum = 0;
    uint hits = 0, misses = 0;
    for (uint i = 0; i < count; ++i)
    {
        uint a = p + 16 + i * 32;
        float3 throughput = asfloat(Inputs.Load3(a));
        uint material = Inputs.Load(a + 12);
        float3 radiance = asfloat(Inputs.Load3(a + 16));
        if (Inputs.Load(a + 28) != i || (material >= 65534 && material != 65535) ||
            !all(isfinite(throughput)) || !all(isfinite(radiance)) ||
            any(throughput < 0) || any(radiance < 0)) return;
        nativeSum = ReplayAccumulate(throughput, radiance, nativeSum);
        if (material == 65535) {
            fallbackSum = ReplayAccumulate(throughput, radiance, fallbackSum);
            ++misses;
        } else {
            surfaceSum = ReplayAccumulate(throughput, radiance, surfaceSum);
            ++hits;
        }
    }
    // Keep native operation order and denominator, including black and miss rays.
    float3 total = ReplayNormalize(scale, nativeSum, count);
    float3 surface = ReplayNormalize(scale, surfaceSum, count);
    float3 fallback = ReplayNormalize(scale, fallbackSum, count);
    if (!all(isfinite(total)) || !all(isfinite(surface)) || !all(isfinite(fallback))) return;
    Outputs.Store4(o, uint4(asuint(total), 1));
    Outputs.Store4(o + 16, uint4(asuint(surface), hits));
    Outputs.Store4(o + 32, uint4(asuint(fallback), misses));
}
