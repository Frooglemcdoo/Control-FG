#pragma once
// G12 CPU reference primitives for Control's future RR reflectance guides.
// These routines deliberately do not discover or bind resources and do not
// enable RR evaluation. They provide byte-/equation-testable implementation
// building blocks for the later GPU integration.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace control_rr_reflectance {

inline constexpr const char* ImplementationContract = "RR_GUIDE_G12_REFLECTANCE_PRIMITIVES";

struct Float3 { float x, y, z; };

inline bool CopyRGBA16FToDiffuseReflectance(const void* source,
                                            std::size_t sourceRowPitch,
                                            void* destination,
                                            std::size_t destinationRowPitch,
                                            std::uint32_t width,
                                            std::uint32_t height) noexcept {
    if (!width || !height) return true;
    if (!source || !destination) return false;
    const auto maximum = (std::numeric_limits<std::size_t>::max)();
    if (static_cast<std::size_t>(width) > maximum / 8u) return false;
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 8u;
    if (sourceRowPitch < rowBytes || destinationRowPitch < rowBytes) return false;
    if (static_cast<std::size_t>(height - 1u) > (maximum - rowBytes) / sourceRowPitch ||
        static_cast<std::size_t>(height - 1u) > (maximum - rowBytes) / destinationRowPitch) return false;
    // Caller owns buffers of at least (height-1)*pitch+rowBytes bytes.
    // Exact in-place conversion is supported; other overlapping layouts are not.
    if (source == destination && sourceRowPitch != destinationRowPitch) return false;
    const auto* src = static_cast<const std::uint8_t*>(source);
    auto* dst = static_cast<std::uint8_t*>(destination);
    for (std::uint32_t y = 0; y < height; ++y) {
        const auto* s = src + static_cast<std::size_t>(y) * sourceRowPitch;
        auto* d = dst + static_cast<std::size_t>(y) * destinationRowPitch;
        for (std::uint32_t x = 0; x < width; ++x) {
            std::memmove(d + static_cast<std::size_t>(x) * 8u,
                        s + static_cast<std::size_t>(x) * 8u, 6u);
            // IEEE-754 binary16 1.0, little endian: 0x3c00.
            d[static_cast<std::size_t>(x) * 8u + 6u] = 0x00u;
            d[static_cast<std::size_t>(x) * 8u + 7u] = 0x3cu;
        }
    }
    return true;
}

inline Float3 DecodeControlMaterialF0(std::uint32_t packedSpecularAndBRDF,
                                      float gbuffer2SpecularLuminance) noexcept {
    const Float3 material{
        static_cast<float>((packedSpecularAndBRDF >> 24u) & 255u) / 255.0f,
        static_cast<float>((packedSpecularAndBRDF >> 16u) & 255u) / 255.0f,
        static_cast<float>((packedSpecularAndBRDF >> 8u) & 255u) / 255.0f};
    if ((packedSpecularAndBRDF & 255u) != 0u) return material;
    const float luminance = material.x * 0.2126f + material.y * 0.7152f + material.z * 0.0722f;
    const float blend = std::fmin(luminance * 10000.0f, 1.0f);
    const float denominator = luminance + 1.0e-6f;
    const Float3 tint{
        1.0f + (material.x / denominator - 1.0f) * blend,
        1.0f + (material.y / denominator - 1.0f) * blend,
        1.0f + (material.z / denominator - 1.0f) * blend};
    return {gbuffer2SpecularLuminance * tint.x,
            gbuffer2SpecularLuminance * tint.y,
            gbuffer2SpecularLuminance * tint.z};
}

// NVIDIA Streamline ProgrammingGuideDLSS_RR.md section 4.2.1.
// Algebraically expanded dot(mul(M, X), Y). Alpha is BRDF roughness squared,
// NOT the linear roughness stored in the normal/roughness guide.
// Finite f0 >= 0, alpha in [0,1], and nDotV in [-1,1] are caller preconditions.
inline Float3 EnvBRDFApprox2(Float3 f0, float alpha, float nDotV) noexcept {
    const float v = std::fabs(nDotV), v2 = v*v, v3 = v*v2;
    const float a = alpha, a3 = a*a*a;
    const float biasNumerator = (0.99044f - 1.28514f*v) + a*(1.29678f - 0.755907f*v);
    const float biasDenominator = (1.0f + 2.92338f*v + 59.4188f*v3)
        + a*(20.3225f - 27.0302f*v + 222.592f*v3)
        + a3*(121.563f + 626.13f*v + 316.627f*v3);
    const float scaleNumerator = (0.0365463f + 3.32707f*v) + a*(9.0632f - 9.04756f*v);
    const float scaleDenominator = (1.0f + 3.59685f*v2 - 1.36772f*v3)
        + a*(9.04401f - 16.3174f*v2 + 9.22949f*v3)
        + a3*(5.56589f + 19.7886f*v2 - 20.2123f*v3);
    const float zeroWeight = std::fmax(0.0f, std::fmin(1.0f, f0.y * 50.0f));
    const float bias = std::fmax(0.0f, (biasNumerator / biasDenominator) * zeroWeight);
    const float scale = std::fmax(0.0f, scaleNumerator / scaleDenominator);
    return {f0.x * scale + bias, f0.y * scale + bias, f0.z * scale + bias};
}

inline Float3 SpecularReflectanceFromLinearRoughness(Float3 f0, float roughness,
                                                     float nDotV) noexcept {
    return EnvBRDFApprox2(f0, roughness * roughness, nDotV);
}

} // namespace control_rr_reflectance
