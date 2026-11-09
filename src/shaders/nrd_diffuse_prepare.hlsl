// Explicit Control -> NRD conversion boundary. Not wired to the live renderer
// until the same-frame camera, material and motion contracts are proven.
#include "NRD.hlsli"
Texture2D<float4> NativeGI : register(t0);
Texture2D<float4> ReceiverNormalRoughness : register(t1);
Texture2D<float> ReceiverViewZ : register(t2);
Texture2D<float2> NativeMotion : register(t3);
Texture2D<float> DiffuseHitDistance : register(t4);
Texture2D<float4> ReceiverThroughput : register(t5);
RWTexture2D<float4> RadianceHitDistance : register(u0);
RWTexture2D<float4> WorldNormalRoughness : register(u1);
RWTexture2D<float> LinearViewDepth : register(u2);
RWTexture2D<float4> SurfaceMotion : register(u3);
RWTexture2D<float4> RemodulationFactor : register(u4);
cbuffer Conversion : register(b0) {
 uint2 Extent;
 float2 MotionScale;
 float2 MotionBias;
 float RadianceScale; // fixed across an accumulation epoch; inverse on output
 float DenoisingRange;
 uint InputContractValid;
};
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID) {
 uint2 p=tid.xy;if(any(p>=Extent))return;
 float3 rgb=NativeGI.Load(int3(p,0)).rgb;
 float4 nr=ReceiverNormalRoughness.Load(int3(p,0));
 float depth=ReceiverViewZ.Load(int3(p,0));
 float2 motion=NativeMotion.Load(int3(p,0));
 float hit=DiffuseHitDistance.Load(int3(p,0));
 float4 receiver=ReceiverThroughput.Load(int3(p,0));
 float3 material=receiver.rgb;
 float3 n=nr.xyz;
 float nl=length(n);
 bool valid=InputContractValid!=0 && all(isfinite(rgb)) && all(rgb>=0) && all(isfinite(nr)) &&
  all(isfinite(motion)) && isfinite(depth) && receiver.a==1 && isfinite(nl) && nl>.5 && nl<1.5 &&
  nr.w>=0 && nr.w<=1 && isfinite(hit) && hit>=0 && RadianceScale>0 && isfinite(RadianceScale);
 valid=valid && depth>0 && depth<DenoisingRange;
 valid=valid && all(isfinite(material)) && all(material>=0);
 if(!valid){
  RadianceHitDistance[p]=0;WorldNormalRoughness[p]=NRD_FrontEnd_PackNormalAndRoughness(float3(0,0,1),1,0);
  LinearViewDepth[p]=DenoisingRange+1;SurfaceMotion[p]=0;RemodulationFactor[p]=0;return;
 }
 // This reversible floor prevents division by zero without throwing away
 // dark-material energy. It must be identical in the output conversion.
 float3 factor=max(material,1.0/1024.0);
 float3 signal=rgb*RadianceScale/factor;
 // NRD uses half precision and squared luminance internally. Reject an
 // unrepresentable input instead of silently clipping away lighting energy.
 if(!all(isfinite(signal))||any(signal>250)){
  RadianceHitDistance[p]=0;WorldNormalRoughness[p]=NRD_FrontEnd_PackNormalAndRoughness(float3(0,0,1),1,0);
  LinearViewDepth[p]=DenoisingRange+1;SurfaceMotion[p]=0;RemodulationFactor[p]=0;return;
 }
 RadianceHitDistance[p]=RELAX_FrontEnd_PackRadianceAndHitDist(signal,hit,true);
 WorldNormalRoughness[p]=NRD_FrontEnd_PackNormalAndRoughness(n/nl,nr.w,0);
 LinearViewDepth[p]=depth;
 SurfaceMotion[p]=float4(motion*MotionScale+MotionBias,0,0);
 RemodulationFactor[p]=float4(factor/RadianceScale,1);
}
