#ifndef CONTROL_GI_FROZEN_RECEIVER_H
#define CONTROL_GI_FROZEN_RECEIVER_H
#include "../gi_reuse/producer_sampling.h"
#include "../gi_reuse/frozen_guide_training.h"
GIPoint GIFrozenPoint(float3 value){GIPoint p;p.x=value.x;p.y=value.y;p.z=value.z;return p;}
float3 GIFrozenVector(GIPoint value){return float3(value.x,value.y,value.z);}
float4 GIFrozenTransform(float4 value,uint first,uint4 system[71]){
 return value.x*asfloat(system[first])+value.y*asfloat(system[first+1])+
        value.z*asfloat(system[first+2])+value.w*asfloat(system[first+3]);
}
struct GIFrozenReceiver {float3 world,normal,origin;uint valid;};
GIFrozenReceiver GIFrozenDecodeReceiver(uint2 pixel,float4 surface,float depth,float clip,uint4 system[71]){
 GIFrozenReceiver r=(GIFrozenReceiver)0;
 float2 ndc=(float2(pixel)+.5f)*asfloat(system[1].zw)*float2(2,-2)+float2(-1,1);
 float4 h=GIFrozenTransform(float4(ndc,clip,1),22,system);
 uint4 bits=(uint4)floor(surface*255+.5f);
 uint2 packed=(bits.xy<<uint2(3,4))|uint2((bits.w>>1)&7,bits.w>>4);
 float2 xy=(float2(packed)*(2.0f/float2(2047,4095))-1.0f)*1.3f;
 float square=dot(xy,xy);
 float3 local=float3(2*xy,square-1)/(square+1);
 float3 rawNormal=GIFrozenTransform(float4(local,0),6,system).xyz;
 r.world=h.xyz/h.w;
 float3 view=r.world-asfloat(system[54].xyz);
 float n2=dot(rawNormal,rawNormal),v2=dot(view,view);
 if(!all(isfinite(r.world))||!all(isfinite(rawNormal))||!isfinite(depth)||depth<0||
    !isfinite(n2)||n2<=0||!isfinite(v2)||v2<=0)return r;
 r.normal=rawNormal*rsqrt(n2);
 // Preserve the original producer's raw normal in its origin offset.
 r.origin=r.world-depth*.001f*(view*rsqrt(v2))+depth*.002f*rawNormal;
 r.valid=all(isfinite(r.origin))?1u:0u;return r;
}
#endif
