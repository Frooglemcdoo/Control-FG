#pragma once

// Included inside native_lighting, after Trace and PatchSlot are defined.
#include "spot_shadow_audit.h"
namespace spot_resolution_runtime {
using Extent = spot_resolution::Extent;
using Resize = void (*)(void*, const Extent*);
using Request = void* (*)(void*, int, int, unsigned, float, unsigned);
using Clear = void (*)(void*);
inline Clear originalClear = nullptr;
inline Request originalVsmRequest = nullptr;
inline Resize originalResize = nullptr;
inline Request originalRequest = nullptr;
inline unsigned char* rendererBase = nullptr;
inline std::atomic<void*> expandedPool{nullptr};
inline std::atomic<int> atlasWidth{0}, atlasHeight{0};
inline std::atomic<uint64_t> requestedSequence{~uint64_t(0)};
inline std::atomic<uint64_t> loggedSequence{~uint64_t(0)};
inline SRWLOCK installLock = SRWLOCK_INIT;

inline void ResizeAt(void* pool, const Extent* native, const unsigned char* caller) {
    Extent expanded{};
    if (caller != rendererBase + 0x1376c8 || !native ||
        !spot_resolution::ExpandAtlas(*native, expanded)) {
        originalResize(pool, native);
        return;
    }
    // The native resize path owns resource retirement, packing and UV updates.
    // Keep this capacity in both modes to avoid reallocating during comparisons.
    originalResize(pool, &expanded);
    atlasWidth.store(expanded.width);
    atlasHeight.store(expanded.height);
    const auto old = expandedPool.exchange(pool);
    readyMask.fetch_or(kSpotResolutionReadyBit);
    if (old != pool)
        Trace("spot_atlas_capacity native=%dx%d requested=%dx%d native_resize_returned=1", native->width, native->height, expanded.width, expanded.height);
}

inline void* RequestAt(void* pool, int width, int height,
                       unsigned dynamic, float distance, unsigned lightId, const unsigned char* caller) {
    const auto state = selection.Read();
    const auto mode = LiveSelection::GetMode(state);
    const bool spotlight = caller == rendererBase + 0x18acba && dynamic == 1 &&
                           pool == expandedPool.load();
    const Extent request = spot_resolution::TileRequest({width, height},
        spotlight && mode == Mode::SpotResolution2x, {atlasWidth.load(), atlasHeight.load()});
    void* result = originalRequest(pool, request.width, request.height, dynamic, distance, lightId);
    if (spotlight && result) {
        spot_shadow_audit::RecordRequest(result,state,lightId,width,request.width,request.height);
        spot_shadow_audit::RecordPath(state,lightId,0,request.width,request.height);
        const auto seq = LiveSelection::Sequence(state);
        // This confirms CPU allocation requests only, not packed tiles or GPU completion.
        if (mode != Mode::SpotResolution2x || request.width == width * 2)
            requestedSequence.store(seq);
        if (loggedSequence.exchange(seq) != seq)
            Trace("spot_tile_request seq=%llu mode=%s light=%u native=%dx%d requested=%dx%d record=%p gpu_confirmed=0",
                seq, ModeName(mode), lightId, width, height, request.width, request.height, result);
    }
    return result;
}
__declspec(noinline) inline void ResizeHook(void* pool, const Extent* native) {
    ResizeAt(pool, native, static_cast<const unsigned char*>(_ReturnAddress()));
}
__declspec(noinline) inline void* RequestHook(void* pool, int width, int height,
                                           unsigned dynamic, float distance, unsigned lightId) {
    return RequestAt(pool, width, height, dynamic, distance, lightId,
                     static_cast<const unsigned char*>(_ReturnAddress()));
}
__declspec(noinline) inline void ClearHook(void* pool) {
    const auto caller=static_cast<const unsigned char*>(_ReturnAddress());
    originalClear(pool);
    if(caller==rendererBase+0x1a1c46 && pool==expandedPool.load()) spot_shadow_audit::Packed(pool);
}
__declspec(noinline) inline void* VsmRequestHook(void* pool,int width,int height,unsigned dynamic,float distance,unsigned lightId) {
    const auto caller=static_cast<const unsigned char*>(_ReturnAddress());
    const auto state=selection.Read();
    void* record=originalVsmRequest(pool,width,height,dynamic,distance,lightId);
    if(record && caller==rendererBase+0x18ac64 && dynamic==0)
        spot_shadow_audit::RecordPath(state,lightId,1,width,height);
    else if(record && caller==rendererBase+0x18acba && dynamic==1)
        spot_shadow_audit::RecordPath(state,lightId,2,width,height);
    return record;
}
inline bool Observed(uint64_t state) noexcept {
    return requestedSequence.load() == LiveSelection::Sequence(state) && spot_shadow_audit::Observed(state);
}
inline void Install(HMODULE renderer) noexcept {
    if (!renderer) return;
    AcquireSRWLockExclusive(&installLock);
    if (originalRequest) { ReleaseSRWLockExclusive(&installLock); return; }
    auto base = reinterpret_cast<unsigned char*>(renderer);
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (pe->OptionalHeader.SizeOfImage < 0x8029c8) {
        ReleaseSRWLockExclusive(&installLock); return;
    }
    // Exact export, virtual targets and caller bytes from the reviewed renderer.
    // Unknown versions retain the original allocator and unavailable test button.
    const unsigned char resizeCall[] = {0xff,0x50,0x08};
    void** vsmTable = reinterpret_cast<void**>(base + 0x637198);
    void** table = reinterpret_cast<void**>(base + 0x637400);
    const auto getter = GetProcAddress(renderer, "?getDynamicShadowMapResolution@SpotLight@rend@@QEBAHXZ");
    const unsigned char spotCall[] = {0x44,0x8b,0xc2,0x41,0xff,0x52,0x10};
    const bool valid = reinterpret_cast<void*>(getter) == base + 0x25b20 &&
        table[1] == base + 0x1a19a0 && table[2] == base + 0x1a19e0 &&
        table[6] == base + 0x1a1ef0 && vsmTable[2] == base + 0x1a01e0 &&
        !std::memcmp(base+0x1a1c43,"\xff\x50\x30",3) &&
        !std::memcmp(base + 0x1376c5, resizeCall, sizeof(resizeCall)) &&
        !std::memcmp(base + 0x18acb3, spotCall, sizeof(spotCall));
    if (valid) {
        rendererBase = base;
        originalResize = reinterpret_cast<Resize>(table[1]);
        originalRequest = reinterpret_cast<Request>(table[2]);
        originalClear = reinterpret_cast<Clear>(table[6]);
        originalVsmRequest = reinterpret_cast<Request>(vsmTable[2]);
        const bool clear = PatchSlot(&table[6], reinterpret_cast<void*>(originalClear), reinterpret_cast<void*>(&ClearHook));
        const bool vsm = clear && PatchSlot(&vsmTable[2], reinterpret_cast<void*>(originalVsmRequest), reinterpret_cast<void*>(&VsmRequestHook));
        const bool resize = vsm && PatchSlot(&table[1], reinterpret_cast<void*>(originalResize), reinterpret_cast<void*>(&ResizeHook));
        const bool request = resize && PatchSlot(&table[2], reinterpret_cast<void*>(originalRequest), reinterpret_cast<void*>(&RequestHook));
        if (!request) {
            if (resize) PatchSlot(&table[1], reinterpret_cast<void*>(&ResizeHook), reinterpret_cast<void*>(originalResize));
            if (vsm) PatchSlot(&vsmTable[2], reinterpret_cast<void*>(&VsmRequestHook), reinterpret_cast<void*>(originalVsmRequest));
            if (clear) PatchSlot(&table[6], reinterpret_cast<void*>(&ClearHook), reinterpret_cast<void*>(originalClear));
            originalRequest = nullptr;
        }
        Trace("spot_resolution_hooks installed=%u native_filter=preserved", unsigned(request));
    } else Trace("spot_resolution_hooks installed=0 reason=renderer_contract_mismatch");
    ReleaseSRWLockExclusive(&installLock);
}
} // namespace spot_resolution_runtime
