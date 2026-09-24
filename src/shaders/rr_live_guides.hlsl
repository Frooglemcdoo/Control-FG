// r21p: full-frame Control GBuffer -> RR guide conversion, matching the
// guide architecture recovered from the supplied RenoDX RR addon.
// One deterministic pass covers every material pixel; no material-family replay.
#include "ControlPrimaryGuideDecode.hlsli"

Texture2D<float4> GBuffer1 : register(t0);
Texture2D<float4> GBuffer2 : register(t1);
StructuredBuffer<uint2> MaterialDataPart1 : register(t2);
Texture2D<float4> EnvBRDF : register(t3);
SamplerState LinearClamp : register(s0);

RWTexture2D<float4> NormalRoughness : register(u0);
RWTexture2D<float4> DiffuseGuide : register(u1);
RWTexture2D<float4> SpecularGuide : register(u2);

cbuffer GuideConstants : register(b0) {
    uint InvertSmoothness; // Control: 1 => roughness = 1 - GBuffer1.B
    uint GuideFlags;       // bit0 material table/GBuffer2, bit1 EnvBRDF
    uint Width;
    uint Height;
};

float3 DecodeMaterialValue(uint packed, float gbuffer2Green, out uint brdfMode) {
    float3 packedColor = float3((packed >> 24u) & 255u,
                                (packed >> 16u) & 255u,
                                (packed >> 8u) & 255u) / 255.0f;
    brdfMode = packed & 255u;
    float luminance = dot(packedColor, float3(0.2126f, 0.7152f, 0.0722f));
    float3 tint = lerp(1.0f.xxx,
                       packedColor / (luminance + 1.0e-6f),
                       min(luminance * 10000.0f, 1.0f));
    return brdfMode != 0u ? packedColor : gbuffer2Green * tint;
}

[numthreads(16,16,1)]
void GenerateLiveGuides(uint3 tid : SV_DispatchThreadID) {
    if (tid.x >= Width || tid.y >= Height) return;

    const float4 g1 = GBuffer1.Load(int3(tid.xy, 0));
    const float3 normalView = ControlDecodeNormalView(g1);
    const float roughness = InvertSmoothness != 0u ? 1.0f - g1.b : g1.b;

    // RenoDX's recovered guide shader writes view-space normals directly.
    NormalRoughness[tid.xy] = float4(normalView, roughness);

    float3 materialGuide = 0.0f.xxx;
    float3 specularGuide = 0.0f.xxx;

    if ((GuideFlags & 1u) != 0u) {
        const float4 g2 = GBuffer2.Load(int3(tid.xy, 0));
        const uint materialID = ((uint)(g2.b * 255.0f) << 8u) |
                                (uint)(g2.a * 255.0f);
        const uint packed = MaterialDataPart1[materialID].x;
        uint brdfMode = 0u;
        materialGuide = DecodeMaterialValue(packed, g2.g, brdfMode);

        if ((GuideFlags & 2u) != 0u) {
            // Exact coordinate convention recovered from the reference pass:
            // abs(viewNormal.z), native Control smoothness (GBuffer1.B).
            const float2 env = EnvBRDF.SampleLevel(
                LinearClamp, float2(min(abs(normalView.z), 1.0f), g1.b), 0.0f).xy;
            specularGuide = materialGuide * env.x + env.y;
            // Control BRDF mode 3 is excluded by the reference pass.
            if (brdfMode == 3u) specularGuide = 0.0f.xxx;
        }
    }

    // The supplied reference addon binds this stable per-material value as its
    // DLSS diffuse-albedo guide. It is intentionally not a replayed texture.
    DiffuseGuide[tid.xy] = float4(materialGuide, 1.0f);
    SpecularGuide[tid.xy] = float4(specularGuide, 1.0f);
}
