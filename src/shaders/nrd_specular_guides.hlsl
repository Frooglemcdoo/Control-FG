#include "NRD.hlsli"
#include "rr_hit_distance_shared.h"
#include "ControlPrimaryGuideDecode.hlsli"
Texture2DArray<uint> MaterialId : register(t0);
Texture2DArray<float4> HitPosition : register(t1);
Texture2D<float> ClipDepth : register(t2);
Texture2D<float4> GBuffer1 : register(t3);
RWTexture2D<float4> NormalRoughness : register(u0);
RWTexture2D<float> ViewZ : register(u1);
RWTexture2D<float> NormalizedHit : register(u2);
RWTexture2D<float4> Throughput : register(u3);
cbuffer Parameters : register(b0) {
 float4 WorldRow0;float4 WorldRow1;float4 WorldRow2;
 uint Width;uint Height;uint Layers;uint Reserved;
 float4 ClipColumn0;float4 ClipColumn1;float4 ClipColumn2;float4 ClipColumn3;
 float2 InvResolution;float2 Padding;
};
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint2 p=id.xy;if(any(p>=uint2(Width,Height)))return;
 NormalRoughness[p]=float4(0,0,1,1);ViewZ[p]=0;NormalizedHit[p]=0;Throughput[p]=0;
 float depth=ClipDepth.Load(int3(p,0));
 float2 uv=(float2(p)+.5)*InvResolution;
 float4 q=(2*uv.x-1)*ClipColumn0+(1-2*uv.y)*ClipColumn1+depth*ClipColumn2+ClipColumn3;
 if(!all(isfinite(q))||q.w==0||!isfinite(depth)||depth<0||depth>1)return;
 float3 origin=q.xyz/q.w;float z=abs(origin.z);
 if(!all(isfinite(origin))||z<=0)return;
 float4 gb=GBuffer1.Load(int3(p,0));
 float3 nv=ControlDecodeNormalView(gb);
 float3 nw=normalize(float3(dot(WorldRow0.xyz,nv),dot(WorldRow1.xyz,nv),dot(WorldRow2.xyz,nv)));
 float roughness=ControlDecodeMaterialRoughness(gb);
 float hit=NRD_FrontEnd_SpecHitDistAveraging_Begin();bool sampled=false;
 // Native adaptive streams terminate at RRHDEndMarker. Never read
 // uninitialized positions for misses or layers beyond the end marker.
 for(uint ray=0;ray<Layers;++ray){
  uint material=MaterialId.Load(int4(p,ray,0));
  if(material==RRHDEndMarker)break;
  if(material>RRHDMissMarker)return;
  float distance=65504;
  if(material<RRHDEndMarker){
   float3 delta=HitPosition.Load(int4(p,ray,0)).xyz-origin;
   distance=length(float3(dot(WorldRow0.xyz,delta),dot(WorldRow1.xyz,delta),dot(WorldRow2.xyz,delta)));
   if(!isfinite(distance)||distance<0)return;
  }
  NRD_FrontEnd_SpecHitDistAveraging_Add(hit,REBLUR_FrontEnd_GetNormHitDist(distance,z,float3(3,.1,20),roughness));sampled=true;
 }
 if(!sampled||!all(isfinite(nw)))return;
 NRD_FrontEnd_SpecHitDistAveraging_End(hit);
 NormalRoughness[p]=float4(nw,roughness);ViewZ[p]=z;NormalizedHit[p]=hit;
 // This boundary precedes native BRDF composition. Do not divide by GI albedo.
 Throughput[p]=1;
}
