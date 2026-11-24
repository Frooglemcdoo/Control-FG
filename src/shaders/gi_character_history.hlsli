// Packed arithmetic is used only for the previous frame. Preserve the original
// (unfiltered) GI and color, so a correction cannot feed back into another frame.
Texture2D<float2> CharacterMotion : register(t6);
Texture2D<uint4> PreviousHistory : register(t7);
RWTexture2D<uint4> NextHistory : register(u2);
cbuffer CharacterHistoryParams : register(b0) {
    float2 motionScale;
    float2 jitterDelta;
    uint historyMode; // 0 spatial fallback, 1 seed, 2 use previous and seed current
};

uint PackRGB(float3 c) {
    uint3 h=f32tof16(min(c,65504.0));
    return ((h.r>>4)<<21) | ((h.g>>4)<<10) | (h.b>>5);
}
float3 ReadRGB(uint p) {
    return f16tof32(uint3((p>>21)<<4,((p>>10)&2047)<<4,(p&1023)<<5));
}
uint4 MakeHistory(int2 p,uint key,float3 gi,float3 scene) {
    uint2 n=uint2(round(saturate(G1.Load(int3(p,0)).rg)*255));
    uint geom=f32tof16(Depth.Load(int3(p,0))) | (n.x<<16) | (n.y<<24);
    return uint4(PackRGB(gi),PackRGB(scene),geom,key);
}
bool TemporalGI(int2 p,int2 size,uint key,float depth,float3 normal,float3 gi,float3 scene,
                float3 lowGI,float3 lowScene,float coverage,out float3 result) {
    result=scene;
    float2 xy=p+CharacterMotion.Load(int3(p,0))*motionScale+jitterDelta;
    if (!all(isfinite(xy)) || any(xy<0) || any(xy>size-1)) return false;
    int2 base=int2(floor(xy));
    float2 f=frac(xy);
    float weight=0;
    float3 oldGI=0,oldScene=0;
    [unroll] for(int y=0;y<=1;++y) [unroll] for(int x=0;x<=1;++x) {
        int2 q=base+int2(x,y);
        if(any(q>=size)) continue;
        float w=(x?f.x:1-f.x)*(y?f.y:1-f.y);
        uint4 h=PreviousHistory.Load(int3(q,0));
        if(h.w!=key || abs(f16tof32(h.z&65535)-depth)>max(depth*.005,1e-6)) continue;
        float2 rg=float2((h.z>>16)&255,h.z>>24)/255.0;
        if(dot(normal,ControlDecodeNormalView(float4(rg,0,0)))<.97) continue;
        weight+=w;oldGI+=w*ReadRGB(h.x);oldScene+=w*ReadRGB(h.y);
    }
    if(weight<.75) return false;
    oldGI/=weight;oldScene/=weight;
    float3 giFloor=max(oldGI,lowGI), sceneFloor=max(oldScene,lowScene);
    bool unmatched=false;
    [unroll] for(uint k=0;k<3;++k) {
        bool sceneSpike=scene[k]>max(1.5*sceneFloor[k],0.0001);
        bool giSpike=gi[k]>max(1.5*giFloor[k],0.001);
        unmatched=unmatched || (sceneSpike && !giSpike);
        if(sceneSpike && giSpike) {
            // Only attenuate the simultaneous rise in GI and scene color.
            // Never import a prior bright highlight into a newly dark pixel.
            float excess=min(scene[k]-sceneFloor[k],scene[k]*(1-giFloor[k]/gi[k]));
            result[k]-=0.70*coverage*weight*excess;
        }
    }
    return !unmatched && any(result!=scene);
}
