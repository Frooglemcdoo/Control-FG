#pragma once
// Exact-build native setRenderTargets skips prepareForProducing on workers.
// Preserve the worker's inherited non-null DSV; never infer its GPU state from
// the shared recording tracker. Primary bindings retain strict state checks.
static bool RRAlbedoNativeTextureState(void* texture, unsigned int expected) noexcept {
    if (!texture) return false;
    auto bytes = static_cast<unsigned char*>(texture);
    if (bytes[0x68] || (*reinterpret_cast<const unsigned int*>(bytes + 0x38) & 0x30u)) return false;
    auto tracker = *reinterpret_cast<unsigned char**>(bytes + 0x50);
    if (!tracker) tracker = bytes + 0x40;
    return *reinterpret_cast<const unsigned int*>(tracker + 0x20) == expected;
}
static bool RRAlbedoNativeDepthBinding(void* texture,void* deviceState,bool worker) noexcept {
    if(!worker) return RRAlbedoNativeTextureState(texture,0x10u);
    if(!texture || !deviceState) return false;
    auto* bytes=static_cast<unsigned char*>(texture);
    if(bytes[0x68] || (*reinterpret_cast<const unsigned int*>(bytes+0x38)&0x30u)) return false;
    const auto descriptor=*reinterpret_cast<const std::uintptr_t*>(bytes+0xA0);
    const auto inherited=*reinterpret_cast<const std::uintptr_t*>(static_cast<unsigned char*>(deviceState)+0x40);
    const auto bound=*reinterpret_cast<void* const*>(static_cast<unsigned char*>(deviceState)+0x9E0);
    return descriptor!=0 && descriptor==inherited && bound==texture;
}
