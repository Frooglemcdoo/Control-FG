#include "rr_hit_origin_shared.h"
// First RR experiment: use native ray zero as the representative specular ray.
// Never read later layers, including stale layers after adaptive termination.
// Miss/no-ray uses the documented sky-distance sentinel; status stays explicit.
Texture2DArray<uint> NativeMaterialId : register(t0);
Texture2DArray<float4> NativePositionTexcoordY : register(t1);
Texture2D<float> NativeClipDepth : register(t2);
RWTexture2D<float> Distance : register(u0);
RWTexture2D<uint> Status : register(u1);
cbuffer Parameters : register(b0) {
    float4 WorldFromViewRow0;
    float4 WorldFromViewRow1;
    float4 WorldFromViewRow2;
    uint Width; uint Height; uint RayCapacity; uint AdaptiveRayCount;
    float4 ClipToViewColumn0; float4 ClipToViewColumn1;
    float4 ClipToViewColumn2; float4 ClipToViewColumn3;
    float2 NativeInvOutputRes; float2 Reserved;

};
RRHDVector RRHDMake(float3 v) {RRHDVector r;r.x=v.x;r.y=v.y;r.z=v.z;return r;}
[numthreads(8,8,1)]
void main(uint3 pixel : SV_DispatchThreadID) {
    uint dw,dh,sw,sh;Distance.GetDimensions(dw,dh);Status.GetDimensions(sw,sh);
    if(pixel.x>=dw||pixel.y>=dh||pixel.x>=sw||pixel.y>=sh) return;
    Distance[pixel.xy]=65504.0;Status[pixel.xy]=RRHDInvalid;
    uint mw,mh,ml,mm,pw,ph,pl,pm,ow,oh,om;
    NativeMaterialId.GetDimensions(0,mw,mh,ml,mm);
    NativePositionTexcoordY.GetDimensions(0,pw,ph,pl,pm);
    NativeClipDepth.GetDimensions(0,ow,oh,om);
    if(Width==0||Height==0||RayCapacity==0||RayCapacity>32||
       dw!=Width||dh!=Height||sw!=Width||sh!=Height||
       mw!=Width||mh!=Height||pw!=Width||ph!=Height||ow!=Width||oh!=Height||
       ml<RayCapacity||pl<RayCapacity) return;
    uint materialId=NativeMaterialId.Load(int4(pixel.xy,0,0));
    uint kind=RRHDClassify(materialId);
    if(kind!=RRHDHit){Status[pixel.xy]=kind;return;}
    RRHDBasis basis;
    basis.row0=RRHDMake(WorldFromViewRow0.xyz);
    basis.row1=RRHDMake(WorldFromViewRow1.xyz);
    basis.row2=RRHDMake(WorldFromViewRow2.xyz);
    RRHDClipToView projection;
    projection.column0.x=ClipToViewColumn0.x;projection.column0.y=ClipToViewColumn0.y;projection.column0.z=ClipToViewColumn0.z;projection.column0.w=ClipToViewColumn0.w;
    projection.column1.x=ClipToViewColumn1.x;projection.column1.y=ClipToViewColumn1.y;projection.column1.z=ClipToViewColumn1.z;projection.column1.w=ClipToViewColumn1.w;
    projection.column2.x=ClipToViewColumn2.x;projection.column2.y=ClipToViewColumn2.y;projection.column2.z=ClipToViewColumn2.z;projection.column2.w=ClipToViewColumn2.w;
    projection.column3.x=ClipToViewColumn3.x;projection.column3.y=ClipToViewColumn3.y;projection.column3.z=ClipToViewColumn3.z;projection.column3.w=ClipToViewColumn3.w;
    float clipDepth=NativeClipDepth.Load(int3(pixel.xy,0));
    RRHDPrimary primary=RRHDPrimaryFromClip(pixel.x,pixel.y,NativeInvOutputRes.x,NativeInvOutputRes.y,clipDepth,projection);
    if(primary.valid==0){
        // With finite authenticated constants, an infinite far plane at the
        // depth boundary has no finite primary point. Keep the sky sentinel.
        if(clipDepth==0.0||clipDepth==1.0)Status[pixel.xy]=4;
        return;
    }
    RRHDResult r=RRHDFromView(materialId,primary.position,
        RRHDMake(NativePositionTexcoordY.Load(int4(pixel.xy,0,0)).xyz),basis);
    if(r.status==RRHDHit)Distance[pixel.xy]=min(r.distance,65504.0);
    Status[pixel.xy]=r.status;
}
