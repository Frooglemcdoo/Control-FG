// Inputs are same-frame, same-grid linear GI and visible character coverage.
// Dispatch into a separate output so neighboring reads always see original GI.
#include "ControlPrimaryGuideDecode.hlsli"
#include "gi_character_surface.hlsli"

Texture2D<float4> RawGI : register(t0);
Texture2D<float4> CharacterReplay : register(t1);
Texture2D<float> Depth : register(t2);
Texture2D<float4> G1 : register(t3);
Texture2D<float4> G2 : register(t4);
Texture2D<float4> SceneColor : register(t5);
Texture2D<float4> BothGI : register(t9);
Texture2D<uint> ReconstructionMap : register(t8);

// For coverage sample zero the native shader selects two low coordinate bits
// per destination using bits 0, 8, 16 or 24 of the tile's reconstruction word.
int2 LightingSource(int2 p) {
    uint word=ReconstructionMap.Load(int3(p>>1,0));
    uint pair=word>>((p.x&1)*8+(p.y&1)*16);
    return (p & ~1) + int2(pair&1,(pair>>1)&1);
}
RWTexture2D<float4> FilteredColor : register(u0);

// Dedicated character replay target cleared to (-1,-1,-1,0).
// The common world-albedo target must never be bound here.
float CharacterCoverage(int2 p, out bool drawn) {
    float4 a = CharacterReplay.Load(int3(p,0));
    drawn = a.a > 0;
    if (!drawn) return 0;
    uint rasterDepth=UnpackSurfaceWord(a.r) | (UnpackSurfaceWord(a.g)<<15);
    float visibleDepth = Depth.Load(int3(p,0));
    if (!(visibleDepth >= 0 && visibleDepth <= 1)) return 0;
    return rasterDepth == asuint(visibleDepth) ? saturate(a.a) : 0;
}

uint Material(int2 p) { return ControlDecodeMaterialID(G2.Load(int3(p,0))); }
float Peak(float3 rgb) { return max(rgb.r,max(rgb.g,rgb.b)); }

#ifdef MEASURE_BLEND
RWByteAddressBuffer Counts : register(u1);
groupshared uint localCounts[8];

// Retain one rejected partial-color edit per measured frame, at input resolution.
// The queue fence retires the readback only after every lane finishes its stores.
void RecordMixedPixel(uint2 p,float3 gi,float3 scene,float3 proposed) {
    uint previous;
    Counts.InterlockedCompareExchange(20,0,1u+p.x+(p.y<<16),previous);
    if (previous==0) {
        Counts.Store3(24,asuint(gi));
        Counts.Store3(36,asuint(scene));
        Counts.Store3(48,asuint(proposed));
    }
}
// Record one retained high-contrast character sample on the existing measured frames.
void RecordRetained(int2 p,uint reason,float3 gi,float3 filtered,float3 scene,float3 neighbors){
    if(!any(scene>max(8*neighbors,.0008)))return;
    uint previous;
    Counts.InterlockedCompareExchange(72,0,1u+uint(p.x)+(uint(p.y)<<16),previous);
    if(previous==0){
        Counts.Store(76,reason);Counts.Store3(80,asuint(gi));Counts.Store3(92,asuint(filtered));
        Counts.Store3(104,asuint(scene));Counts.Store3(116,asuint(neighbors));
    }
}
#define RETAIN_SAMPLE(reason) RecordRetained(p,reason,center.rgb,BothGI.Load(int3(source,0)).rgb,scenePixel.rgb,nearbyScene)
#else
#define RETAIN_SAMPLE(reason)
#endif

#include "gi_character_history.hlsli"

// Outcomes 1..5 are drawn-only, visible-only, supported, corrected and mixed-color veto.
uint ApplyBlend(uint3 tid) {
    uint w,h;
    FilteredColor.GetDimensions(w,h);
    int2 p = int2(tid.xy);
    if (any(p >= int2(w,h))) return 0;
    bool drawn;
    float coverage=CharacterCoverage(p,drawn);
    if(!(coverage>0.5)){
        if(historyMode)NextHistory[p]=0;
        return drawn?1:0;
    }
    int2 source=LightingSource(p);
    bool sourceInBounds=all(source<int2(w,h));
    float4 center=sourceInBounds ? RawGI.Load(int3(source,0)) : 0;

    // The owned output was initialized by a bit-for-bit copy of the NGX color input.
    float4 scenePixel=SceneColor.Load(int3(p,0));
    uint key=0;
    if(historyMode) {
        uint4 record=0;
        if(sourceInBounds && coverage>0.5 && all(isfinite(center.rgb)) && all(isfinite(scenePixel.rgb)) &&
           all(center.rgb>=0) && all(scenePixel.rgb>=0)) {
            key=0x80000000u | (Material(p)<<15) | UnpackSurfaceWord(CharacterReplay.Load(int3(p,0)).b);
            record=MakeHistory(p,key,center.rgb,scenePixel.rgb);
        }
        NextHistory[p]=record;
    }
    if (!sourceInBounds || !(coverage > 0.5) || !all(isfinite(center.rgb))) return drawn ? 1 : 0;
    float z = Depth.Load(int3(p,0));
    if (!isfinite(z)) return 1;
    float3 normal = ControlDecodeNormalView(G1.Load(int3(p,0)));
    uint material = Material(p);
    float highest = 0, second = 0;
    float3 sceneHigh=0, sceneSecond=0, giHigh=0, giSecond=0;
    float3 lowGI=1e30, lowGI2=1e30, lowScene=1e30, lowScene2=1e30;
    uint count = 0;
    uint neighborTag=0;
    // Index distinct source receivers in the at-most 4x4 remapped footprint.
    uint seen=0;
    int2 tileOrigin=(p-1)&~1;
    [loop] for (int y=-1; y<=1; ++y) {
        [loop] for (int x=-1; x<=1; ++x) {
            int2 q = p + int2(x,y);
            if ((!x && !y) || any(q<0) || any(q>=int2(w,h))) continue;
            bool neighborDrawn;
            if (!(CharacterCoverage(q, neighborDrawn) > 0.5) || Material(q)!=material) continue;
            float qz = Depth.Load(int3(q,0));
            if (!isfinite(qz) || abs(qz-z)>max(0.005*abs(z),1e-6)) continue;
            if (dot(normal,ControlDecodeNormalView(G1.Load(int3(q,0))))<0.97) continue;
            int2 tap=LightingSource(q);
            if(any(tap>=int2(w,h)) || all(tap==source)) continue;
            int2 local=tap-tileOrigin;
            uint bit=1u<<(local.y*4+local.x);
            if(seen&bit) continue;
            float3 rgb=RawGI.Load(int3(tap,0)).rgb;
            float3 scene=SceneColor.Load(int3(q,0)).rgb;
            if (!all(isfinite(rgb)) || !all(isfinite(scene))) continue;
            seen|=bit;
            lowGI2=min(lowGI2,max(lowGI,rgb)); lowGI=min(lowGI,rgb);
            lowScene2=min(lowScene2,max(lowScene,scene)); lowScene=min(lowScene,scene);
            sceneSecond=max(sceneSecond,min(sceneHigh,scene));
            sceneHigh=max(sceneHigh,scene);
            giSecond=max(giSecond,min(giHigh,rgb));
            giHigh=max(giHigh,rgb);
            float v = Peak(rgb);
            second = max(second,min(highest,v));
            highest = max(highest,v);
            if (count==0 && key) neighborTag=UnpackSurfaceWord(CharacterReplay.Load(int3(q,0)).b);
            ++count;
        }
    }
    // An isolated character pixel has no independent current lighting evidence.
    // With one neighbor require the same replayed instance/primitive and a strong history match.
    bool sparse=count==1;
    if (!count || (sparse && (historyMode!=2 || !key || neighborTag!=(key&32767u)))) return 2;
    if (sparse) {lowGI2=lowGI;lowScene2=lowScene;}
    if(useFilteredGI){
        // One similarly bright neighbor can be a second sample of the same flash.
        // Admit that case only with independent previous-frame evidence.
        float3 nearbyGI=count>=4?giSecond:giHigh;
        float3 nearbyScene=count>=4?sceneSecond:sceneHigh;
        if(!all(isfinite(scenePixel.rgb)) || any(scenePixel.rgb<0) ||
           !any((scenePixel.rgb>max(3*nearbyScene,0.0003)) & (center.rgb>max(3*nearbyGI,0.001)))){ RETAIN_SAMPLE(1);return 3; }

        CharacterPrior prior=(CharacterPrior)0;
        if(historyMode==2 && key)ProjectCharacterPrior(p,int2(w,h),key,z,normal,prior);
        const bool matched=prior.weight>=.9;
        if(!matched && count<4){ RETAIN_SAMPLE(2);return 2; }
        float3 referenceGI=matched?max(nearbyGI,prior.peakGI):giHigh;
        float3 referenceScene=matched?max(nearbyScene,prior.peakScene):sceneHigh;
        bool3 flash=(scenePixel.rgb>max(3*referenceScene,0.0003)) & (center.rgb>max(3*referenceGI,0.001));
        if(!any(flash)){ RETAIN_SAMPLE(3);return 3; }
        float3 filtered=BothGI.Load(int3(source,0)).rgb;
        if(!all(isfinite(filtered)) || any(filtered<0))return 3;

        // Attenuate the excess toward the same-surface color with one RGB factor.
        // A new directly lit channel with ordinary GI vetoes a partial-color edit.
        float retain=0, historyRetain=0;
        float3 admitted=0;
        bool extreme=matched;
        [unroll] for(uint k=0;k<3;++k){
            if(scenePixel[k]<=max(1.5*referenceScene[k],referenceScene[k]+.00005))continue;
            if(center[k]<=max(1.5*referenceGI[k],referenceGI[k]+.0001)){ RETAIN_SAMPLE(4);return 3; }
            admitted[k]=1;
            retain=max(retain,max(filtered[k],referenceGI[k])/max(center[k],1e-8));
            historyRetain=max(historyRetain,2*referenceGI[k]/max(center[k],1e-8));
            extreme=extreme && scenePixel[k]>max(8*referenceScene[k],.0008) && center[k]>max(8*referenceGI[k],.008);
        }
        bool fromHistory=extreme && historyRetain<retain;
        if(fromHistory)retain=historyRetain;
        if(retain>.5 || (sparse && !extreme)){ RETAIN_SAMPLE(5);return 3; }
        float3 result=scenePixel.rgb-coverage*admitted*(1-saturate(retain))*max(scenePixel.rgb-referenceScene,0);
        if(!any(result!=scenePixel.rgb))return 3;
        FilteredColor[p]=float4(result,scenePixel.a);
        return fromHistory?7:8;
    }
    float threshold = max(second*3.0,0.002);
    float peak = Peak(center.rgb);
    if (count>=4 && peak > threshold && highest < peak*0.5) {
        // GI and composed scene color have different scales. Estimate the
        // replacement in scene space and require an impulse in both signals.
        float4 pixel=scenePixel;
        if(!all(isfinite(pixel.rgb))) return 3;
        float3 adjusted=pixel.rgb;
        bool unmatchedSceneSpike=false;
        [unroll] for(uint k=0;k<3;++k) {
            bool giSpike=center[k] > max(3*giSecond[k],0.0001) && giHigh[k] < 0.5*center[k];
            bool sceneSpike=pixel[k] > max(3*sceneSecond[k],0.0001) && sceneHigh[k] < 0.5*pixel[k];
            unmatchedSceneSpike=unmatchedSceneSpike || (sceneSpike && !giSpike);
            if(giSpike && sceneSpike)
                adjusted[k]=lerp(pixel[k],max(sceneSecond[k],0.00001),coverage);
        }
        if(any(adjusted!=pixel.rgb)) {
        // Reducing only the GI-backed channels of a multichannel highlight can
        // turn a warm or white input into a saturated, single-channel flash.
        if(unmatchedSceneSpike) {
#ifdef MEASURE_BLEND
            RecordMixedPixel(tid.xy,center.rgb,pixel.rgb,adjusted);
#endif
            return 5;
        }
        FilteredColor[p]=float4(adjusted,pixel.a);
        return 4;
        }
    }
    if(historyMode==2 && key && any(center.rgb>max(1.5*lowGI2,0.001)) &&
       any(scenePixel.rgb>max(1.5*lowScene2,0.0001))) {
        float3 adjusted;
        // In a larger footprint a second bright sample can be part of a two-pixel impulse.
        // With only one or two neighbors, every neighbor must support the strong correction.
        float3 neighborGI=count>2?giSecond:giHigh, neighborScene=count>2?sceneSecond:sceneHigh;
        uint outcome=TemporalGI(p,int2(w,h),key,z,normal,center.rgb,scenePixel.rgb,
                                lowGI2,lowScene2,neighborGI,neighborScene,sparse,coverage,adjusted);
        if(outcome){
            FilteredColor[p]=float4(adjusted,scenePixel.a);
            return outcome;
        }
    }
    return count<4 ? 2 : 3;
}

[numthreads(8,8,1)]
void main(uint3 tid : SV_DispatchThreadID, uint lane : SV_GroupIndex) {
#ifdef MEASURE_BLEND
    if (lane < 8) localCounts[lane] = 0;
    GroupMemoryBarrierWithGroupSync();
#endif
    uint outcome = ApplyBlend(tid);
#ifdef MEASURE_BLEND
    if (outcome) InterlockedAdd(localCounts[outcome-1], 1);
    GroupMemoryBarrierWithGroupSync();
    if (lane < 8) {
        uint total=localCounts[lane];
        if (total) Counts.InterlockedAdd(lane>=5?60+4*(lane-5):4*lane,total);
    }
#endif
}
