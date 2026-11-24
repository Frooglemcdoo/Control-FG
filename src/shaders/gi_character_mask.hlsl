#include "gi_character_surface.hlsli"
// Match the audited native VS output register layout for each PS family.
struct RasterInput {
#ifdef HAIR_MASK
    float4 extra:TEXCOORD2;
#endif
    float4 tc:TEXCOORD0;float4 normal:NORMAL0;float4 tangent:TANGENT0;
    float3 binormal:BINORMAL0;float2 uv:TEXCOORD1;float2 tc6:TEXCOORD6;
#ifdef AMBIENT_MASK
    float3 ambient:AMBIENT_LIGHT0;
#endif
    nointerpolation uint2 instance:RenderInstance0;
    float4 position:SV_Position;
};
float4 OpaqueMask(RasterInput input,uint primitive:SV_PrimitiveID) : SV_Target {
    return CharacterSurface(input.position.z,1,input.instance,primitive);
}
struct MaterialData {float words[84];};
StructuredBuffer<MaterialData> materials : register(t0,space0);
Texture2D<float4> textures[] : register(t0,space1);
SamplerState materialSampler : register(s19,space1);
cbuffer mid_general : register(b0) {uint materialIndex;};
float4 HairMask(RasterInput input,uint primitive:SV_PrimitiveID) : SV_Target {
    float opacity=textures[35*materialIndex+26].Sample(materialSampler,input.uv).w*materials[materialIndex].words[16];
    if (!(opacity>0)) discard;
    return CharacterSurface(input.position.z,saturate(opacity),input.instance,primitive);
}
