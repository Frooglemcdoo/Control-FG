// Native G1: actual GPU decoding of the verified primary G-buffer layout.
// Diagnostic output only; no RR evaluation or background/material coverage claim.
#include "ControlPrimaryGuideDecode.hlsli"
Texture2D<float4> PrimaryGBuffer1 : register(t0);
Texture2D<float4> PrimaryGBuffer2 : register(t1);
RWTexture2D<float4> WorldNormalRoughness : register(u0);
cbuffer GuideCamera : register(b0)
{
    float4 ViewToWorldBasisX;
    float4 ViewToWorldBasisY;
    float4 ViewToWorldBasisZ;
    uint2 GuideSize;
    uint2 Reserved;
};

[numthreads(8, 8, 1)]
void DecodePrimaryGuides(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(dispatchID.xy >= GuideSize)) return;
    float4 g1 = PrimaryGBuffer1.Load(int3(dispatchID.xy, 0));
    float3 viewNormal = ControlDecodeNormalView(g1);
    float3 worldNormal = ControlNormalViewToWorld(viewNormal,
        ViewToWorldBasisX.xyz, ViewToWorldBasisY.xyz, ViewToWorldBasisZ.xyz);
    WorldNormalRoughness[dispatchID.xy] = float4(worldNormal,
        ControlDecodeMaterialRoughness(g1));
}
