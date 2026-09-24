#include "rr_hit_distance_shared.h"
// Candidate per-ray distances. No reduction to a single RR guide is claimed.
// Caller must supply same-dispatch resources and the authenticated primary position.
Texture2DArray<uint> NativeMaterialId : register(t0);
Texture2DArray<float4> NativePositionTexcoordY : register(t1);
Texture2D<float4> PrimaryPositionView : register(t2);
RWTexture2DArray<float> Distance : register(u0);
RWTexture2DArray<uint> Status : register(u1);
cbuffer Parameters : register(b0) {
    float4 WorldFromViewRow0;
    float4 WorldFromViewRow1;
    float4 WorldFromViewRow2;
    uint Width; uint Height; uint RayCapacity; uint AdaptiveRayCount;
};
RRHDVector RRHDMake(float3 v) {RRHDVector r;r.x=v.x;r.y=v.y;r.z=v.z;return r;}
[numthreads(8,8,1)]
void main(uint3 pixel : SV_DispatchThreadID) {
    uint dw,dh,dl,sw,sh,sl;Distance.GetDimensions(dw,dh,dl);Status.GetDimensions(sw,sh,sl);
    if(any(pixel>=uint3(dw,dh,dl))||any(pixel>=uint3(sw,sh,sl))) return;
    Distance[pixel]=0;Status[pixel]=RRHDInvalid;
    uint mw,mh,ml,mm,pw,ph,pl,pm,ow,oh,om;
    NativeMaterialId.GetDimensions(0,mw,mh,ml,mm);
    NativePositionTexcoordY.GetDimensions(0,pw,ph,pl,pm);
    PrimaryPositionView.GetDimensions(0,ow,oh,om);
    if(Width==0||Height==0||RayCapacity>32||AdaptiveRayCount>1||
       dw!=Width||dh!=Height||sw!=Width||sh!=Height||sl!=dl||
       mw!=Width||mh!=Height||pw!=Width||ph!=Height||ow!=Width||oh!=Height||
       ml<RayCapacity||pl<RayCapacity||dl<RayCapacity) return;
    if(pixel.z>=RayCapacity) {Status[pixel]=RRHDNoRay;return;}
    // The adaptive writer terminates the material-ID array at the ray count.
    // Never read a stale position or ID after that marker.
    if(AdaptiveRayCount!=0) {
        for(uint layer=0;layer<pixel.z;++layer) {
            uint prior=NativeMaterialId.Load(int4(pixel.xy,layer,0));
            if(prior==RRHDEndMarker){Status[pixel]=RRHDNoRay;return;}
            if(prior>RRHDMissMarker)return;
        }
    }
    uint materialId=NativeMaterialId.Load(int4(pixel,0));
    uint kind=RRHDClassify(materialId);
    if(kind!=RRHDHit){Status[pixel]=kind;return;}
    RRHDBasis basis;
    basis.row0=RRHDMake(WorldFromViewRow0.xyz);
    basis.row1=RRHDMake(WorldFromViewRow1.xyz);
    basis.row2=RRHDMake(WorldFromViewRow2.xyz);
    RRHDResult r=RRHDFromView(materialId,RRHDMake(PrimaryPositionView.Load(int3(pixel.xy,0)).xyz),
        RRHDMake(NativePositionTexcoordY.Load(int4(pixel,0)).xyz),basis);
    Distance[pixel]=r.distance;Status[pixel]=r.status;
}
