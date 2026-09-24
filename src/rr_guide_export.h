#pragma once
// Include after CameraSnapshot. Dedicated worker only, after the GPU fence.
// Input pointers are borrowed synchronously; camera is copied, never dereferenced.
// No SEH, graphics submission, game-resource access or image-library dependency.
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <locale>
#include <limits>

enum class RRGuideImageSemantic : uint32_t { NativeAlbedoTarget0 = 0, NativeAlbedoTarget1 = 1 };
enum class RRGuideImageFormat : uint32_t { Rgba16Float = 1 };
struct RRGuideExportImageView {
    RRGuideImageSemantic semantic = RRGuideImageSemantic::NativeAlbedoTarget0;
    RRGuideImageFormat format = RRGuideImageFormat::Rgba16Float;
    uint32_t width = 0, height = 0;
    const void* data = nullptr;
    size_t rowPitch = 0;
    uint64_t sourceFrame = 0;
};
struct RRGuideExportShaderView {
    const void* data = nullptr;
    size_t bytes = 0;
    uint64_t sourceFrame = 0;
    uint32_t renderTargetCount = 0; // Declared signature count; zero means unknown.
};
struct RRGuideExportView {
    const void* reflectanceData=nullptr;
    size_t reflectanceBytes=0;
    double reflectanceMaxError=0;
    const void* part1Data=nullptr;
    size_t part1Bytes=0;
    uint32_t width = 0, height = 0;
    const void* gbuffer1 = nullptr;
    size_t gbuffer1RowPitch = 0;
    const void* gbuffer2 = nullptr;
    size_t gbuffer2RowPitch = 0;
    const void* normalRoughness = nullptr;
    size_t normalRoughnessRowPitch = 0;
    uint64_t engineFrame = 0, presentToken = 0;
    CameraSnapshot camera{};
    const RRGuideExportImageView* additionalImages = nullptr;
    size_t additionalImageCount = 0;
    const RRGuideExportShaderView* shaders = nullptr;
    size_t shaderCount = 0;
    uint32_t albedoSourceBatches = 0, albedoAcceptedBatches = 0, albedoReplayedBatches = 0;
    uint32_t albedoDrawRanges = 0, albedoSkippedRanges = 0;
    const char* materialRejectionAuditJson = nullptr;
    size_t materialRejectionAuditBytes = 0;
};

namespace RRGuideExportDetail {
struct File {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit File(const std::wstring& path) noexcept
        : value(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr)) {}
    ~File() noexcept { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    bool Write(const void* data, size_t size) noexcept {
        if (value == INVALID_HANDLE_VALUE) return false;
        const auto* bytes = static_cast<const unsigned char*>(data);
        while (size) {
            const DWORD chunk = static_cast<DWORD>(size > 16777216u ? 16777216u : size);
            DWORD written = 0;
            if (!WriteFile(value, bytes, chunk, &written, nullptr) || written != chunk) return false;
            bytes += chunk; size -= chunk;
        }
        return true;
    }
    bool Finish() noexcept {
        if (value == INVALID_HANDLE_VALUE) return false;
        const HANDLE handle = value; value = INVALID_HANDLE_VALUE;
        return CloseHandle(handle) != FALSE;
    }
};

inline uint64_t HashBytes(const void* data, size_t size) noexcept {
    uint64_t hash = UINT64_C(14695981039346656037);
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= UINT64_C(1099511628211); }
    return hash;
}
inline bool Validate(const RRGuideExportView& v) noexcept {
    if((v.part1Bytes && !v.part1Data) || (!v.part1Bytes && v.part1Data) ||
       v.part1Bytes>524288u || v.part1Bytes%8) return false;
    if (!v.width || !v.height || v.width > 16384 || v.height > 16384 ||
        !v.gbuffer1 || !v.gbuffer2 || !v.normalRoughness) return false;
    const uint64_t pixels = static_cast<uint64_t>(v.width) * v.height;
    if((v.reflectanceBytes && (!v.reflectanceData || v.reflectanceBytes!=pixels*24)) ||
       (!v.reflectanceBytes && v.reflectanceData) || !std::isfinite(v.reflectanceMaxError) || v.reflectanceMaxError<0) return false;
    if (pixels > UINT64_C(8388608) || v.additionalImageCount > 2 || v.shaderCount > 128 ||
        (v.additionalImageCount && !v.additionalImages) || (v.shaderCount && !v.shaders) ||
        (v.materialRejectionAuditBytes && !v.materialRejectionAuditJson) || v.materialRejectionAuditBytes > 8388608u) return false;
    if (v.albedoAcceptedBatches > v.albedoSourceBatches || v.albedoReplayedBatches > v.albedoAcceptedBatches) return false;
    if (!v.additionalImageCount && !v.materialRejectionAuditBytes && (v.albedoSourceBatches || v.albedoAcceptedBatches ||
        v.albedoReplayedBatches || v.albedoDrawRanges || v.albedoSkippedRanges)) return false;
    const size_t rgba8 = static_cast<size_t>(v.width) * 4;
    const size_t rgba32 = static_cast<size_t>(v.width) * 16;
    if (v.gbuffer1RowPitch < rgba8 || v.gbuffer2RowPitch < rgba8 ||
        v.normalRoughnessRowPitch < rgba32) return false;
    const size_t max = (std::numeric_limits<size_t>::max)();
    const size_t last = static_cast<size_t>(v.height - 1);
    if (last && !(v.gbuffer1RowPitch <= (max - rgba8) / last &&
        v.gbuffer2RowPitch <= (max - rgba8) / last &&
        v.normalRoughnessRowPitch <= (max - rgba32) / last)) return false;
    const uint64_t bmpBytes = 54u + ((static_cast<uint64_t>(v.width) * 3u + 3u) & ~UINT64_C(3)) * v.height;
    uint64_t payload = pixels * 24u + bmpBytes * 3u;
    unsigned int seenTargets = 0;
    for (size_t i = 0; i < v.additionalImageCount; ++i) {
        const auto& image = v.additionalImages[i];
        const uint32_t target = static_cast<uint32_t>(image.semantic);
        if (target > 1u || (seenTargets & (1u << target)) != 0u ||
            image.format != RRGuideImageFormat::Rgba16Float || !image.data ||
            image.width != v.width || image.height != v.height || image.sourceFrame != v.engineFrame)
            return false;
        seenTargets |= 1u << target;
        const size_t active = static_cast<size_t>(v.width) * 8u;
        if (image.rowPitch < active || (last && image.rowPitch > (max - active) / last)) return false;
        payload += pixels * 8u + bmpBytes * 2u;
    }
    uint64_t shaderBytes = 0;
    uint64_t shaderHashes[128]{};
    for (size_t i = 0; i < v.shaderCount; ++i) {
        const auto& shader = v.shaders[i];
        if (!shader.data || shader.bytes < 32u || shader.bytes > 262144u ||
            !shader.sourceFrame || shader.renderTargetCount > 8u) return false;
        const auto* bytes = static_cast<const unsigned char*>(shader.data);
        if (std::memcmp(bytes, "DXBC", 4u) != 0) return false;
        const uint32_t declared = static_cast<uint32_t>(bytes[24]) |
            (static_cast<uint32_t>(bytes[25]) << 8u) | (static_cast<uint32_t>(bytes[26]) << 16u) |
            (static_cast<uint32_t>(bytes[27]) << 24u);
        if (declared != shader.bytes) return false;
        shaderBytes += shader.bytes;
        if (shaderBytes > UINT64_C(16777216)) return false;
        shaderHashes[i] = HashBytes(shader.data, shader.bytes);
        for (size_t j = 0; j < i; ++j) {
            if (v.shaders[j].bytes == shader.bytes && shaderHashes[j] == shaderHashes[i] &&
                std::memcmp(v.shaders[j].data, shader.data, shader.bytes) == 0) return false;
        }
    }
    // Reserve 1 MiB for metadata; the collector enforces the same 640 MiB cap.
    return payload + shaderBytes + v.materialRejectionAuditBytes + UINT64_C(1048576) <= UINT64_C(671088640);
}
inline void Put16(unsigned char* p, uint16_t v) noexcept {
    p[0] = static_cast<unsigned char>(v); p[1] = static_cast<unsigned char>(v >> 8);
}
inline void Put32(unsigned char* p, uint32_t v) noexcept {
    for (unsigned int i = 0; i < 4; ++i) p[i] = static_cast<unsigned char>(v >> (8 * i));
}
inline unsigned char Quantize(float v) noexcept {
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 255;
    return static_cast<unsigned char>(std::floor(v * 255.0f + 0.5f));
}
// Exact binary16 storage decode, including signed zero, subnormals and the
// nonfinite classes. Raw image export keeps original half words unchanged.
inline float HalfToFloat(uint16_t value) noexcept {
    const uint32_t sign = static_cast<uint32_t>(value & 0x8000u) << 16u;
    const uint32_t exponent = (value >> 10u) & 31u;
    uint32_t fraction = value & 1023u;
    uint32_t bits = sign;
    if (exponent == 0u) {
        if (fraction != 0u) {
            uint32_t adjusted = 113u;
            while ((fraction & 1024u) == 0u) { fraction <<= 1u; --adjusted; }
            bits |= (adjusted << 23u) | ((fraction & 1023u) << 13u);
        }
    } else if (exponent == 31u) {
        bits |= 0x7F800000u | (fraction << 13u);
    } else {
        bits |= ((exponent + 112u) << 23u) | (fraction << 13u);
    }
    float result = 0.0f;
    static_assert(sizeof(result) == sizeof(bits), "binary32 float storage required");
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
// Diagnostic display transform only. The caller handles nonfinite components
// before conversion. Negative and HDR values remain intact in raw output.
inline float LinearToSrgbPreview(float value) noexcept {
    if (value <= 0.0f) return 0.0f;
    if (value >= 1.0f) return 1.0f;
    return value <= 0.0031308f ? value * 12.92f
        : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
}
inline void ReadHalfPixel(const RRGuideExportImageView& image, uint32_t x, uint32_t y,
                          float out[4]) noexcept {
    const auto* bytes = static_cast<const unsigned char*>(image.data) +
        static_cast<size_t>(y) * image.rowPitch + static_cast<size_t>(x) * 8u;
    for (size_t c = 0; c < 4; ++c) {
        const uint16_t word = static_cast<uint16_t>(static_cast<uint32_t>(bytes[c * 2u]) |
            (static_cast<uint32_t>(bytes[c * 2u + 1u]) << 8u));
        out[c] = HalfToFloat(word);
    }
}
struct HalfStatistics {
    uint64_t pixels = 0, finiteRgbPixels = 0, finiteRgbaPixels = 0;
    uint64_t finite[4]{}, nonfinite[4]{}, belowZero[4]{}, aboveOne[4]{};
    uint64_t alphaZero = 0, alphaOne = 0, alphaNonzero = 0;
    double minimum[4]{}, maximum[4]{};
};
inline HalfStatistics AnalyzeHalf(const RRGuideExportImageView& image) noexcept {
    HalfStatistics s{}; s.pixels = static_cast<uint64_t>(image.width) * image.height;
    for (size_t c = 0; c < 4; ++c) {
        s.minimum[c] = (std::numeric_limits<double>::max)();
        s.maximum[c] = -(std::numeric_limits<double>::max)();
    }
    for (uint32_t y = 0; y < image.height; ++y) for (uint32_t x = 0; x < image.width; ++x) {
        float p[4]; ReadHalfPixel(image, x, y, p);
        bool rgb = true, rgba = true;
        for (size_t c = 0; c < 4; ++c) {
            if (std::isfinite(p[c])) {
                ++s.finite[c];
                if (p[c] < s.minimum[c]) s.minimum[c] = p[c];
                if (p[c] > s.maximum[c]) s.maximum[c] = p[c];
                if (p[c] < 0.0f) ++s.belowZero[c];
                if (p[c] > 1.0f) ++s.aboveOne[c];
            } else {
                ++s.nonfinite[c]; rgba = false;
                if (c < 3) rgb = false;
            }
        }
        if (rgb) ++s.finiteRgbPixels;
        if (rgba) ++s.finiteRgbaPixels;
        if (std::isfinite(p[3])) {
            if (p[3] == 0.0f) ++s.alphaZero; else ++s.alphaNonzero;
            if (p[3] == 1.0f) ++s.alphaOne;
        }
    }
    return s;
}
inline std::string TargetStem(const RRGuideExportImageView& image) {
    return "native-albedo-target" + std::to_string(static_cast<uint32_t>(image.semantic));
}
inline std::string ShaderName(size_t index) {
    char name[48]{};
    std::snprintf(name, sizeof(name), "native-albedo-ps-%03u.dxbc", static_cast<unsigned>(index));
    return name;
}
inline std::wstring WidenAscii(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}
inline bool WriteHalfBmp(const std::wstring& path, const RRGuideExportImageView& image, bool alpha) {
    const uint32_t pitch = (image.width * 3u + 3u) & ~3u;
    const uint32_t size = pitch * image.height;
    unsigned char header[54]{};
    header[0] = 'B'; header[1] = 'M';
    Put32(header + 2, 54u + size); Put32(header + 10, 54);
    Put32(header + 14, 40); Put32(header + 18, image.width);
    Put32(header + 22, 0u - image.height);
    Put16(header + 26, 1); Put16(header + 28, 24); Put32(header + 34, size);
    std::vector<unsigned char> row(pitch, 0);
    File file(path);
    if (!file.Write(header, sizeof(header))) return false;
    for (uint32_t y = 0; y < image.height; ++y) {
        for (uint32_t x = 0; x < image.width; ++x) {
            float p[4]; ReadHalfPixel(image, x, y, p);
            unsigned char r = 0, g = 0, b = 0;
            if (alpha) {
                if (std::isfinite(p[3])) r = g = b = Quantize(p[3]);
                else { r = 255; b = 255; }
            } else if (std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2])) {
                r = Quantize(LinearToSrgbPreview(p[0]));
                g = Quantize(LinearToSrgbPreview(p[1]));
                b = Quantize(LinearToSrgbPreview(p[2]));
            } else { r = 255; b = 255; }
            const size_t offset = static_cast<size_t>(x) * 3u;
            row[offset] = b; row[offset + 1u] = g; row[offset + 2u] = r;
        }
        if (!file.Write(row.data(), row.size())) return false;
    }
    return file.Finish();
}
inline void ReadPixel(const RRGuideExportView& v, uint32_t x, uint32_t y, float out[4]) noexcept {
    const auto* row = static_cast<const unsigned char*>(v.normalRoughness) +
        static_cast<size_t>(y) * v.normalRoughnessRowPitch;
    std::memcpy(out, row + static_cast<size_t>(x) * 16, 16);
}
enum class Preview { WorldNormal, Roughness, MaterialId };
inline bool WriteBmp(const std::wstring& path, const RRGuideExportView& v, Preview kind) {
    const uint32_t pitch = (v.width * 3u + 3u) & ~3u;
    const uint32_t size = pitch * v.height; // bounded to < 2^32 by Validate
    unsigned char header[54]{};
    header[0] = 'B'; header[1] = 'M';
    Put32(header + 2, 54u + size); Put32(header + 10, 54);
    Put32(header + 14, 40); Put32(header + 18, v.width);
    Put32(header + 22, 0u - v.height); // signed negative height: top-down
    Put16(header + 26, 1); Put16(header + 28, 24); Put32(header + 34, size);
    std::vector<unsigned char> row(pitch, 0);
    File file(path);
    if (!file.Write(header, sizeof(header))) return false;
    for (uint32_t y = 0; y < v.height; ++y) {
        for (uint32_t x = 0; x < v.width; ++x) {
            unsigned char r = 0, g = 0, b = 0;
            if (kind == Preview::MaterialId) {
                const auto* p = static_cast<const unsigned char*>(v.gbuffer2) +
                    static_cast<size_t>(y) * v.gbuffer2RowPitch + static_cast<size_t>(x) * 4;
                const uint32_t id = (static_cast<uint32_t>(p[2]) << 8) | p[3];
                // Reversible two-byte key, not material albedo or a background mask.
                r = static_cast<unsigned char>((id & 255u) ^ 0x5Au);
                g = static_cast<unsigned char>((id >> 8) ^ 0xA5u);
                b = static_cast<unsigned char>((id * 73u + (id >> 8) * 151u + 31u) & 255u);
            } else {
                float p[4]; ReadPixel(v, x, y, p);
                if (kind == Preview::WorldNormal) {
                    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
                        r = 255; b = 255;
                    } else {
                        r = Quantize(p[0] * 0.5f + 0.5f);
                        g = Quantize(p[1] * 0.5f + 0.5f);
                        b = Quantize(p[2] * 0.5f + 0.5f);
                    }
                } else if (!std::isfinite(p[3]) || p[3] < 0.0f || p[3] > 1.0f) {
                    r = 255; b = 255;
                } else r = g = b = Quantize(p[3]);
            }
            const size_t index = static_cast<size_t>(x) * 3;
            row[index] = b; row[index + 1] = g; row[index + 2] = r;
        }
        if (!file.Write(row.data(), row.size())) return false;
    }
    return file.Finish();
}
inline bool WriteRaw(const std::wstring& path, const void* data, size_t pitch,
                     size_t active, uint32_t height) noexcept {
    File file(path);
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (uint32_t y = 0; y < height; ++y)
        if (!file.Write(bytes + static_cast<size_t>(y) * pitch, active)) return false;
    return file.Finish();
}
struct Statistics {
    uint64_t pixels = 0, finiteNormal = 0, nonfiniteNormal = 0;
    uint64_t finiteRoughness = 0, nonfiniteRoughness = 0, nonunitNormal = 0, outsideRoughness = 0;
    double minLength = (std::numeric_limits<double>::max)(), maxLength = 0, maxError = 0;
    double minRoughness = (std::numeric_limits<double>::max)();
    double maxRoughness = -(std::numeric_limits<double>::max)();
};
inline Statistics Analyze(const RRGuideExportView& v) noexcept {
    Statistics s{}; s.pixels = static_cast<uint64_t>(v.width) * v.height;
    for (uint32_t y = 0; y < v.height; ++y) {
        for (uint32_t x = 0; x < v.width; ++x) {
            float p[4]; ReadPixel(v, x, y, p);
            if (std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2])) {
                ++s.finiteNormal;
                const double nx = p[0], ny = p[1], nz = p[2];
                const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
                const double error = std::fabs(length - 1.0);
                if (length < s.minLength) s.minLength = length;
                if (length > s.maxLength) s.maxLength = length;
                if (error > s.maxError) s.maxError = error;
                if (error > 0.002) ++s.nonunitNormal;
            } else ++s.nonfiniteNormal;
            if (std::isfinite(p[3])) {
                ++s.finiteRoughness;
                if (p[3] < s.minRoughness) s.minRoughness = p[3];
                if (p[3] > s.maxRoughness) s.maxRoughness = p[3];
                if (p[3] < 0.0f || p[3] > 1.0f) ++s.outsideRoughness;
            } else ++s.nonfiniteRoughness;
        }
    }
    return s;
}
inline void JsonNumber(std::ostringstream& out, double value) {
    if (std::isfinite(value)) out << value; else out << "null";
}
template <size_t N> inline void JsonArray(std::ostringstream& out, const double (&a)[N]) {
    out << '[';
    for (size_t i = 0; i < N; ++i) { if (i) out << ','; JsonNumber(out, a[i]); }
    out << ']';
}
inline void OptionalNumber(std::ostringstream& out, bool present, double value) {
    if (present) JsonNumber(out, value); else out << "null";
}
inline void HalfMetadata(std::ostringstream& out, const RRGuideExportImageView& image) {
    const auto s = AnalyzeHalf(image);
    const auto target = static_cast<uint32_t>(image.semantic);
    const std::string stem = TargetStem(image);
    out << "{\"target_index\":" << target << ",\"semantic\":\"NativeAlbedoTarget" << target
        << "\",\"format\":\"R16G16B16A16_FLOAT\",\"width\":" << image.width
        << ",\"height\":" << image.height << ",\"source_frame\":" << image.sourceFrame
        << ",\"raw_file\":\"" << stem << ".rgba16f\",\"raw_bytes\":"
        << static_cast<uint64_t>(image.width) * image.height * 8u
        << ",\"color_preview_file\":\"" << stem << ".bmp\",\"alpha_preview_file\":\""
        << stem << "-alpha.bmp\",\"source_row_pitch_bytes\":" << image.rowPitch
        << ",\"semantics_unverified\":true,\"alpha_interpretation\":\"unspecified\","
        << "\"color_preview_transfer\":\"Linear RGB clamped to [0,1], then sRGB; nonfinite RGB is magenta. Raw values remain unchanged.\","
        << "\"alpha_preview_transfer\":\"Linear alpha clamped to [0,1]; nonfinite alpha is magenta. This is not a coverage mask.\","
        << "\"statistics\":{\"pixels\":" << s.pixels << ",\"finite_rgb_pixels\":" << s.finiteRgbPixels
        << ",\"finite_rgba_pixels\":" << s.finiteRgbaPixels
        << ",\"finite_alpha_zero_pixels\":" << s.alphaZero
        << ",\"finite_alpha_one_pixels\":" << s.alphaOne
        << ",\"finite_alpha_nonzero_pixels\":" << s.alphaNonzero
        << ",\"channels\":{";
    const char* names[4] = {"r", "g", "b", "a"};
    for (size_t c = 0; c < 4; ++c) {
        if (c) out << ',';
        out << '\"' << names[c] << "\":{\"finite\":" << s.finite[c]
            << ",\"nonfinite\":" << s.nonfinite[c] << ",\"below_zero\":" << s.belowZero[c]
            << ",\"above_one\":" << s.aboveOne[c] << ",\"minimum\":";
        OptionalNumber(out, s.finite[c] != 0, s.minimum[c]);
        out << ",\"maximum\":"; OptionalNumber(out, s.finite[c] != 0, s.maximum[c]); out << '}';
    }
    out << "}}}";
}
inline void AdditionalMetadata(std::ostringstream& out, const RRGuideExportView& v) {
    const size_t pixels=size_t(v.width)*v.height;
    out << "  \"gpu_reflectance_candidates\":" << (v.reflectanceBytes?"true":"false") << ",\n";
    if(v.reflectanceBytes) {
        const auto* bytes=static_cast<const unsigned char*>(v.reflectanceData);
        std::ostringstream diffuseHash,specularHash;
        diffuseHash << std::hex << std::setfill('0') << std::setw(16) << HashBytes(bytes,pixels*8);
        specularHash << std::hex << std::setfill('0') << std::setw(16) << HashBytes(bytes+pixels*8,pixels*16);
        out << "  \"reflectance_source_frame\":" << v.engineFrame << ",\n"
            << "  \"reflectance_diffuse_bytes\":" << pixels*8 << ",\n"
            << "  \"reflectance_specular_bytes\":" << pixels*16 << ",\n"
            << "  \"reflectance_diffuse_fnv1a64\":\"" << diffuseHash.str() << "\",\n"
            << "  \"reflectance_specular_fnv1a64\":\"" << specularHash.str() << "\",\n"
            << "  \"reflectance_max_absolute_cpu_error\":" << v.reflectanceMaxError << ",\n"
            << "  \"reflectance_view_convention\":\"unjittered_symmetric_perspective_pixel_centers\",\n"
            << "  \"reflectance_coverage_validated\":false,\n"
            << "  \"reflectance_rr_validated\":false,\n";
    }
    std::ostringstream part1Hash;
    if(v.part1Bytes) part1Hash << std::hex << std::setfill('0') << std::setw(16) << HashBytes(v.part1Data,v.part1Bytes);
    out << "  \"part1_file\":" << (v.part1Bytes ? "\"material-part1.bin\"" : "null")
        << ",\n  \"part1_fnv1a64\":" << (v.part1Bytes ? "\""+part1Hash.str()+"\"" : "null")
        << ",\n  \"part1_bytes\":" << v.part1Bytes << ",\n  \"part1_stride\":8,\n"
        << "  \"part1_records\":" << v.part1Bytes/8 << ",\n"
        << "  \"part1_source_frame\":" << (v.part1Bytes?v.engineFrame:0) << ",\n"
        << "  \"part1_rr_validated\":false,\n";
    out << "  \"material_rejection_audit_file\":" << (v.materialRejectionAuditBytes ? "\"material-rejection-audit.json\"" : "null")
        << ",\n  \"material_rejection_audit_bytes\":" << v.materialRejectionAuditBytes << ",\n"
        << "  \"semantics_unverified\":true,\n"
        << "  \"has_native_albedo_outputs\":" << (v.additionalImageCount ? "true" : "false")
        << ",\n  \"native_albedo_output_count\":" << v.additionalImageCount
        << ",\n  \"native_albedo_capture_counts\":{\"source_batches\":" << v.albedoSourceBatches
        << ",\"accepted_batches\":" << v.albedoAcceptedBatches
        << ",\"replayed_batches\":" << v.albedoReplayedBatches
        << ",\"draw_ranges\":" << v.albedoDrawRanges << ",\"skipped_ranges\":" << v.albedoSkippedRanges << '}'
        << ",\n  \"native_albedo_images\":[";
    for (size_t i = 0; i < v.additionalImageCount; ++i) {
        if (i) out << ',';
        HalfMetadata(out, v.additionalImages[i]);
    }
    out << "],\n  \"native_albedo_shader_count\":" << v.shaderCount
        << ",\n  \"native_albedo_shaders\":[";
    for (size_t i = 0; i < v.shaderCount; ++i) {
        if (i) out << ',';
        const auto& shader = v.shaders[i];
        std::ostringstream hash; hash.imbue(std::locale::classic());
        hash << std::hex << std::setfill('0') << std::setw(16) << HashBytes(shader.data, shader.bytes);
        out << "{\"index\":" << i << ",\"file\":\"" << ShaderName(i)
            << "\",\"bytes\":" << shader.bytes << ",\"fnv1a64\":\"" << hash.str()
            << "\",\"source_frame\":" << shader.sourceFrame
            << ",\"declared_render_target_count\":" << shader.renderTargetCount << '}';
    }
    out << "],\n";
}
inline std::string Metadata(const RRGuideExportView& v, const Statistics& s, const SYSTEMTIME& utc) {
    char time[64]{};
    std::snprintf(time, sizeof(time), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        static_cast<unsigned>(utc.wYear), static_cast<unsigned>(utc.wMonth),
        static_cast<unsigned>(utc.wDay), static_cast<unsigned>(utc.wHour),
        static_cast<unsigned>(utc.wMinute), static_cast<unsigned>(utc.wSecond),
        static_cast<unsigned>(utc.wMilliseconds));
    std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(17);
    out << "{\n  \"schema\":\"ControlFG.RRGuideG3.Capture.v1\",\n"
        << "  \"status\":\"GUIDE_CAPTURE_EXPORTED\",\n  \"exported_utc\":\"" << time << "\",\n"
        << "  \"width\":" << v.width << ",\n  \"height\":" << v.height
        << ",\n  \"engine_frame\":" << v.engineFrame << ",\n  \"present_token\":" << v.presentToken
        << ",\n  \"camera_engine_frame\":" << v.camera.engineFrame
        << ",\n  \"camera_frame_matches_capture\":" << (v.engineFrame == v.camera.engineFrame ? "true" : "false")
        << ",\n  \"background_mask_applied\":false,\n  \"material_coverage_validated\":false,"
        << "\n  \"validated_rr_inputs\":false,\n  \"rr_evaluated\":false,"
        << "\n  \"diffuse_albedo_provided\":false,\n  \"specular_albedo_provided\":false,"
        << "\n  \"hit_distance_provided\":false,\n"
        << "  \"limitations\":\"Unmasked diagnostic; optional native albedo-named targets have unverified semantics and unspecified alpha. Numeric checks do not prove coverage, background validity, camera convention or RR image correctness. No RR inputs are tagged or evaluated.\",\n";
    AdditionalMetadata(out, v);
    out
        << "  \"raw_layout\":{\"row_order\":\"top_to_bottom\",\"byte_order\":\"little_endian\","
        << "\"gbuffer1\":\"gbuffer1.rgba8\",\"gbuffer2\":\"gbuffer2.rgba8\","
        << "\"gbuffer_row_bytes\":" << static_cast<uint64_t>(v.width) * 4
        << ",\"normal_roughness\":\"normal-roughness.rgba32f\",\"normal_roughness_row_bytes\":"
        << static_cast<uint64_t>(v.width) * 16
        << ",\"normal_roughness_channels\":\"world_normal_x,world_normal_y,world_normal_z,linear_roughness\"},\n"
        << "  \"source_row_pitch_bytes\":{\"gbuffer1\":" << v.gbuffer1RowPitch
        << ",\"gbuffer2\":" << v.gbuffer2RowPitch << ",\"normal_roughness\":" << v.normalRoughnessRowPitch << "},\n"
        << "  \"previews\":{\"format\":\"24-bit BGR BMP, top-down, four-byte row padding\","
        << "\"world_normal\":\"RGB=round(clamp(normal*0.5+0.5,0,1)*255); nonfinite normal is magenta\","
        << "\"roughness\":\"Grayscale=round(roughness*255); nonfinite or outside [0,1] is magenta\","
        << "\"material_id\":\"ID=(GBuffer2.B<<8)|GBuffer2.A; R=low^0x5A,G=high^0xA5,B=(ID*73+high*151+31)&255; diagnostic only, not albedo\"},\n"
        << "  \"statistics\":{\"pixels\":" << s.pixels
        << ",\"finite_normal_pixels\":" << s.finiteNormal << ",\"nonfinite_normal_pixels\":" << s.nonfiniteNormal
        << ",\"normal_unit_length_tolerance\":0.002,\"normal_length_outside_tolerance\":" << s.nonunitNormal
        << ",\"normal_length_min\":"; OptionalNumber(out, s.finiteNormal != 0, s.minLength);
    out << ",\"normal_length_max\":"; OptionalNumber(out, s.finiteNormal != 0, s.maxLength);
    out << ",\"max_normal_unit_length_error\":"; OptionalNumber(out, s.finiteNormal != 0, s.maxError);
    out << ",\"finite_roughness_pixels\":" << s.finiteRoughness
        << ",\"nonfinite_roughness_pixels\":" << s.nonfiniteRoughness
        << ",\"roughness_outside_range\":" << s.outsideRoughness << ",\"roughness_min\":";
    OptionalNumber(out, s.finiteRoughness != 0, s.minRoughness);
    out << ",\"roughness_max\":"; OptionalNumber(out, s.finiteRoughness != 0, s.maxRoughness);
    out << "},\n  \"camera\":{\"matrix_layout\":\"raw copied CameraSnapshot array order; nonfinite elements are null\",\"world_to_view\":";
    JsonArray(out, v.camera.worldToView); out << ",\"view_to_world\":"; JsonArray(out, v.camera.viewToWorld);
    out << ",\"view_to_clip\":"; JsonArray(out, v.camera.viewToClip);
    out << ",\"clip_to_view\":"; JsonArray(out, v.camera.clipToView);
    out << ",\"world_to_clip\":"; JsonArray(out, v.camera.worldToClip);
    out << ",\"clip_to_world\":"; JsonArray(out, v.camera.clipToWorld);
    out << "}\n}\n"; return out.str();
}
inline bool IsDirectory(const std::wstring& path) noexcept {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
inline bool Export(const RRGuideExportView& v, wchar_t* output, size_t capacity) {
    if (!Validate(v) || !output || !capacity) return false;
    std::vector<wchar_t> env(32768, 0);
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", env.data(), static_cast<DWORD>(env.size()));
    if (!length || length >= env.size()) return false;
    std::wstring base(env.data(), length);
    while (!base.empty() && (base.back() == L'\\' || base.back() == L'/')) base.pop_back();
    if (base.empty() || !IsDirectory(base)) return false;
    base += L"\\ControlFGProbe";
    if (!CreateDirectoryW(base.c_str(), nullptr) && !IsDirectory(base)) return false;
    SYSTEMTIME utc{}; GetSystemTime(&utc);
    wchar_t leaf[120]{};
    std::swprintf(leaf, sizeof(leaf) / sizeof(leaf[0]), L"\\rr-guide-g3-%04u%02u%02u-%02u%02u%02u-%03u-%lu",
        static_cast<unsigned>(utc.wYear), static_cast<unsigned>(utc.wMonth),
        static_cast<unsigned>(utc.wDay), static_cast<unsigned>(utc.wHour),
        static_cast<unsigned>(utc.wMinute), static_cast<unsigned>(utc.wSecond),
        static_cast<unsigned>(utc.wMilliseconds), static_cast<unsigned long>(GetCurrentProcessId()));
    const std::wstring stem = base + leaf;
    std::wstring directory; bool created = false;
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        directory = stem;
        if (attempt) directory += L"-" + std::to_wstring(attempt);
        if (directory.size() + 1 > capacity || directory.size() + 64 >= 32760) return false;
        if (CreateDirectoryW(directory.c_str(), nullptr)) { created = true; break; }
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    if (!created) return false;
    // A failed export may leave partial data, but metadata.json is committed last.
    if (!WriteRaw(directory + L"\\gbuffer1.rgba8", v.gbuffer1, v.gbuffer1RowPitch, static_cast<size_t>(v.width) * 4, v.height) ||
        !WriteRaw(directory + L"\\gbuffer2.rgba8", v.gbuffer2, v.gbuffer2RowPitch, static_cast<size_t>(v.width) * 4, v.height) ||
        !WriteRaw(directory + L"\\normal-roughness.rgba32f", v.normalRoughness, v.normalRoughnessRowPitch, static_cast<size_t>(v.width) * 16, v.height) ||
        !WriteBmp(directory + L"\\world-normal.bmp", v, Preview::WorldNormal) ||
        !WriteBmp(directory + L"\\roughness.bmp", v, Preview::Roughness) ||
        !WriteBmp(directory + L"\\material-id.bmp", v, Preview::MaterialId)) return false;
    for (size_t i = 0; i < v.additionalImageCount; ++i) {
        const auto& image = v.additionalImages[i];
        const std::wstring imagePathStem = directory + L"\\" + WidenAscii(TargetStem(image));
        if (!WriteRaw(imagePathStem + L".rgba16f", image.data, image.rowPitch,
                      static_cast<size_t>(image.width) * 8u, image.height) ||
            !WriteHalfBmp(imagePathStem + L".bmp", image, false) ||
            !WriteHalfBmp(imagePathStem + L"-alpha.bmp", image, true)) return false;
    }
    for (size_t i = 0; i < v.shaderCount; ++i) {
        const auto& shader = v.shaders[i];
        File file(directory + L"\\" + WidenAscii(ShaderName(i)));
        if (!file.Write(shader.data, shader.bytes) || !file.Finish()) return false;
    }
    if (v.materialRejectionAuditBytes) {
        File audit(directory + L"\\material-rejection-audit.json");
        if (!audit.Write(v.materialRejectionAuditJson, v.materialRejectionAuditBytes) || !audit.Finish()) return false;
    }
    if(v.part1Bytes) {
        File part1(directory+L"\\material-part1.bin");
        if(!part1.Write(v.part1Data,v.part1Bytes) || !part1.Finish()) return false;
    }
    if(v.reflectanceBytes) {
        const size_t pixels=size_t(v.width)*v.height;
        const auto* bytes=static_cast<const unsigned char*>(v.reflectanceData);
        File diffuse(directory+L"\\diffuse-reflectance-candidate.rgba16f");
        File specular(directory+L"\\specular-reflectance-candidate.rgba32f");
        if(!diffuse.Write(bytes,pixels*8) || !diffuse.Finish() ||
           !specular.Write(bytes+pixels*8,pixels*16) || !specular.Finish()) return false;
    }
    const std::string metadata = Metadata(v, Analyze(v), utc);
    if (metadata.size() > 1048576u) return false;
    const std::wstring pendingPath = directory + L"\\metadata.pending";
    File pending(pendingPath);
    if (!pending.Write(metadata.data(), metadata.size()) || !pending.Finish()) return false;
    if (!MoveFileExW(pendingPath.c_str(), (directory + L"\\metadata.json").c_str(), MOVEFILE_WRITE_THROUGH)) return false;
    std::memcpy(output, directory.c_str(), (directory.size() + 1) * sizeof(wchar_t));
    return true;
}
} // namespace RRGuideExportDetail

// True only after all output and metadata.json complete; output is empty on failure.
// Pass an output buffer of 32768 wchar_t elements for the supported path size.
inline bool RRGuideExportCapture(const RRGuideExportView& view, wchar_t* outputDirectory,
                                 size_t outputDirectoryCapacity) noexcept {
    if (outputDirectory && outputDirectoryCapacity) outputDirectory[0] = L'\0';
    try { return RRGuideExportDetail::Export(view, outputDirectory, outputDirectoryCapacity); }
    catch (...) { return false; }
}
