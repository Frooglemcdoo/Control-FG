// Recorder fixture only; not a replay of scene lighting.
struct GIShadingRegisters {
 uint4 r5,r15;float3 ogiVolume,ogiPoint,ogiSpot,ogiWorld,ogiNormal;float ogiDistance;uint ogiMask;
};
struct GISecondaryInput {uint material;uint4 positionUV,normalUV;};
#include "../src/shaders/gi_owned_source_recorder.hlsl"
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint2 pixel=uint2(id.x,3);uint base=OGIReserve(pixel,3);if(base==0xffffffffu)return;
 GIShadingRegisters s=(GIShadingRegisters)0;GISecondaryInput hit=(GISecondaryInput)0;
 s.r15=asuint(float4(1,2,3,0));s.r5=asuint(float4(0,0,0,4));
 s.ogiVolume=float3(1,0,0);s.ogiPoint=float3(0,2,0);s.ogiSpot=float3(0,0,3);s.ogiMask=2;
 [unroll] for(uint i=0;i<2;++i) OGIRay(base+i*256,pixel,i,2,s,hit,1,10,0,0,float3(1,2,3)*(i+1));
 OGIFinal(base+512,pixel,2,float3(1,2,3),4,1,10,s.ogiVolume,s.ogiPoint,s.ogiSpot,0,1,float3(2,4,6),0,0);
}
