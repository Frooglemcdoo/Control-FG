// Transcription of the previously reviewed Control shader operations.
// Compiled into the embedded G1 diagnostic shader; no NGX guide binding is enabled.
// Source identities, equations, and limits: GUIDE-SEMANTICS.md.
// No diffuse-color substitute, shader entry point, resources, or register bindings
// are supplied here. Callers must establish visibility, current-frame resources,
// correct matrix/sampler bindings, resource states, and material coverage first.
#ifndef CONTROL_PRIMARY_GUIDE_DECODE_HLSLI
#define CONTROL_PRIMARY_GUIDE_DECODE_HLSLI

// gbuffer1 must be an unfiltered/nearest value from the primary UNORM GBuffer1.
// R/G are the high bits; alpha bits 1..3 and 4..7 are the low bits.
// Alpha bit 0 is a separate object mask and does not belong to the normal.
float2 ControlDecodeNormalCoordinates(float4 gbuffer1)
{
    uint3 bytesRGA = (uint3)floor(gbuffer1.rga * 255.0f + 0.5f);
    uint normalX = (bytesRGA.x << 3u) | ((bytesRGA.z >> 1u) & 7u);
    uint normalY = (bytesRGA.y << 4u) | ((bytesRGA.z >> 4u) & 15u);
    return float2(normalX, normalY) / float2(2047.0f, 4095.0f);
}

// Matches the primary DXR ray-generation path: stereographic decode followed
// by normalization. This returns VIEW space, not world space.
float3 ControlDecodeNormalView(float4 gbuffer1)
{
    float2 q = 1.3f * (2.0f * ControlDecodeNormalCoordinates(gbuffer1) - 1.0f);
    float squaredLength = dot(q, q);
    float3 normalView = float3(2.0f * q, squaredLength - 1.0f)
                      / (squaredLength + 1.0f);
    return normalize(normalView);
}

// Pass the CURRENT view-to-world rotation basis exactly as consumed by Control:
// sys_constants legacy registers 6.xyz, 7.xyz, and 8.xyz (byte offsets 96/112/128)
// in the inspected reflection raygen. Separate basis vectors avoid an implicit
// row-major/column-major HLSL matrix convention. No translation is applied.
float3 ControlNormalViewToWorld(float3 normalView,
                               float3 viewToWorldBasisX,
                               float3 viewToWorldBasisY,
                               float3 viewToWorldBasisZ)
{
    return normalize(normalView.x * viewToWorldBasisX
                   + normalView.y * viewToWorldBasisY
                   + normalView.z * viewToWorldBasisZ);
}

// Material roughness. Do not substitute squared BRDF alpha or the lighting-only
// 0.05 roughness floor. Input blue is final GBuffer1 smoothness in [0,1].
float ControlDecodeMaterialRoughness(float4 gbuffer1)
{
    return 1.0f - gbuffer1.b;
}

// Exact operation in the inspected consumers: unsigned conversion after *255.
// gbuffer2 must be unfiltered/nearest. Its B/A encode material ID, not albedo.
uint ControlDecodeMaterialID(float4 gbuffer2)
{
    uint2 highLow = (uint2)(gbuffer2.ba * 255.0f);
    return (highLow.x << 8u) | highLow.y;
}

// packedSpecularAndBRDF is the first uint of MaterialDataPart1[materialID]
// (8-byte element stride; RDEF member vSpecularColor_uBRDF at byte 0).
// This recovers F0 only. F0 is not the final view-dependent RR specular guide.
float3 ControlDecodeMaterialF0(uint packedSpecularAndBRDF,
                              float gbuffer2SpecularLuminance)
{
    uint3 packedRGB = uint3((packedSpecularAndBRDF >> 24u) & 255u,
                           (packedSpecularAndBRDF >> 16u) & 255u,
                           (packedSpecularAndBRDF >> 8u) & 255u);
    float3 materialSpecular = float3(packedRGB) / 255.0f;
    uint brdfMode = packedSpecularAndBRDF & 255u;
    float luminance = dot(materialSpecular, float3(0.2126f, 0.7152f, 0.0722f));
    float3 tint = lerp(float3(1.0f, 1.0f, 1.0f),
                       materialSpecular / (luminance + 1.0e-6f),
                       min(luminance * 10000.0f, 1.0f));
    return brdfMode != 0u ? materialSpecular : gbuffer2SpecularLuminance * tint;
}

// Exact native reflection composition factor for the supplied LUT sample.
// No clamp added: this matches F0 * EnvBRDF.x + EnvBRDF.y in the original.
float3 ControlNativeSpecularReflectivity(float3 materialF0, float2 envBRDF)
{
    return materialF0 * envBRDF.x + envBRDF.y;
}

// Candidate specular albedo using Control's existing integrated BRDF lookup.
// The original LUT is indexed by SMOOTHNESS, not roughness, in coordinate y.
// Caller must bind the real current EnvBRDF SRV and matching sampler; neither
// its texture contents nor a substitute sampler is provided by this helper.
// Requires a valid foreground view position and normal. Special BRDF coverage,
// sky/background handling, and final RR guide validation remain integration work.
float3 ControlSampleNativeSpecularAlbedo(Texture2D<float4> envBRDFTexture,
                                       SamplerState envBRDFSampler,
                                       float3 materialF0,
                                       float3 normalView,
                                       float3 positionView,
                                       float smoothness)
{
    float nDotV = dot(normalView, -normalize(positionView));
    float2 envBRDF = envBRDFTexture.SampleLevel(envBRDFSampler,
                                               float2(nDotV, smoothness), 0.0f).xy;
    return ControlNativeSpecularReflectivity(materialF0, envBRDF);
}

#endif
