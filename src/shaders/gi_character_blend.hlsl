// Inputs are same-frame, same-grid linear GI and visible character coverage.
// Dispatch into a separate output so neighboring reads always see original GI.
#include "ControlPrimaryGuideDecode.hlsli"

Texture2D<float4> RawGI : register(t0);
Texture2D<float4> CharacterReplay : register(t1);
Texture2D<float> Depth : register(t2);
Texture2D<float4> G1 : register(t3);
Texture2D<float4> G2 : register(t4);
RWTexture2D<float4> FilteredGI : register(u0);

// Dedicated character replay target cleared to (-1,-1,-1,0).
// The common world-albedo target must never be bound here.
float CharacterCoverage(int2 p, out bool drawn) {
    float4 a = CharacterReplay.Load(int3(p,0));
    drawn = a.r >= 0 && a.a > 0;
    if (!drawn) return 0;
    uint3 limbs = uint3(a.rgb);
    uint rasterDepth = limbs.r | (limbs.g << 11) | (limbs.b << 22);
    float visibleDepth = Depth.Load(int3(p,0));
    if (!(visibleDepth >= 0 && visibleDepth <= 1)) return 0;
    return rasterDepth == asuint(visibleDepth) ? saturate(a.a) : 0;
}

uint Material(int2 p) { return ControlDecodeMaterialID(G2.Load(int3(p,0))); }
float Peak(float3 rgb) { return max(rgb.r,max(rgb.g,rgb.b)); }

// Result advances through drawn, visible, sufficiently supported, and corrected.
uint ApplyBlend(uint3 tid) {
    uint w,h;
    FilteredGI.GetDimensions(w,h);
    int2 p = int2(tid.xy);
    if (any(p >= int2(w,h))) return 0;
    float4 center = RawGI.Load(int3(p,0));
    bool drawn;
    float coverage = CharacterCoverage(p, drawn);
    // The verified R495 bypass has already written RGBR to this target.
    if (!(coverage > 0.5) || !all(isfinite(center.rgb))) return drawn ? 1 : 0;
    float z = Depth.Load(int3(p,0));
    if (!isfinite(z)) return 1;
    float3 normal = ControlDecodeNormalView(G1.Load(int3(p,0)));
    uint material = Material(p);
    float highest = 0, second = 0;
    uint count = 0;
    [unroll] for (int y=-1; y<=1; ++y) {
        [unroll] for (int x=-1; x<=1; ++x) {
            int2 q = p + int2(x,y);
            if ((!x && !y) || any(q<0) || any(q>=int2(w,h))) continue;
            bool neighborDrawn;
            if (!(CharacterCoverage(q, neighborDrawn) > 0.5) || Material(q)!=material) continue;
            float qz = Depth.Load(int3(q,0));
            if (!isfinite(qz) || abs(qz-z)>max(0.005*abs(z),1e-6)) continue;
            if (dot(normal,ControlDecodeNormalView(G1.Load(int3(q,0))))<0.97) continue;
            float3 rgb = RawGI.Load(int3(q,0)).rgb;
            if (!all(isfinite(rgb))) continue;
            float v = Peak(rgb);
            second = max(second,min(highest,v));
            highest = max(highest,v);
            ++count;
        }
    }
    // Keep supported highlights and leave sparse silhouette samples alone.
    if (count < 4) return 2;
    float threshold = max(second*3.0,0.002);
    float peak = Peak(center.rgb);
    if (peak > threshold && highest < peak*0.5) {
        // Bring a confirmed opaque impulse back to the same-surface neighborhood.
        // Fractional hair opacity keeps its proportional correction.
        float limit = max(second, 0.00005);
        float3 corrected = center.rgb * lerp(1.0,limit/peak,coverage);
        FilteredGI[p] = corrected.rgbr;
        return 4;
    }
    return 3;
}

#ifdef MEASURE_BLEND
RWByteAddressBuffer Counts : register(u1);
groupshared uint localCounts[4];
#endif

[numthreads(8,8,1)]
void main(uint3 tid : SV_DispatchThreadID, uint lane : SV_GroupIndex) {
#ifdef MEASURE_BLEND
    if (lane < 4) localCounts[lane] = 0;
    GroupMemoryBarrierWithGroupSync();
#endif
    uint outcome = ApplyBlend(tid);
#ifdef MEASURE_BLEND
    if (outcome) InterlockedAdd(localCounts[outcome-1], 1);
    GroupMemoryBarrierWithGroupSync();
    if (lane < 4 && localCounts[lane]) Counts.InterlockedAdd(4*lane,localCounts[lane]);
#endif
}
