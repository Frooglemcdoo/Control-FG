#pragma once
// Included inside native_lighting. All code/data addresses belong to the
// already hash-verified renderer; exact call sites and option loads are checked.
namespace spot_culling_runtime {
using CollectScene = void (*)(void*);
using CollectObjects = void (*)(void*, void*, void*);
inline CollectScene originalScene = nullptr;
inline CollectObjects originalObjects = nullptr;
inline RRAlbedoCallPatch patches[2]{};
inline std::atomic<bool> ready{false};
inline SRWLOCK updateLock = SRWLOCK_INIT;
inline float* farDistance = nullptr;
struct ExpansionSlot {
    float* value = nullptr;
    float native = 0.0f, applied = 0.0f;
    bool owned = false;
};
inline ExpansionSlot slots[2];
inline LONG Bits(float value) noexcept { LONG bits; memcpy(&bits,&value,sizeof(bits)); return bits; }
inline float Value(LONG bits) noexcept { float value; memcpy(&value,&bits,sizeof(value)); return value; }
inline float Read(float* address) noexcept {
    return Value(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),0,0));
}
inline bool Exchange(float* address, float expected, float next) noexcept {
    return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),Bits(next),Bits(expected)) == Bits(expected);
}
inline void UpdateSlot(ExpansionSlot& slot, bool enabled, float distance) noexcept {
    const float current = Read(slot.value);
    if (!enabled) {
        // Do not overwrite a later engine/debug-option change.
        if (slot.owned) Exchange(slot.value,slot.applied,slot.native);
        slot.owned = false;
        return;
    }
    if (!spot_culling::ValidExpansion(current)) return;
    if (!slot.owned || Bits(current) != Bits(slot.applied)) slot.native = current;
    const float next = spot_culling::ExpandedView(slot.native,distance);
    slot.owned = Exchange(slot.value,current,next);
    slot.applied = next;
}
inline void Update() noexcept {
    if (!ready.load() || !farDistance || !TryAcquireSRWLockExclusive(&updateLock)) return;
    const auto state = selection.Read();
    const bool enabled = LiveSelection::GetMode(state) == Mode::RtSpot &&
        (readyMask.load() & kRtSpotReadyBit) != 0;
    const float distance = Read(farDistance);
    const float before0 = Read(slots[0].value), before1 = Read(slots[1].value);
    for (auto& slot : slots) UpdateSlot(slot,enabled,distance);
    const float after0 = Read(slots[0].value), after1 = Read(slots[1].value);
    if (Bits(before0) != Bits(after0) || Bits(before1) != Bits(after1))
        Trace("rt_spot_culling seq=%llu enabled=%u expansion=%g no_reflections_expansion=%g far_distance=%g native_expansion=%g native_no_reflections_expansion=%g",
            LiveSelection::Sequence(state),unsigned(enabled),double(after0),double(after1),double(distance),double(slots[0].native),double(slots[1].native));
    ReleaseSRWLockExclusive(&updateLock);
}
inline void SceneHook(void* scene) {
    Update();
    originalScene(scene);
}
inline void ObjectsHook(void* objects, void* view, void* output) {
    Update();
    originalObjects(objects,view,output);
}
inline bool Install(HMODULE renderer) noexcept {
    if (ready.load()) return true;
    if (!renderer || renderer != verifiedRenderer) return false;
    auto* base = reinterpret_cast<unsigned char*>(renderer);
    // Both consumers select between the same two frustum-expansion options.
    const unsigned char loads[][8] = {
        {0xf3,0x0f,0x10,0x05,0x9b,0x65,0x86,0x00},
        {0xf3,0x0f,0x10,0x05,0xd1,0x64,0x86,0x00},
        {0xf3,0x0f,0x10,0x05,0x7e,0xd7,0x75,0x00},
        {0xf3,0x0f,0x10,0x05,0xb4,0xd6,0x75,0x00}
    };
    const size_t rvas[] = {0xa82ad,0xa82b7,0x1b10ca,0x1b10d4};
    for (unsigned i=0;i<4;++i) if (memcmp(base+rvas[i],loads[i],8)) return false;
    originalScene = reinterpret_cast<CollectScene>(base+0xa81b0);
    originalObjects = reinterpret_cast<CollectObjects>(base+0x1b0fe0);
    if (!RRAlbedoPrepareCallPatch(base+0x123f6e,reinterpret_cast<void*>(originalScene),reinterpret_cast<void*>(&SceneHook),&patches[0]) ||
        !RRAlbedoPrepareCallPatch(base+0x1b5147,reinterpret_cast<void*>(originalObjects),reinterpret_cast<void*>(&ObjectsHook),&patches[1])) return false;
    slots[0].value = reinterpret_cast<float*>(base+0x90e790);
    slots[1].value = reinterpret_cast<float*>(base+0x90e850);
    farDistance = reinterpret_cast<float*>(base+0x90e910);
    if (!RRAlbedoExchangeCall(&patches[0],true)) return false;
    if (!RRAlbedoExchangeCall(&patches[1],true)) {
        RRAlbedoExchangeCall(&patches[0],false);
        return false;
    }
    ready.store(true);
    Trace("rt_spot_culling_hooks ready=1 native_expansion=%g native_no_reflections_expansion=%g far_distance=%g",
        double(Read(slots[0].value)),double(Read(slots[1].value)),double(Read(farDistance)));
    return true;
}
} // namespace spot_culling_runtime
