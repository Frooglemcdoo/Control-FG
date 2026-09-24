#pragma once
#include <cstdint>
// Exact supplied renderer DLL identity must pass before these addresses are used.
// All offsets revalidated by g2-review/native-draw/revalidate.py.
namespace control_rr_albedo {
inline constexpr std::uintptr_t kPreparationVtable=0x631FF8;
inline constexpr std::uintptr_t kPreparationInvokeSlot=0x632008;
inline constexpr std::uintptr_t kPreparationThunk=0x13E4D0;
inline constexpr std::uintptr_t kPrimaryInvokeSlot=0x631C18;
inline constexpr std::uintptr_t kPrimaryThunk=0x13EBB0;
inline constexpr std::uintptr_t kSerialDrawCall=0x12E29E;
inline constexpr std::uintptr_t kWorkerDrawCall=0x126591;
inline constexpr std::uintptr_t kDrawOpaque=0x1C1AE0;
inline constexpr std::uintptr_t kGetShader=0x1DD3F0;
inline constexpr std::uintptr_t kWireframe=0x802A33;
inline constexpr std::uintptr_t kWireframeWithoutDepth=0x802A6A;
inline constexpr std::uintptr_t kRenderPrimary=0x7EC152;
inline constexpr std::uintptr_t kParallelPrimary=0x911830;
struct PreparationState {
    std::uintptr_t renderer=0,manager=0,resourceTable=0;
    void* primaryView=nullptr;
    bool parallel=false;
};
// Windows headers/intrinsics are supplied by probe.cpp. Keep this POD-only
// SEH leaf separate from vector-owning preparation and hook orchestration.
// The primary scene callback installs its captured rend::View at renderer TLS+8
// (136DB5). Preparation126077 captures it for the worker; worker1263E1 restores
// it. This is distinct from the persistent camera pointer at renderer+678.
static bool ReadPrimaryView(std::uintptr_t module, void** out, DWORD* fault) noexcept {
    if (fault) *fault=0;
    if (!out) return false;
    *out=nullptr;
    if (!module) return false;
    __try {
        const DWORD index=*reinterpret_cast<const DWORD*>(module+0x80283C);
        if (index>4095) return false;
        auto slots=reinterpret_cast<const unsigned char* const*>(__readgsqword(0x58));
        if (!slots || !slots[index]) return false;
        auto view=*reinterpret_cast<void* const*>(slots[index]+8);
        if (!view || *static_cast<const std::uintptr_t*>(view)!=module+0x62E1F0) return false;
        *out=view;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (fault) *fault=GetExceptionCode();
        *out=nullptr;
        return false;
    }
}

static bool ReadPreparationState(std::uintptr_t module, const void* callback,
                                 const void* resourceTableIndirect,
                                 PreparationState* out, const char** reason,
                                 DWORD* fault) noexcept {
    if (reason) *reason=nullptr;
    if (fault) *fault=0;
    if (!module || !callback || !resourceTableIndirect || !out) {
        if (reason) *reason="invalid_preparation_arguments";
        return false;
    }
    *out=PreparationState{};
    __try {
        if (*static_cast<const std::uintptr_t*>(callback)!=module+kPreparationVtable) {
            if (reason) *reason="unexpected_preparation_callback_vtable";
            return false;
        }
        if (*reinterpret_cast<const unsigned char*>(module+kWireframe) ||
            *reinterpret_cast<const unsigned char*>(module+kWireframeWithoutDepth)) {
            if (reason) *reason="wireframe_or_altered_depth_mode";
            return false;
        }
        if (!*reinterpret_cast<const unsigned char*>(module+kRenderPrimary)) {
            if (reason) *reason="primary_rendering_disabled";
            return false;
        }
        out->renderer=*reinterpret_cast<const std::uintptr_t*>(
            static_cast<const unsigned char*>(callback)+8);
        out->resourceTable=*static_cast<const std::uintptr_t*>(resourceTableIndirect);
        if (!out->renderer || !out->resourceTable) {
            if (reason) *reason="missing_primary_renderer_or_resources";
            return false;
        }
        out->manager=*reinterpret_cast<const std::uintptr_t*>(out->renderer+0x80);
        out->parallel=*reinterpret_cast<const unsigned char*>(module+kParallelPrimary)!=0;
        if (!out->manager) {
            if (reason) *reason="missing_primary_opaque_manager";
            return false;
        }
        if (!ReadPrimaryView(module, &out->primaryView, fault)) {
            if (reason) *reason="primary_preparation_view_unavailable";
            *out=PreparationState{};
            return false;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (fault) *fault=GetExceptionCode();
        if (reason) *reason="preparation_state_read_fault";
        *out=PreparationState{};
        return false;
    }
}

// This is the worker job's captured View, not a general promise about View::use.
// Preparation reads renderer static TLS+8 at126077 and stores it in the worker
// job. Worker1263E1 copies that pointer into its own renderer TLS+8;126555 passes
// the same pointer to View::use before the intercepted opaque draw at126591.
static bool ReadWorkerView(std::uintptr_t module, void** out, DWORD* fault) noexcept {
    return ReadPrimaryView(module, out, fault);
}

// The preparation callback contains only renderer+8. Its +16 is not a View.
// The distinct primary/join callback contains renderer+8 and View+16. Its serial
// native branch12E285→12E289 passes that exact View to View::use before DrawOpaque.
// Root scopes this value around the original Join call for serial draw hooks.
static bool ReadJoinView(const void* callback, void** out, DWORD* fault) noexcept {
    if (fault) *fault=0;
    if (!callback || !out || !verifiedRenderer) return false;
    *out=nullptr;
    __try {
        const auto module=reinterpret_cast<std::uintptr_t>(verifiedRenderer);
        if (*static_cast<const std::uintptr_t*>(callback)!=module+0x631C08) return false;
        auto view=*reinterpret_cast<void* const*>(static_cast<const unsigned char*>(callback)+16);
        if (!view || *static_cast<const std::uintptr_t*>(view)!=module+0x62E1F0) return false;
        *out=view;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (fault) *fault=GetExceptionCode();
        *out=nullptr;
        return false;
    }
}
}
