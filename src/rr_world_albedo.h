#pragma once
#include "../build/rr_world_albedo_compiled.h"
// Exact bytes establish semantic identity; the normal replay validator still
// proves vertex compatibility, bindings, and the single-target draw contract.
inline int RRWorldAlbedoVariant(const RRAlbedoShader::CapturedPixelShader& shader) noexcept {
    for (unsigned i=0;i<sizeof(kRRWorldVariants)/sizeof(kRRWorldVariants[0]);++i) {
        const auto& variant=kRRWorldVariants[i];
        if(shader.bytes.size()==variant.nativeSize &&
           std::memcmp(shader.bytes.data(),variant.native,variant.nativeSize)==0) return static_cast<int>(i);
    }
    return -1;
}
static constexpr int kRRWorldAlbedoStorageBase=0x6F475040;
static std::atomic<bool> rrWorldAlbedoStorageReady{false};
static bool RRWorldAlbedoRequested() noexcept;
static std::uintptr_t RRWorldAlbedoReplacement(std::uintptr_t selected,int variant) noexcept;
