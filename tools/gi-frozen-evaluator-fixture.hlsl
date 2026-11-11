// Deterministic lighting stub for testing the REAL generated evaluator main.
// Does not stand in for native material/light correctness or gameplay parity.
Texture2D<float4> g_tGBuffer1 : register(t0);
Texture2D<float4> g_tLinearDepth : register(t2);
Texture2D<float4> g_tClipDepth : register(t3);
Texture2DArray<uint4> g_tMaterialId : register(t24);
Texture2DArray<float4> g_tPosition_TexcoordY : register(t26);
Texture2DArray<float4> g_tNormal_TexcoordX : register(t25);
RWTexture2D<float4> g_rwtDiffuseGIColorTarget : register(u0);
RWTexture2D<float4> g_rwtHitInfo : register(u1);
cbuffer Lighting : register(b1) {uint4 giCB1[26];}
cbuffer GI : register(b4) {uint4 giCB4[1];}
struct GIShadingRegisters {uint4 r1,r3,r5,r15;};
struct GISecondaryInput {uint material;uint4 positionUV,normalUV;};
bool GIInitializeReceiver(uint2 pixel,out GIShadingRegisters r){
 r=(GIShadingRegisters)0;r.r1.w=asuint(1.0f);r.r3.xyz=asuint(float3(2,3,4));
 return pixel.x%7!=0;
}
void GIEvaluateSecondary(inout GIShadingRegisters r,GISecondaryInput hit){
 bool miss=hit.material==65535;
 r.r15.xyz=asuint(miss?float3(5,7,11):float3(1,2,3));
 r.r5.w=asuint(asfloat(r.r5.w)+(miss?5.0f:2.5f));
}
