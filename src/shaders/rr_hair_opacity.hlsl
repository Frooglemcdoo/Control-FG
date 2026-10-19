// Exact captured hair albedo ABI. Skin uses its original native shader.
struct MaterialData { float words[84]; }; // native Part3 stride 336
StructuredBuffer<MaterialData> g_sbMaterialDataPart3 : register(t0, space0);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
SamplerState g_sMaterialArrayGlobal : register(s19, space1);
cbuffer mid_general : register(b0, space0) { uint materialIndex; };
struct Input {
 float4 tc2:TEXCOORD2; float4 tc0:TEXCOORD0;
 float4 normal:NORMAL0; float4 tangent:TANGENT0; float3 binormal:BINORMAL0;
 float2 uv:TEXCOORD1; float2 tc6:TEXCOORD6;
 uint2 instance:RenderInstance0;
};
float4 HairAlbedo(Input input):SV_Target0 {
 const uint baseIndex=materialIndex*35u;
 const MaterialData m=g_sbMaterialDataPart3[materialIndex];
 const float4 tint=float4(m.words[13],m.words[14],m.words[15],m.words[16]);
 // Native hair draw samples .w of slot 26, not the base color's alpha.
 const float opacity=g_sMaterialTextureArray[baseIndex+26u].Sample(g_sMaterialArrayGlobal,input.uv).w*tint.w;
 // Only true holes are rejected. No cutoff is introduced for visible strands.
 if (!(opacity>0.0f)) discard;
 // Private replay metadata: only the exact audited hair replacement writes
 // alpha in [-3,-2]. Native character alpha is not interpreted as coverage.
 const float3 color=g_sMaterialTextureArray[baseIndex].Sample(g_sMaterialArrayGlobal,input.uv).rgb*tint.rgb;
 return float4(color,-2.0f-saturate(opacity));
}
