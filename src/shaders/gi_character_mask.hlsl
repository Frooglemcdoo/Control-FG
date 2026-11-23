// The native target is RGBA16F. Small integer limbs preserve all bits of
// rasterized device depth, including reverse-Z values near zero.
float4 Coverage(float depth, float opacity) {
    uint bits=asuint(depth);
    return float4(bits & 2047, (bits >> 11) & 2047, bits >> 22, opacity);
}
float4 OpaqueMask(float4 position : SV_Position) : SV_Target {
    return Coverage(position.z, 1);
}
struct MaterialData {float words[84];};
StructuredBuffer<MaterialData> materials : register(t0,space0);
Texture2D<float4> textures[] : register(t0,space1);
SamplerState materialSampler : register(s19,space1);
cbuffer mid_general : register(b0) {uint materialIndex;};
float4 HairMask(float4 position : SV_Position, float2 uv : TEXCOORD1) : SV_Target {
    float opacity=textures[35*materialIndex+26].Sample(materialSampler,uv).w*materials[materialIndex].words[16];
    if (!(opacity>0)) discard;
    return Coverage(position.z,saturate(opacity));
}
