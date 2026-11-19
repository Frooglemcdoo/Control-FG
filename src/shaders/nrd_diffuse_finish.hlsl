#include "NRD.hlsli"
Texture2D<float4> Denoised : register(t0);
Texture2D<float4> RemodulationFactor : register(t1);
Texture2D<float4> NativeGI : register(t2);
Texture2D<float4> ReceiverNormalRoughness : register(t3);
Texture2D<float> ReceiverViewZ : register(t4);
cbuffer Conversion : register(b0) {
 uint2 Extent;float2 MotionScale;float2 MotionBias;
 float RadianceScale;float DenoisingRange;uint InputContractValid;float Sharpening;
};
float3 Unpack(int2 p) {
#ifdef CONTROL_REBLUR_ONLY
 return REBLUR_BackEnd_UnpackRadianceAndNormHitDist(Denoised.Load(int3(p,0))).rgb;
#else
 return Denoised.Load(int3(p,0)).rgb;
#endif
}
RWTexture2D<float4> NativeOutput : register(u0);
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID){
 uint w,h;NativeOutput.GetDimensions(w,h);if(any(tid.xy>=uint2(w,h)))return;
 int3 p=int3(tid.xy,0);float4 factor=RemodulationFactor.Load(p);
 float3 radiance=Unpack(p.xy);
 // Sharpen radiance before material remodulation, preserving texture/albedo edges.
 // Depth/normal rejection prevents pulling contrast across silhouettes.
 if(Sharpening>0 && factor.a!=0 && all(isfinite(radiance)) && all(radiance>=0)) {
  const float3 luma=float3(.2126,.7152,.0722);
  float center=dot(radiance,luma),sum=0,weight=0;
  float z=ReceiverViewZ.Load(p);float3 normal=ReceiverNormalRoughness.Load(p).xyz;
  const int2 offsets[4]={int2(-1,0),int2(1,0),int2(0,-1),int2(0,1)};
  [unroll]for(uint i=0;i<4;++i){
   int2 q=clamp(p.xy+offsets[i],int2(0,0),int2(w-1,h-1));int3 qp=int3(q,0);
   float3 sample=Unpack(q);float nz=ReceiverViewZ.Load(qp);
   float3 nn=ReceiverNormalRoughness.Load(qp).xyz;
   bool valid=RemodulationFactor.Load(qp).a!=0 && all(isfinite(sample)) && all(sample>=0) &&
    abs(nz-z)<=max(.001,abs(z)*.01) && dot(normal,nn)>.95;
   if(valid){sum+=dot(sample,luma);weight+=1;}
  }
  if(weight>0 && center>1e-6){
   float detail=center-sum/weight;
   float gain=clamp(1+Sharpening*detail/center,.8,1.2);
   radiance*=gain;
  }
 }
 float3 rgb=radiance*factor.rgb;
 // Rejected guides preserve native energy. Never manufacture black holes.
 if(factor.a==0 || !all(isfinite(rgb)) || any(rgb<0))rgb=NativeGI.Load(p).rgb;
 NativeOutput[tid.xy]=float4(rgb,rgb.r);
}
