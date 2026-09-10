#pragma once
// Build-locked read-only snapshot on the thread entering the existing AA hook.
// Layout comes from renderer exports, world-render setup, RTTI/vtables, and the
// v0.4 runtime matrix-consistency review. Raw doubles remain logged unchanged.
inline constexpr size_t kRendererInstancePointerRva = 0x802A00;
inline constexpr size_t kRendererVtableRva = 0x6323D8;
inline constexpr size_t kRendererViewOffset = 0x678;
inline constexpr size_t kViewVtableRva = 0x62E1F0;
inline constexpr size_t kActiveMathViewPointerRva = 0x8029D8;
inline constexpr size_t kRendererHudVtableSlot = 17;
inline constexpr size_t kRendererHudTargetRva = 0x1343B0;

struct CameraSnapshot {
    unsigned long long engineFrame;
    void* renderer;
    void* view;
    void* activeMathView;
    double worldToView[12];
    double viewToWorld[12];
    double viewToClip[16];
    double clipToView[16];
    double worldToClip[16];
    double clipToWorld[16];
};

// This leaf contains SEH and POD only. Never keep these borrowed pointers for
// deferred reads or perform work on another thread using the sampled objects.
static bool ReadCamera(CameraSnapshot* out, DWORD* fault, const char** reason) noexcept {
    *fault = 0;
    *reason = "null_renderer";
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedRenderer);
        auto renderer = *reinterpret_cast<unsigned char**>(base + kRendererInstancePointerRva);
        out->renderer = renderer;
        out->engineFrame = *reinterpret_cast<const unsigned long long*>(reinterpret_cast<unsigned char*>(verifiedD3d) + kEngineFrameCounterRva);
        if (!renderer) return false;
        if (*reinterpret_cast<void**>(renderer) != base + kRendererVtableRva) {
            *reason = "unexpected_renderer_vtable";
            return false;
        }
        auto view = *reinterpret_cast<unsigned char**>(renderer + kRendererViewOffset);
        out->view = view;
        *reason = "null_view";
        if (!view) return false;
        if (*reinterpret_cast<void**>(view) != base + kViewVtableRva) {
            *reason = "unexpected_view_vtable";
            return false;
        }
        out->activeMathView = *reinterpret_cast<void**>(base + kActiveMathViewPointerRva);
        memcpy(out->worldToView, view + 0x20, sizeof(out->worldToView));
        memcpy(out->viewToWorld, view + 0x80, sizeof(out->viewToWorld));
        memcpy(out->viewToClip, view + 0xE0, sizeof(out->viewToClip));
        memcpy(out->clipToView, view + 0x160, sizeof(out->clipToView));
        memcpy(out->worldToClip, view + 0x1E0, sizeof(out->worldToClip));
        memcpy(out->clipToWorld, view + 0x260, sizeof(out->clipToWorld));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        *reason = "read_exception";
        return false;
    }
}

static void LogCameraArray(unsigned long long call, const char* name,
                           const double* values, unsigned int count) noexcept {
    unsigned int finite = 1;
    for (unsigned int i = 0; i < count; ++i) {
        if (!std::isfinite(values[i])) finite = 0;
    }
    Log("CAMERA_ARRAY call=%llu name=%s count=%u finite=%u order=raw_memory", call, name, count, finite);
    // Groups are log chunks only. The runtime review interpreted the 12-value
    // affine arrays as row-major 4x3 for validation, but the raw data is kept.
    for (unsigned int i = 0; i < count; i += 4) {
        Log("CAMERA_VALUES call=%llu name=%s offset=%u values=%.17g,%.17g,%.17g,%.17g",
            call, name, i, values[i], values[i+1], values[i+2], values[i+3]);
    }
}

static void CaptureCamera(unsigned long long call, const char* phase) noexcept {
    const DWORD saved = GetLastError();
    CameraSnapshot snapshot{};
    DWORD fault = 0;
    const char* reason = nullptr;
    if (!ReadCamera(&snapshot, &fault, &reason)) {
        Log("CAMERA_UNAVAILABLE call=%llu phase=%s reason=%s exception=0x%08lX renderer=%p view=%p",
            call, phase, reason, fault, snapshot.renderer, snapshot.view);
    } else {
        Log("CAMERA_SNAPSHOT call=%llu phase=%s engine_frame=%llu renderer=%p view=%p active_math_view=%p begin=%llu present=%llu matrix_relations=v0.4_validated streamline_mapping=pending",
            call, phase, snapshot.engineFrame, snapshot.renderer, snapshot.view, snapshot.activeMathView, beginCount.load(), presentCount.load());
        LogCameraArray(call, "WorldToView", snapshot.worldToView, 12);
        LogCameraArray(call, "ViewToWorld", snapshot.viewToWorld, 12);
        LogCameraArray(call, "ViewToClip", snapshot.viewToClip, 16);
        LogCameraArray(call, "ClipToView", snapshot.clipToView, 16);
        LogCameraArray(call, "WorldToClip", snapshot.worldToClip, 16);
        LogCameraArray(call, "ClipToWorld", snapshot.clipToWorld, 16);
    }
    SetLastError(saved);
}
