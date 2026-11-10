#pragma once
#include <cstddef>
#include <cstring>
// R417: only captured skin families whose diffuse reflectance is base*tint.
// Detail-bearing skin must keep its original albedo technique.
namespace control_skin_guide {
struct Evidence { std::size_t bytes; const char* sha; };
inline constexpr Evidence originals[] = {
    {30160,"0cd8a4b8144f1add1eac28edb815477ad7e3b42291735aa361e2f8e77b20bb96"}, // skin-basic
    {30120,"cf68e7a8831c24b45c9c0fd54f03cb3c1defd0f99fe0bf68f0771333d888cb66"}, // skin-live
    {34136,"f13bd19019bfd15cd222e74732d07b27736e613c10876314ef12524311dbfe59"}, // skin-live-noise-a
    {34176,"004d3c430bdb795fa3201d0cf737d9ec1206443377a1d746eb47b6ba613e2b4f"}, // skin-live-noise-b
};
inline bool KnownSize(std::size_t bytes) noexcept {
    for (const auto& e : originals) if (e.bytes == bytes) return true;
    return false;
}
inline bool Matches(std::size_t bytes, const char* sha) noexcept {
    if (!sha) return false;
    for (const auto& e : originals)
        if (e.bytes == bytes && std::strcmp(e.sha, sha) == 0) return true;
    return false;
}
}
