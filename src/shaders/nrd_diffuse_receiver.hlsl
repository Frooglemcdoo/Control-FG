// Same GI6 binding layout and exact receiver decode as Owned GI. The host must
// supply private u2/u3/u4 textures and keep them with this frame's raw GI.
// This shader is not installed into the live rendering path yet.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "ControlPrimaryGuideDecode.hlsli"
RWTexture2D<float4> ReceiverNormalRoughness : register(u2);
RWTexture2D<float4> ReceiverThroughput : register(u3);
RWTexture2D<float> ReceiverViewZ : register(u4);
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint width,height;ReceiverNormalRoughness.GetDimensions(width,height);
 if(any(id.xy>=uint2(width,height)))return;
 ReceiverNormalRoughness[id.xy]=float4(0,0,1,1);
 ReceiverThroughput[id.xy]=0;ReceiverViewZ[id.xy]=0;
 GIShadingRegisters receiver;
 if(!GIInitializeReceiver(id.xy,receiver))return;
 float3 n=asfloat(receiver.r7.xyz);
 float3 throughput=asfloat(receiver.r3.xyz);
 // r2.xyz/r2.w reconstruction has already multiplied xyz by reciprocal w
 // into r3 earlier; r2 itself retains homogeneous xyz and reciprocal w.
 float viewZ=abs(asfloat(receiver.r2.z)*asfloat(receiver.r2.w));
 float roughness=ControlDecodeMaterialRoughness(g_tGBuffer1.Load(int3(id.xy,0)));
 if(!all(isfinite(n))||abs(dot(n,n)-1)>.01||!all(isfinite(throughput))||
    any(throughput<0)||!isfinite(viewZ)||viewZ<=0||!isfinite(roughness))return;
 // This is GI6's receiver throughput, NOT RR diffuse albedo. Primary
 // specular energy compensation is part of this factor in native GI.
 ReceiverNormalRoughness[id.xy]=float4(normalize(n),saturate(roughness));
 ReceiverThroughput[id.xy]=float4(throughput,1);
 ReceiverViewZ[id.xy]=viewZ;
}
