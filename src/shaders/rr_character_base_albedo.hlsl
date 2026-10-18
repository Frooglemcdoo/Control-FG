// R18: replay the diffuse reflectance used by the captured native character
// lighting shader, without the selected albedo variant's extra detail map.
// This is an RR-only replay. The native skin lighting/material is unchanged.
struct MaterialData { float words[84]; };
StructuredBuffer<MaterialData> g_sbMaterialDataPart3 : register(t0, space0);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
SamplerState g_sMaterialArrayGlobal : register(s19, space1);
cbuffer mid_general : register(b0, space0) { uint materialIndex; };
struct Input {
 float4 tc0:TEXCOORD0; float4 normal:NORMAL0; float4 tangent:TANGENT0;
 float3 binormal:BINORMAL0; float2 uv:TEXCOORD1; float2 tc6:TEXCOORD6;
 float3 ambient:AMBIENT_LIGHT0; uint2 instance:RenderInstance0;
};
float4 CharacterAlbedo(Input input):SV_Target0 {
 const MaterialData m=g_sbMaterialDataPart3[materialIndex];
 const float4 tint=float4(m.words[13],m.words[14],m.words[15],m.words[16]);
 return tint*g_sMaterialTextureArray[materialIndex*35u].Sample(g_sMaterialArrayGlobal,input.uv);
}
