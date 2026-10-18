// r21p: full-frame Control GBuffer -> RR guide conversion, matching the
// guide architecture recovered from the supplied RenoDX RR addon.
// Character/hair can override diffuse with the current native albedo replay.
#include "ControlPrimaryGuideDecode.hlsli"
#include "rr_character_albedo.hlsli"

Texture2D<float4> GBuffer1 : register(t0);
Texture2D<float4> GBuffer2 : register(t1);
StructuredBuffer<uint2> MaterialDataPart1 : register(t2);
Texture2D<float4> EnvBRDF : register(t3);
Texture2D<float4> TexturedCharacterAlbedo : register(t4);
SamplerState LinearClamp : register(s0);

RWTexture2D<float4> NormalRoughness : register(u0);
RWTexture2D<float4> DiffuseGuide : register(u1);
RWTexture2D<float4> SpecularGuide : register(u2);
RWTexture2D<float> HairResponsivity : register(u3);

cbuffer GuideConstants : register(b0) {
    uint InvertSmoothness; // Control: 1 => roughness = 1 - GBuffer1.B
    uint GuideFlags;       // bit0 material table/GBuffer2, bit1 EnvBRDF, bit2 current albedo replay
    uint Width;
    uint Height;
};

// R7: reduce strand-scale normal variation only inside the audited hair replay.
// Read immutable GBuffer inputs, never neighboring output UAVs. A two-pixel
// radius, matching material and hemisphere gate keep silhouettes and folds.
float3 HairNormalGuide(uint2 pixel, float3 center) {
    const uint material = ControlDecodeMaterialID(GBuffer2.Load(int3(pixel, 0)));
    // R11: choose a shared patch direction before any center-hemisphere gate.
    // Otherwise a stochastic center flip changes the entire accepted neighborhood.
    // Opacity weighting suppresses weak card-edge samples; matching material and
    // the private replay tag keep skin/background out of the estimate.
    float3 patchSum = 0.0f.xxx;
    float patchWeight = 0.0f;
    uint patchCount = 0u;
    [unroll] for (int py = -2; py <= 2; ++py) {
        [unroll] for (int px = -2; px <= 2; ++px) {
            const int2 q = int2(pixel) + int2(px, py);
            if (any(q < 0) || q.x >= (int)Width || q.y >= (int)Height) continue;
            const float4 replay = TexturedCharacterAlbedo.Load(int3(q, 0));
            if (!ControlHairGuideTagged(replay.a) ||
                !ControlCharacterAlbedoValid(replay.r, replay.g, replay.b)) continue;
            if (ControlDecodeMaterialID(GBuffer2.Load(int3(q, 0))) != material) continue;
            const float opacity = ControlHairGuideWeight(replay.a);
            if (opacity <= 0.0f) continue;
            const float weight = (3 - abs(px)) * (3 - abs(py)) * opacity;
            patchSum += ControlDecodeNormalView(GBuffer1.Load(int3(q, 0))) * weight;
            patchWeight += weight;
            ++patchCount;
        }
    }
    const float patchLength = length(patchSum);
    // Reject unsupported or cancelling folds. Preserve R8's established fallback.
    if (patchCount >= 3u && patchLength > 1.0e-5f &&
        patchLength >= 0.5f * patchWeight)
        return patchSum / patchLength;
    // R8: a center-gated bilateral filter preserves isolated flipped normals.
    // Select a local direction only when >= 2/3 of valid 3x3 hair samples
    // agree. Ties, thin unsupported pixels and real folds retain the center.
    float3 directions[9];
    uint count = 0u;
    [unroll] for (int dy = -1; dy <= 1; ++dy) {
        [unroll] for (int dx = -1; dx <= 1; ++dx) {
            const int2 q = int2(pixel) + int2(dx, dy);
            if (any(q < 0) || q.x >= (int)Width || q.y >= (int)Height) continue;
            const float4 replay = TexturedCharacterAlbedo.Load(int3(q, 0));
            if (!ControlHairGuideTagged(replay.a) ||
                !ControlCharacterAlbedoValid(replay.r, replay.g, replay.b)) continue;
            if (ControlDecodeMaterialID(GBuffer2.Load(int3(q, 0))) != material) continue;
            directions[count++] = ControlDecodeNormalView(GBuffer1.Load(int3(q, 0)));
        }
    }
    uint bestSupport = 0u;
    float3 bestDirection = center;
    [loop] for (uint i = 0u; i < count; ++i) {
        uint support = 0u;
        [loop] for (uint j = 0u; j < count; ++j)
            if (dot(directions[i], directions[j]) >= 0.5f) ++support;
        if (support > bestSupport) {
            bestSupport = support;
            bestDirection = directions[i];
        }
    }
    if (bestSupport >= 3u && bestSupport * 3u >= count * 2u &&
        dot(center, bestDirection) < 0.5f)
        center = bestDirection;
    float3 sum = center * 9.0f;
    [unroll] for (int y = -2; y <= 2; ++y) {
        [unroll] for (int x = -2; x <= 2; ++x) {
            if (x == 0 && y == 0) continue;
            const int2 q = int2(pixel) + int2(x, y);
            if (any(q < 0) || q.x >= (int)Width || q.y >= (int)Height) continue;
            const float4 replay = TexturedCharacterAlbedo.Load(int3(q, 0));
            if (!ControlHairGuideTagged(replay.a) ||
                !ControlCharacterAlbedoValid(replay.r, replay.g, replay.b)) continue;
            if (ControlDecodeMaterialID(GBuffer2.Load(int3(q, 0))) != material) continue;
            const float3 n = ControlDecodeNormalView(GBuffer1.Load(int3(q, 0)));
            // Do not average across opposed cards or a sharp surface fold.
            if (dot(center, n) < 0.5f) continue;
            const float weight = (3 - abs(x)) * (3 - abs(y));
            sum += n * weight;
        }
    }
    return normalize(sum);
}

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
    float3 normalView = ControlDecodeNormalView(g1);
    if ((GuideFlags & 5u) == 5u) {
        const float4 hair = TexturedCharacterAlbedo.Load(int3(tid.xy, 0));
        if (ControlHairGuideTagged(hair.a) &&
            ControlCharacterAlbedoValid(hair.r, hair.g, hair.b))
            normalView = HairNormalGuide(tid.xy, normalView);
    }
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

    // The packed material value is F0, not textured diffuse reflectance.
    // Use the selected native albedo technique only where it wrote valid RGB.
    // The replay clears RGB to -1, preserving valid black and avoiding any
    // assumption that the native shader's alpha channel is a coverage mask.
    float3 diffuseGuide = materialGuide;
    float hairResponsivity = 0.0f;
    if ((GuideFlags & 4u) != 0u) {
        const float4 replay = TexturedCharacterAlbedo.Load(int3(tid.xy, 0));
        const float3 albedo = replay.rgb;
        if (ControlCharacterAlbedoValid(albedo.r, albedo.g, albedo.b))
        {
            // Preserve the exact character assignment. Only tagged replacement
            // hair blends with the existing material guide at partial opacity.
            const float hairWeight = ControlHairGuideWeight(replay.a);
            if (ControlHairGuideTagged(replay.a)) {
                // Model F: negative hints reduce flicker. Apply only to exact
                // hair coverage, never dilate onto skin or background pixels.
                uint neighbors = 0u;
                [unroll] for (int y = -1; y <= 1; ++y) {
                    [unroll] for (int x = -1; x <= 1; ++x) {
                        const int2 q = int2(tid.xy) + int2(x,y);
                        if (any(q < 0) || q.x >= (int)Width || q.y >= (int)Height) continue;
                        const float4 a = TexturedCharacterAlbedo.Load(int3(q,0));
                        if (ControlHairGuideTagged(a.a) &&
                            ControlCharacterAlbedoValid(a.r,a.g,a.b)) ++neighbors;
                    }
                }
                const float edge = 1.0f - float(neighbors) / 9.0f;
                hairResponsivity = -0.75f * max(1.0f-hairWeight, edge);
            }
            diffuseGuide = hairWeight == 1.0f ? albedo : lerp(materialGuide, albedo, hairWeight);
        }
    }
    HairResponsivity[tid.xy] = hairResponsivity;
    DiffuseGuide[tid.xy] = float4(diffuseGuide, 1.0f);
    SpecularGuide[tid.xy] = float4(specularGuide, 1.0f);
}
