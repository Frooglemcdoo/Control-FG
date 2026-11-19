// Capture-only PSO. Reads the same native resources and producer weights as Lighting.
// Writes only its private comparison tail: no game UAV, training or guide mutation.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_texture_comparison.hlsl"
#include "gi_evaluator_secondary.generated.hlsl"
#include "gi_frozen_receiver.hlsl"
#include "gi_auto_sample.hlsl"
#include "gi_auto_training.hlsl"
cbuffer Selection:register(b0,space19){GIAutoSelection selection;}
#include "gi_weight_read.hlsl"
#include "gi_pair_write.hlsl"
#include "gi_scene_texture_probe.hlsl"
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(!GIPairEnabled()||any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 GIStoreTextureProbe(id.xy);
 GIPairWrite(id.xy,0,0,0,0);
 GIShadingRegisters a;
 if(giCB4[0].z!=selection.rayCount||(selection.rayCount!=4&&selection.rayCount!=10)||!GIInitializeReceiver(id.xy,a))return;
 // Match the production receiver validity gate as well as its primary decode.
 GIFrozenReceiver source=GIFrozenDecodeReceiver(id.xy,g_tGBuffer1.Load(int3(id.xy,0)),g_tLinearDepth.Load(int3(id.xy,0)).x,g_tClipDepth.Load(int3(id.xy,0)).x,giCB0);
 if(!source.valid)return;
 GIShadingRegisters b=a;a.textureLod=0;b.textureLod=1;
 float3 throughput=asfloat(a.r3.xyz);float depth=asfloat(a.r1.w);
 float3 sumsA=0,sumsB=0;bool validA=true,validB=true;
 [loop]for(uint slice=0;slice<selection.rayCount;++slice){
  int4 address=int4(id.xy,slice,0);GISecondaryInput hit;
  hit.material=g_tMaterialId.Load(address).x;
  hit.positionUV=asuint(g_tPosition_TexcoordY.Load(address));hit.normalUV=asuint(g_tNormal_TexcoordX.Load(address));
  float weight=GIReadWeight(id.xy,slice);
  GIEvaluateSecondary(a,hit);GIEvaluateSecondary(b,hit);
  float3 rgbA=throughput*asfloat(a.r15.xyz),rgbB=throughput*asfloat(b.r15.xyz);
  bool validWeight=isfinite(weight)&&weight>=0&&weight<=(selection.rayCount==4?2.00001:1.66667);
  validA=validA&&validWeight&&all(isfinite(rgbA))&&all(rgbA>=0);
  validB=validB&&validWeight&&all(isfinite(rgbB))&&all(rgbB>=0);
  sumsA+=rgbA*weight;sumsB+=rgbB*weight;
 }
 float3 radianceA=sumsA/float(selection.rayCount),radianceB=sumsB/float(selection.rayCount);
 validA=validA&&all(isfinite(radianceA));validB=validB&&all(isfinite(radianceB));
 float4 fade=asfloat(giCB1[17]);float nearEdge=fade.x-fade.y;
 float width=(fade.y+fade.x)-nearEdge;float t=saturate((depth-nearEdge)*rcp(width));
 float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
 GIPairWrite(id.xy,scale*radianceA,validA?3u:1u,scale*radianceB,validB?3u:1u);
}
