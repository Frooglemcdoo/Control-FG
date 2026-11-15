// R438 candidate: run Classify then Gather with UAV barriers between passes.
// Native GI6 t0/t1/t2 and u0; private RGBA32F donor texture at u2.
// No history, ray count, light evaluation or hit distance changes.
#include "ControlPrimaryGuideDecode.hlsli"
Texture2D<float4> GBuffer1 : register(t0);
Texture2D<float4> GBuffer2 : register(t1);
Texture2D<float4> LinearDepth : register(t2);
Texture2DArray<uint4> Coverage : register(t24);
RWTexture2D<float4> RawGI : register(u0);
RWTexture2D<float4> Donors : register(u2);

bool MatchingGrid(uint w,uint h) {
    uint a=0,b=0,c=0,d=0,e=0,f=0,g=0,hc=0,slices=0,i=0,j=0;
    GBuffer1.GetDimensions(a,b); GBuffer2.GetDimensions(c,d);
    LinearDepth.GetDimensions(e,f); Coverage.GetDimensions(g,hc,slices);
    Donors.GetDimensions(i,j);
    return a==w && b==h && c==w && d==h && e==w && f==h
        && g==w && hc==h && slices>0 && i==w && j==h;
}

bool Inside(int2 p, uint2 size) { return all(p >= 0) && all(p < int2(size)); }
bool FiniteColor(float3 c) { return all(isfinite(c)) && all(c >= 0); }
float Luma(float3 c) { return dot(c, float3(.2126,.7152,.0722)); }
struct Surface { float z; float3 n; uint material; uint objectMask; bool valid; };
Surface ReadSurface(int2 p) {
    Surface s=(Surface)0;
    float4 g1 = GBuffer1.Load(int3(p,0));
    float4 g2 = GBuffer2.Load(int3(p,0));
    s.z = LinearDepth.Load(int3(p,0)).x;
    s.n = ControlDecodeNormalView(g1);
    s.material = ControlDecodeMaterialID(g2);
    s.objectMask = ((uint)floor(g1.a*255+.5)) & 1;
    s.valid = Coverage.Load(int4(p,0,0)).x != 0xfffeu
              && isfinite(s.z) && s.z > 0 && all(isfinite(g1)) && all(isfinite(g2))
              && all(isfinite(s.n));
    return s;
}
bool Compatible(Surface donor, Surface receiver) {
    return donor.valid && receiver.valid && donor.material == receiver.material
        && donor.objectMask == receiver.objectMask && dot(donor.n,receiver.n) >= .95
        && abs(donor.z-receiver.z) <= .01*donor.z;
}

[numthreads(8,8,1)]
void Classify(uint3 tid : SV_DispatchThreadID) {
    uint w=0,h=0; RawGI.GetDimensions(w,h); if(any(tid.xy>=uint2(w,h))||!MatchingGrid(w,h))return;
    int2 p=int2(tid.xy); float3 c=RawGI[p].rgb; Surface s=ReadSurface(p);
    Donors[p]=0;
    if(!s.valid || !FiniteColor(c))return;
    Donors[p]=float4(0,0,0,-1); // Valid recipient, no excess yet.
    float l=Luma(c);
    // Fixed lamp diagnostic threshold for this candidate, not a universal
    // scene-independent threshold. Requires gameplay validation before release.
    if(l<=.1)return;
    float values[25]; [unroll] for(uint init=0;init<25;++init)values[init]=0; uint count=0;
    [loop] for(int y=-2;y<=2;++y) [loop] for(int x=-2;x<=2;++x) {
        int2 q=p+int2(x,y); if(!Inside(q,uint2(w,h)))continue;
        float3 qc=RawGI[q].rgb;
        if(!FiniteColor(qc)||!Compatible(s,ReadSurface(q)))continue;
        values[count++]=Luma(qc);
    }
    if(count<=1)return;
    // Small donor-only insertion sort; ordinary pixels return before this work.
    [loop] for(uint i=1;i<count;++i) {
        float v=values[i]; int j=int(i)-1;
        [loop] while(j>=0) { if(values[j]<=v)break; values[j+1]=values[j]; --j; }
        values[j+1]=v;
    }
    float median=(values[(count-1)/2]+values[count/2])*.5;
    float3 excess=c*max(0,1-median/l);
    Donors[p]=float4(excess/float(count),float(count));
}

[numthreads(8,8,1)]
void Gather(uint3 tid : SV_DispatchThreadID) {
    uint w=0,h=0; RawGI.GetDimensions(w,h); if(any(tid.xy>=uint2(w,h))||!MatchingGrid(w,h))return;
    int2 p=int2(tid.xy); float4 own=Donors[p]; if(own.a==0)return;
    Surface receiver=ReadSurface(p); float4 original=RawGI[p];
    float3 result=original.rgb-(own.a>0?own.rgb*own.a:float3(0,0,0));
    [loop] for(int y=-2;y<=2;++y) [loop] for(int x=-2;x<=2;++x) {
        int2 q=p+int2(x,y); if(!Inside(q,uint2(w,h)))continue;
        float4 donor=Donors[q];
        // Normalize at the donor, and test its depth tolerance in the same
        // direction as Classify. Receiver normalization would lose energy.
        if(donor.a>0 && Compatible(ReadSurface(q),receiver)) result+=donor.rgb;
    }
    RawGI[p]=float4(result,original.a);
}
