#include "pt_terminal_gi.hlsli"
#include "pt_terminal_gi_request.hlsli"
RWByteAddressBuffer PTGIMetrics : register(u6);
RWByteAddressBuffer PTGIRadiance : register(u7);
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID){
    // Same replay/probe admission as raygen. Admission zero touches no frame
    // resources; sparse validation dispatches must not read pooled request data.
    if(PTGIMetrics.Load(32)!=1)return;
    uint2 pixel=tid.xy,extent=PTGIParams[3].zw;
    if(PTGIParams[3].y!=1||!PTGIRequestExtent(pixel,extent))return;
    uint bytes;PTGIRadiance.GetDimensions(bytes);if(bytes!=extent.x*extent.y*64)return;
    uint index=pixel.y*extent.x+pixel.x,at=index*32,raw=index*64;
    if(asfloat(PTGIRequestGeometry.Load(at+12))!=1||asfloat(PTGIRadiance.Load(raw+12))!=1)return;
    float3 position=asfloat(PTGIRequestGeometry.Load3(at)),normal=asfloat(PTGIRequestGeometry.Load3(at+16));
    float3 weight=asfloat(PTGIRequestWeight.Load3(at)),fraction=asfloat(PTGIRequestWeight.Load3(at+16));
    if(!all(isfinite(position))||!all(isfinite(normal))||!all(isfinite(weight))||any(weight<0)||
       !all(isfinite(fraction))||any(fraction<0)||any(fraction>1))return;
    float3 tail=weight*PTTerminalGI(position,normal);
    float3 diffuse=asfloat(PTGIRadiance.Load3(raw))+tail*fraction;
    float3 specular=asfloat(PTGIRadiance.Load3(raw+16))+tail*(1-fraction);
    if(!all(isfinite(tail))||any(tail<0)||!all(isfinite(diffuse))||!all(isfinite(specular)))return;
    PTGIRadiance.Store3(raw,asuint(diffuse));PTGIRadiance.Store3(raw+16,asuint(specular));
    if(any(tail>0)&&((pixel.x|pixel.y)&63)==0){uint old;PTGIMetrics.InterlockedAdd(96,1,old);}
}
