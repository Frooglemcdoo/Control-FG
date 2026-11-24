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
// Return the ordinary or strongly corroborated temporal outcome, or zero to preserve the input.
uint TemporalGI(int2 p,int2 size,uint key,float depth,float3 normal,float3 gi,float3 scene,
                float3 lowGI,float3 lowScene,float3 neighborGI,float3 neighborScene,
                bool sparse,float coverage,out float3 result) {
    result=scene;
    float2 xy=p+CharacterMotion.Load(int3(p,0))*motionScale+jitterDelta;
    if (!all(isfinite(xy)) || any(xy<0) || any(xy>size-1)) return 0;
    int2 base=int2(floor(xy));
    float2 f=frac(xy);
    float weight=0;
    float3 oldGI=0,oldScene=0,peakGI=0,peakScene=0;
    [unroll] for(int y=0;y<=1;++y) [unroll] for(int x=0;x<=1;++x) {
        int2 q=base+int2(x,y);
        if(any(q>=size)) continue;
        float w=(x?f.x:1-f.x)*(y?f.y:1-f.y);
        if(w<=0) continue;
        uint4 h=PreviousHistory.Load(int3(q,0));
        if(h.w!=key || abs(f16tof32(h.z&65535)-depth)>max(depth*.005,1e-6)) continue;
        float2 rg=float2((h.z>>16)&255,h.z>>24)/255.0;
        if(dot(normal,ControlDecodeNormalView(float4(rg,0,0)))<.97) continue;
        float3 g=ReadRGB(h.x),s=ReadRGB(h.y);
        weight+=w;oldGI+=w*g;oldScene+=w*s;
        peakGI=max(peakGI,g);peakScene=max(peakScene,s);
    }
    if(weight<.75) return 0;
    float3 giFloor=max(oldGI/weight,lowGI),sceneFloor=max(oldScene/weight,lowScene);
    float3 giUpper=max(peakGI,neighborGI),sceneUpper=max(peakScene,neighborScene);
    float3 bounded=scene;
    bool corroborated=weight>=.9;
    [unroll] for(uint k=0;k<3;++k) {
        if(scene[k]<=max(1.5*sceneFloor[k],0.0001)) continue;
        if(gi[k]<=max(1.5*giFloor[k],0.001)) return 0;
        float excess=min(scene[k]-sceneFloor[k],scene[k]*(1-giFloor[k]/gi[k]));
        result[k]-=.70*coverage*weight*excess;
        // A fixed percentage leaves an arbitrarily large residual as the ray gets brighter.
        // Bound a high-confidence impulse by the two independently observed signals.
        bool extreme=scene[k]>max(8*sceneUpper[k],0.0008) && gi[k]>max(8*giUpper[k],0.008);
        corroborated=corroborated && extreme;
        float ceiling=max(2*max(sceneUpper[k],0.00005),scene[k]*(2*giUpper[k]/gi[k]));
        bounded[k]=lerp(scene[k],min(scene[k],ceiling),coverage);
    }
    if(corroborated && any(bounded!=scene)){result=bounded;return 7;}
    return !sparse && any(result!=scene)?6:0;
}
