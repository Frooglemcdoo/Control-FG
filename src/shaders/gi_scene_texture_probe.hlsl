#define CONTROL_GI_REGIONAL 1
#include "gi_auto_pixel_audit.hlsl"
#include "gi_texture_probe.hlsl"
void GIStoreTextureProbe(uint2 pixel){
 uint auditIndex=GIAutoAuditIndex(pixel);if(auditIndex==0xffffffff)return;
 uint points=min(trainingWidth,512u)*min(trainingHeight,512u);
 uint pointIndex=(auditIndex-2*trainingCapacity)/2;
 uint redBase=2*trainingCapacity+2*points+(points*(selection.rayCount+1)+1)/2+6*pointIndex;
 GIAutoTraining red=autoTraining[redBase],position=autoTraining[redBase+1],terms=autoTraining[redBase+2];
 uint ray=asuint(position.energy),material=red.reserved1;
 GITextureProbeRecord probe=(GITextureProbeRecord)0;probe.words[0]=uint4(material,0xffffffff,ray,0);
 // Only the actual surface base-color branch and its selected red contributor.
 if(red.energy>0&&terms.energy==2&&material<65534&&ray<selection.rayCount){
  int4 address=int4(pixel,ray,0);uint actual=g_tMaterialId.Load(address).x;
  if(actual==material){
   float2 uv=float2(g_tNormal_TexcoordX.Load(address).w,g_tPosition_TexcoordY.Load(address).w);
   uint index=material*35u;
   probe=GIReadTextureProbe(g_sMaterialTextureArray[NonUniformResourceIndex(index)],g_sLinearWrap,uv,material,index,ray,red.energy);
  }
 }
 uint base=GIPairBase()/32u+trainingWidth*trainingHeight+pointIndex*6u;
 [unroll]for(uint i=0;i<6;++i){
  uint4 a=probe.words[2*i],b=probe.words[2*i+1];GIAutoTraining v;
  v.position.x=asfloat(a.x);v.position.y=asfloat(a.y);v.position.z=asfloat(a.z);v.energy=asfloat(a.w);
  v.valid=b.x;v.version=b.y;v.reserved0=b.z;v.reserved1=b.w;autoTraining[base+i]=v;
 }
}
