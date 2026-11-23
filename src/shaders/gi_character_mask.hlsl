// The native target is RGBA16F. Small integer limbs preserve all bits of
// rasterized device depth, including reverse-Z values near zero.
float4 Coverage(float depth, float opacity) {
    uint bits=asuint(depth);
    return float4(bits & 2047, (bits >> 11) & 2047, bits >> 22, opacity);
}
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
float4 OpaqueMask(RasterInput input) : SV_Target {
    return Coverage(input.position.z, 1);
}
struct MaterialData {float words[84];};
StructuredBuffer<MaterialData> materials : register(t0,space0);
Texture2D<float4> textures[] : register(t0,space1);
SamplerState materialSampler : register(s19,space1);
cbuffer mid_general : register(b0) {uint materialIndex;};
float4 HairMask(RasterInput input) : SV_Target {
    float opacity=textures[35*materialIndex+26].Sample(materialSampler,input.uv).w*materials[materialIndex].words[16];
    if (!(opacity>0)) discard;
    return Coverage(input.position.z,saturate(opacity));
}
