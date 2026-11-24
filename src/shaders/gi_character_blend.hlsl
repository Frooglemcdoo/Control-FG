// Inputs are same-frame, same-grid linear GI and visible character coverage.
// Dispatch into a separate output so neighboring reads always see original GI.
#include "ControlPrimaryGuideDecode.hlsli"

Texture2D<float4> RawGI : register(t0);
Texture2D<float4> CharacterReplay : register(t1);
Texture2D<float> Depth : register(t2);
Texture2D<float4> G1 : register(t3);
Texture2D<float4> G2 : register(t4);
Texture2D<float4> SceneColor : register(t5);
RWTexture2D<float4> FilteredColor : register(u0);

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

// Clockwise; indices four apart sample opposite sides of the receiver.
static const int2 Directions[8] = {
    int2(1,0), int2(1,1), int2(0,1), int2(-1,1),
    int2(-1,0), int2(-1,-1), int2(0,-1), int2(1,-1)
};

struct Neighbors {
    float highest, second;
    float3 sceneHigh, sceneSecond, giHigh, giSecond;
    uint count;
};

bool AddNeighbor(int2 q, int2 extent, uint material, float z, float3 normal, inout Neighbors n) {
    if (any(q<0) || any(q>=extent)) return false;
    bool drawn;
    if (!(CharacterCoverage(q,drawn)>0.5) || Material(q)!=material) return false;
    float qz=Depth.Load(int3(q,0));
    if (!isfinite(qz) || abs(qz-z)>max(0.005*abs(z),1e-6)) return false;
    if (dot(normal,ControlDecodeNormalView(G1.Load(int3(q,0))))<0.97) return false;
    float3 rgb=RawGI.Load(int3(q,0)).rgb;
    float3 scene=SceneColor.Load(int3(q,0)).rgb;
    if (!all(isfinite(rgb)) || !all(isfinite(scene))) return false;
    n.sceneSecond=max(n.sceneSecond,min(n.sceneHigh,scene));
    n.sceneHigh=max(n.sceneHigh,scene);
    n.giSecond=max(n.giSecond,min(n.giHigh,rgb));
    n.giHigh=max(n.giHigh,rgb);
    float v=Peak(rgb);
    n.second=max(n.second,min(n.highest,v));
    n.highest=max(n.highest,v);
    ++n.count;
    return true;
}

uint ApplyBlend(uint3 tid) {
    uint w,h;
    FilteredColor.GetDimensions(w,h);
    int2 p=int2(tid.xy), extent=int2(w,h);
    if (any(p>=extent)) return 0;
    bool drawn;
    float coverage=CharacterCoverage(p,drawn);
    if (!(coverage>0.5)) return drawn ? 1 : 0;
    float3 center=RawGI.Load(int3(p,0)).rgb;
    float z=Depth.Load(int3(p,0));
    if (!all(isfinite(center)) || !isfinite(z)) return 1;
    float3 normal=ControlDecodeNormalView(G1.Load(int3(p,0)));
    uint material=Material(p);
    Neighbors n=(Neighbors)0;
    uint connected=0;
    [unroll] for (uint i=0; i<8; ++i) {
        if (AddNeighbor(p+Directions[i],extent,material,z,normal,n)) connected |= 1u<<i;
    }

    float peak=Peak(center);
    bool extended=n.count<4;
    if (extended) {
        // Require bracketing inner taps before extending along already visible
        // character pixels. Adding samples only raises either rejection threshold.
        if ((connected & (connected>>4))==0 || peak<=max(3*n.second,0.002) || n.highest>=0.5*peak)
            return 2;
        [loop] for (uint i=0; i<8; ++i) {
            if (connected & (1u<<i)) AddNeighbor(p+2*Directions[i],extent,material,z,normal,n);
        }
        if (n.count<4) return 2;
    }
    uint supported=extended ? 5 : 3;
    float threshold=max(n.second*3.0,0.002);
    if (peak > threshold && n.highest < peak*0.5) {
        // GI and composed scene color have different scales. Estimate the
        // replacement in scene space and require an impulse in both signals.
        float4 pixel=SceneColor.Load(int3(p,0));
        if(!all(isfinite(pixel.rgb))) return supported;
        float3 adjusted=pixel.rgb;
        [unroll] for(uint k=0;k<3;++k) {
            if(center[k] > max(3*n.giSecond[k],0.0001) && n.giHigh[k] < 0.5*center[k] &&
               pixel[k] > max(3*n.sceneSecond[k],0.0001) && n.sceneHigh[k] < 0.5*pixel[k])
                adjusted[k]=lerp(pixel[k],max(n.sceneSecond[k],0.00001),coverage);
        }
        if(all(adjusted==pixel.rgb))return supported;
        FilteredColor[p]=float4(adjusted,pixel.a);
        return supported+1;
    }
    return supported;
}

#ifdef MEASURE_BLEND
RWByteAddressBuffer Counts : register(u1);
groupshared uint localCounts[6];
#endif

[numthreads(8,8,1)]
void main(uint3 tid : SV_DispatchThreadID, uint lane : SV_GroupIndex) {
#ifdef MEASURE_BLEND
    if (lane < 6) localCounts[lane] = 0;
    GroupMemoryBarrierWithGroupSync();
#endif
    uint outcome = ApplyBlend(tid);
#ifdef MEASURE_BLEND
    if (outcome) InterlockedAdd(localCounts[outcome-1], 1);
    GroupMemoryBarrierWithGroupSync();
    if (lane < 6 && localCounts[lane]) Counts.InterlockedAdd(4*lane,localCounts[lane]);
#endif
}
