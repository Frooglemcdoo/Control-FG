#pragma once
#include "rr_dimensions.h"
// Exact-build native target primitives for the bounded G2 material-image pass.
// Include after rr_guide_inputs.h. These helpers do not select shaders, schedule
// worker jobs, or establish GPU completion. All SEH leaves contain POD only.
// Original DLL identities are checked by probe.cpp before Initialize is called.

struct RRAlbedoNativeFloat4 { float x, y, z, w; };
static_assert(sizeof(RRAlbedoNativeFloat4) == 16, "Microsoft x64 Vector4 payload");
using RRAlbedoCreateTextureFn = void (*)(int, int, int, int, int, int, int,
    const void*, unsigned long long, void**, const char*, int, int, float,
    RRAlbedoNativeFloat4, int);
using RRAlbedoTextureFn = void (*)(void*);
using RRAlbedoTargetsFn = void (*)(void*, int, const void* const*, const void*);
using RRAlbedoClearFn = void (*)(void*, void*, const RRAlbedoNativeFloat4*);
using RRAlbedoScissorFn = void (*)(void*, int, int, int, int);
using RRAlbedoFloatViewportFn = void (*)(void*, const float*);

struct RRAlbedoNativeAPI {
    unsigned char* module;
    RRAlbedoCreateTextureFn create;
    RRAlbedoTextureFn destroy;
    RRAlbedoTextureFn deleteMemory;
    RRAlbedoTextureFn producing;
    RRAlbedoTextureFn consuming;
    RRAlbedoTargetsFn targets;
    RRAlbedoClearFn clear;
    RRAlbedoScissorFn scissor;
    RRAlbedoFloatViewportFn viewport;
};
struct RRAlbedoNativeTarget {
    void* nativeTexture;
    ID3D12Resource* resource; // borrowed from the engine-owned NativeTexture
    unsigned int width;
    unsigned int height;
    DWORD creationThread;
    bool everRecorded;
};
struct RRAlbedoNativeBindings {
    void* deviceState;
    void* commandContext;
    void* staticTlsBlock;
    void* nativeTargets[8];
    void* depthTarget;
    float viewport[6];
    int scissor[4];
    int targetCount;
    unsigned int width;
    unsigned int height;
    DWORD threadId;
    bool workerContext;
};

static bool RRAlbedoNativeInitialize(HMODULE d3d, RRAlbedoNativeAPI* out) noexcept {
    if (!d3d || d3d != verifiedD3d || !out) return false;
    RRAlbedoNativeAPI api{};
    api.module = reinterpret_cast<unsigned char*>(d3d);
#define RR_ALBEDO_EXPORT(field, type, rva, name) \
    api.field = reinterpret_cast<type>(GetProcAddress(d3d, name)); \
    if (reinterpret_cast<unsigned char*>(api.field) != api.module + rva) return false
    RR_ALBEDO_EXPORT(create, RRAlbedoCreateTextureFn, 0x3B0E0,
        "?createTexture2D@NativeTextureUtil@d3d@@SAXHHHHW4PixelFormat@2@HHPEBX_KPEAPEAVNativeTexture@2@PEBDW4MiscFlags@2@HMV?$Vector4Template@M@m@@W4TileMode@2@@Z");
    RR_ALBEDO_EXPORT(destroy, RRAlbedoTextureFn, 0x3A3E0, "??1NativeTexture@d3d@@QEAA@XZ");
    RR_ALBEDO_EXPORT(producing, RRAlbedoTextureFn, 0x3AC50, "?prepareForProducing@NativeTexture@d3d@@QEAAXXZ");
    RR_ALBEDO_EXPORT(consuming, RRAlbedoTextureFn, 0x3A520, "?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ");
    RR_ALBEDO_EXPORT(targets, RRAlbedoTargetsFn, 0x2EB00,
        "?setRenderTargets@DeviceUtil@d3d@@SAXAEAVDeviceState@2@HQEAPEBVNativeTexture@2@PEBV42@@Z");
    RR_ALBEDO_EXPORT(clear, RRAlbedoClearFn, 0x30820,
        "?clearRenderTarget@DeviceUtil@d3d@@SAXAEAVDeviceState@2@PEAVNativeTexture@2@AEBV?$Vector4Template@M@m@@@Z");
    RR_ALBEDO_EXPORT(scissor, RRAlbedoScissorFn, 0x2F610,
        "?setScissorRect@DeviceUtil@d3d@@SAXAEAVDeviceState@2@HHHH@Z");
#undef RR_ALBEDO_EXPORT
    // The exact-float helper is the native setViewport tail target. Its cache
    // remains caller-owned; RestoreViewport writes only the six proven floats.
    api.viewport = reinterpret_cast<RRAlbedoFloatViewportFn>(api.module + 0x2E930);
    __try {
        api.deleteMemory = *reinterpret_cast<RRAlbedoTextureFn*>(api.module + 0x5D880);
        if (!api.deleteMemory) return false;
        *out = api;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Borrowed pointers are read only inside the selected native callback, while
// the native renderer still owns them. Never retain a Bindings across frames.
static bool RRAlbedoNativeContext(const RRAlbedoNativeAPI* api,
    void** tlsOut, void** contextOut, void** stateOut, bool* workerOut) noexcept {
    const DWORD index = *reinterpret_cast<const DWORD*>(api->module + kD3dTlsIndexRva);
    if (index > 4095) return false;
    auto array = reinterpret_cast<unsigned char**>(__readgsqword(0x58));
    if (!array || !array[index]) return false;
    auto tls = array[index];
    void* context = *reinterpret_cast<void**>(tls + 8);
    *workerOut = context != nullptr;
    if (!context) context = *reinterpret_cast<void**>(api->module + kD3dFallbackContextPointerRva);
    if (!context || !*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(context) + 0x18)) return false;
    *tlsOut = tls;
    *contextOut = context;
    *stateOut = tls + 0x44250;
    return true;
}
#include "rr_albedo_depth.h"
static bool RRAlbedoNativeReadBindings(const RRAlbedoNativeAPI* api,
    RRAlbedoNativeBindings* out, const char** reason, DWORD* fault) noexcept {
    if (!api || !api->module || !out || !reason || !fault) return false;
    *out = {}; *fault = 0; *reason = "native_binding_unavailable";
    __try {
        if (!RRAlbedoNativeContext(api, &out->staticTlsBlock, &out->commandContext,
            &out->deviceState, &out->workerContext)) return false;
        auto state = static_cast<unsigned char*>(out->deviceState);
        out->targetCount = *reinterpret_cast<const int*>(state + 0x103C);
        if (out->targetCount < 1 || out->targetCount > 8) { *reason = "unsupported_native_target_count"; return false; }
        memcpy(out->nativeTargets, state + 0x9A0, sizeof(out->nativeTargets));
        out->depthTarget = *reinterpret_cast<void**>(state + 0x9E0);
        // Native primary binding would otherwise change inherited resource
        // states. This gate also establishes the expected worker inheritance.
        for (int i = 0; i < out->targetCount; ++i) {
            if (!RRAlbedoNativeTextureState(out->nativeTargets[i], D3D12_RESOURCE_STATE_RENDER_TARGET)) {
                *reason = "original_color_not_render_target"; return false;
            }
        }
        if (!RRAlbedoNativeDepthBinding(out->depthTarget,out->deviceState,out->workerContext)) {
            *reason = out->workerContext?"worker_depth_descriptor_not_inherited":"original_depth_not_depth_write"; return false;
        }
        auto depth = static_cast<unsigned char*>(out->depthTarget);
        auto depthResource = *reinterpret_cast<ID3D12Resource**>(depth + 0x88);
        if (!depthResource) { *reason = "depth_resource_missing"; return false; }
        D3D12_RESOURCE_DESC desc = depthResource->GetDesc();
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width < 64 || desc.Width > 8192 ||
            desc.Height < 64 || desc.Height > 8192 || desc.Width * desc.Height > control_rr::MaxRenderPixels ||
            desc.DepthOrArraySize != 1 || desc.MipLevels != 1 || desc.SampleDesc.Count != 1 || desc.SampleDesc.Quality != 0 ||
            !(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
            *reason = "unsupported_native_depth_description"; return false;
        }
        out->width = static_cast<unsigned int>(desc.Width); out->height = desc.Height;
        memcpy(out->viewport, state + 0x894, sizeof(out->viewport));
        memcpy(out->scissor, state + 0x8AC, sizeof(out->scissor));
        for (int i = 0; i < 6; ++i) {
            if (!std::isfinite(out->viewport[i])) { *reason = "native_viewport_nonfinite"; return false; }
        }
        if (out->viewport[0] < 0.0f || out->viewport[1] < 0.0f ||
            out->viewport[2] <= 0.0f || out->viewport[3] <= 0.0f ||
            out->viewport[0] + out->viewport[2] > static_cast<float>(out->width) ||
            out->viewport[1] + out->viewport[3] > static_cast<float>(out->height) ||
            out->viewport[4] < 0.0f || out->viewport[5] > 1.0f || out->viewport[4] > out->viewport[5] ||
            out->scissor[0] > out->scissor[2] || out->scissor[1] > out->scissor[3]) {
            *reason = "unsupported_native_viewport_or_scissor"; return false;
        }
        out->threadId = GetCurrentThreadId(); *reason = "ready"; return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode(); *reason = "native_binding_read_exception"; return false;
    }
}

static bool RRAlbedoNativeCreate(const RRAlbedoNativeAPI* api, unsigned int width,
    unsigned int height, RRAlbedoNativeTarget* out, const char** reason, DWORD* fault) noexcept {
    if (!api || !out || out->nativeTexture || !reason || !fault || width < 64 || height < 64 ||
        width > 8192 || height > 8192 || static_cast<unsigned long long>(width) * height > control_rr::MaxRenderPixels) return false;
    *fault = 0; *reason = "native_target_creation_failed";
    RRGuideInputSnapshot primary{};
    __try {
        if (!RRGuideReadRendererContext(&primary, reason)) {
            *reason = "native_allocation_requires_primary_context"; return false;
        }
        out->creationThread = GetCurrentThreadId(); out->width = width; out->height = height;
        // Native PixelFormat23=RGBA16F. Usage1 permits RT and native SRV views.
        api->create(static_cast<int>(width), static_cast<int>(height), 1, 1, 23, 1, 1,
            nullptr, 0, &out->nativeTexture, nullptr, 0, 0, 0.5f, {0, 0, 0, 0}, -1);
        if (!out->nativeTexture) return false;
        auto bytes = static_cast<unsigned char*>(out->nativeTexture);
        out->resource = *reinterpret_cast<ID3D12Resource**>(bytes + 0x88);
        if (!out->resource) return false;
        D3D12_RESOURCE_DESC desc = out->resource->GetDesc();
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width != width || desc.Height != height ||
            desc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT || desc.DepthOrArraySize != 1 ||
            desc.MipLevels != 1 || desc.SampleDesc.Count != 1 || desc.SampleDesc.Quality != 0 ||
            desc.Flags != D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || bytes[0x68]) return false;
        *reason = "ready"; return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // A non-null partial native allocation is retained for diagnosis. A
        // fault does not prove that native destruction is safe to attempt.
        *fault = GetExceptionCode(); *reason = "native_target_creation_exception"; return false;
    }
}
static bool RRAlbedoNativePrepareClear(const RRAlbedoNativeAPI* api,
    RRAlbedoNativeTarget* target, DWORD* fault) noexcept {
    if (!api || !target || !target->nativeTexture || !fault) return false;
    *fault = 0;
    void* tls = nullptr; void* context = nullptr; void* state = nullptr; bool worker = false;
    RRGuideInputSnapshot primary{}; const char* reason = nullptr;
    RRAlbedoNativeFloat4 zero{};
    __try {
        if (!RRGuideReadRendererContext(&primary, &reason) ||
            !RRAlbedoNativeContext(api, &tls, &context, &state, &worker) || worker) return false;
        target->everRecorded = true; // before the first potentially recorded command
        api->producing(target->nativeTexture);
        api->clear(state, target->nativeTexture, &zero);
        return RRAlbedoNativeTextureState(target->nativeTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    } __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return false; }
}
static bool RRAlbedoNativeSameContext(const RRAlbedoNativeAPI* api, const RRAlbedoNativeBindings* saved) noexcept {
    void* tls = nullptr; void* context = nullptr; void* state = nullptr; bool worker = false;
    return saved->threadId == GetCurrentThreadId() &&
        RRAlbedoNativeContext(api, &tls, &context, &state, &worker) &&
        tls == saved->staticTlsBlock && context == saved->commandContext && state == saved->deviceState &&
        worker == saved->workerContext;
}
static void RRAlbedoNativeRestoreViewport(const RRAlbedoNativeAPI* api, const RRAlbedoNativeBindings* saved) noexcept {
    auto cache = static_cast<unsigned char*>(saved->deviceState) + 0x894;
    memcpy(cache, saved->viewport, sizeof(saved->viewport));
    api->viewport(saved->deviceState, reinterpret_cast<const float*>(cache));
    api->scissor(saved->deviceState, saved->scissor[0], saved->scissor[1], saved->scissor[2], saved->scissor[3]);
}
static bool RRAlbedoNativeBind(const RRAlbedoNativeAPI* api, const RRAlbedoNativeBindings* saved,
    RRAlbedoNativeTarget* const* targets, unsigned int count, bool* bindingsChanged, DWORD* fault) noexcept {
    if (!api || !saved || !targets || !bindingsChanged || !fault || count < 1 || count > 2) return false;
    *fault = 0; *bindingsChanged = false;
    const void* native[2]{};
    __try {
        if (!RRAlbedoNativeSameContext(api, saved)) return false;
        if (!RRAlbedoNativeDepthBinding(saved->depthTarget,saved->deviceState,saved->workerContext)) return false;
        for (unsigned int i = 0; i < count; ++i) {
            if (!targets[i] || !targets[i]->everRecorded || targets[i]->width != saved->width || targets[i]->height != saved->height ||
                !RRAlbedoNativeTextureState(targets[i]->nativeTexture, D3D12_RESOURCE_STATE_RENDER_TARGET)) return false;
            native[i] = targets[i]->nativeTexture;
            if (native[i] == saved->depthTarget || (i && native[0] == native[i])) return false;
            for (int j = 0; j < saved->targetCount; ++j) if (native[i] == saved->nativeTargets[j]) return false;
        }
        *bindingsChanged = true; // restore is mandatory even if a native call faults
        api->targets(saved->deviceState, static_cast<int>(count), native, saved->depthTarget);
        RRAlbedoNativeRestoreViewport(api, saved);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return false; }
}
static bool RRAlbedoNativeRestore(const RRAlbedoNativeAPI* api,
    const RRAlbedoNativeBindings* saved, DWORD* fault) noexcept {
    if (!api || !saved || !fault) return false;
    *fault = 0;
    __try {
        if (!RRAlbedoNativeSameContext(api, saved)) return false;
        // Once binding changed, always attempt original bindings rather than
        // abandoning restoration because a late diagnostic state check failed.
        api->targets(saved->deviceState, saved->targetCount,
            const_cast<const void* const*>(saved->nativeTargets), saved->depthTarget);
        RRAlbedoNativeRestoreViewport(api, saved);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return false; }
}

// Deliberately no automatic destructor: a worker, scope exit, DLL detach, or
// merely a successful CPU draw does not establish a safe native retirement.
// The caller retains target owners until GPU completion AND all native CPU
// users finish. Allocation/recording faults must quarantine owners instead.

static bool RRAlbedoNativeReadbackMatches(const RRAlbedoNativeTarget* target,
    ID3D12Resource* readback, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* placed) noexcept {
    if (!target || !target->resource || !readback || readback == target->resource || !placed) return false;
    const D3D12_RESOURCE_DESC source = target->resource->GetDesc();
    if (source.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || source.Width != target->width ||
        source.Height != target->height || source.Format != DXGI_FORMAT_R16G16B16A16_FLOAT ||
        source.MipLevels != 1 || source.DepthOrArraySize != 1 || source.SampleDesc.Count != 1 ||
        source.SampleDesc.Quality != 0 || source.Flags != D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) return false;
    const D3D12_RESOURCE_DESC destination = readback->GetDesc();
    if (destination.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || destination.Flags != D3D12_RESOURCE_FLAG_NONE ||
        placed->Footprint.Format != source.Format || placed->Footprint.Width != target->width ||
        placed->Footprint.Height != target->height || placed->Footprint.Depth != 1 ||
        placed->Footprint.RowPitch < target->width * 8u || placed->Footprint.RowPitch % 256u || placed->Offset % 512ull)
        return false;
    D3D12_HEAP_PROPERTIES heap{}; D3D12_HEAP_FLAGS flags{};
    if (FAILED(readback->GetHeapProperties(&heap, &flags)) || heap.Type != D3D12_HEAP_TYPE_READBACK) return false;
    const unsigned long long bytes = static_cast<unsigned long long>(placed->Footprint.RowPitch) *
        (target->height - 1u) + static_cast<unsigned long long>(target->width) * 8u;
    return placed->Offset <= destination.Width && bytes <= destination.Width - placed->Offset;
}
static bool RRAlbedoNativeEmergencyRestore(ID3D12GraphicsCommandList* list,
    void* context, unsigned int count, const D3D12_RESOURCE_BARRIER* barriers, DWORD* fault) noexcept {
    __try {
        ++*reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(context) + 8);
        list->ResourceBarrier(count, barriers);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return false; }
}
// Preconditions owned by the orchestration layer: all native worker command
// lists contributing to these targets have joined into the main ordered list,
// no future worker can write them, and all target/readback owners stay alive
// through the fence enqueued after the corresponding native Present flush.
static bool RRAlbedoNativeCopyAfterJoinedImpl(const RRAlbedoNativeAPI* api,
    RRAlbedoNativeTarget* const* targets, unsigned int count, ID3D12Resource* const* readbacks,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, const RRGuideInputSnapshot* expected,
    RRGuideInputSnapshot* submission,
    const char** reason, bool* commandsStarted, RRGuideCopyProgress* progress) noexcept {
    if (!api || !targets || !readbacks || !layouts || !expected || !submission ||
        expected == submission || count < 1 || count > 2) return false;
    *submission = {};
    if (!RRGuideReadRendererContext(submission, reason)) return false;
    if (submission->captureThreadId != expected->captureThreadId || submission->engineFrame != expected->engineFrame ||
        submission->presentToken != expected->presentToken || submission->commandContext != expected->commandContext ||
        submission->commandList != expected->commandList || submission->recordingLock != expected->recordingLock ||
        submission->engineQueue != expected->engineQueue || submission->queue != expected->queue ||
        submission->staticTlsBlock != expected->staticTlsBlock) {
        *reason = "native_readback_expected_context_changed"; return false;
    }
    void* trackers[2]{};
    for (unsigned int i = 0; i < count; ++i) {
        if (!targets[i] || !RRAlbedoNativeTextureState(targets[i]->nativeTexture, D3D12_RESOURCE_STATE_RENDER_TARGET) ||
            !RRAlbedoNativeReadbackMatches(targets[i], readbacks[i], &layouts[i])) {
            *reason = "native_readback_preflight_failed"; return false;
        }
        auto bytes = static_cast<unsigned char*>(targets[i]->nativeTexture);
        trackers[i] = *reinterpret_cast<void**>(bytes + 0x50);
        if (!trackers[i]) trackers[i] = bytes + 0x40;
        if (i && (trackers[0] == trackers[1] || targets[0]->resource == targets[1]->resource || readbacks[0] == readbacks[1])) {
            *reason = "native_readback_alias"; return false;
        }
    }
    RRGuideInputSnapshot deviceCheck = *submission;
    deviceCheck.gbuffer1.resource = targets[0]->resource;
    deviceCheck.gbuffer2.resource = targets[count - 1u]->resource;
    if (!RRGuideResourcesShareDevice(&deviceCheck, readbacks[0], count == 2 ? readbacks[1] : nullptr)) {
        *reason = "native_readback_device_mismatch"; return false;
    }
    if (count == 2 && reinterpret_cast<uintptr_t>(trackers[0]) > reinterpret_cast<uintptr_t>(trackers[1])) {
        void* temporary = trackers[0]; trackers[0] = trackers[1]; trackers[1] = temporary;
    }
    bool locked[2]{}; bool recordingLocked = false; bool copied = false;
    RRGuideInputSnapshot current{};
    D3D12_RESOURCE_BARRIER before[2]{}; D3D12_RESOURCE_BARRIER after[2]{};
    __try {
        locked[0] = RRGuideTryAcquireNativeLock(trackers[0]);
        if (!locked[0]) { *reason = "native_target_tracker_busy"; __leave; }
        if (count == 2) {
            locked[1] = RRGuideTryAcquireNativeLock(trackers[1]);
            if (!locked[1]) { *reason = "native_target_tracker_busy"; __leave; }
        }
        recordingLocked = RRGuideTryAcquireNativeLock(submission->recordingLock);
        if (!recordingLocked) { *reason = "native_recording_context_busy"; __leave; }
        if (!RRGuideReadRendererContext(&current, reason)) __leave;
        if (current.commandContext != submission->commandContext || current.commandList != submission->commandList ||
            current.recordingLock != submission->recordingLock || current.engineQueue != submission->engineQueue ||
            current.queue != submission->queue || current.engineFrame != submission->engineFrame ||
            current.presentToken != submission->presentToken) {
            *reason = "native_readback_context_changed"; __leave;
        }
        bool statesMatch = true;
        for (unsigned int i = 0; i < count; ++i) {
            if (!RRAlbedoNativeTextureState(targets[i]->nativeTexture, D3D12_RESOURCE_STATE_RENDER_TARGET)) statesMatch = false;
            before[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            before[i].Transition.pResource = targets[i]->resource;
            before[i].Transition.Subresource = 0;
            before[i].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            before[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            after[i] = before[i];
            after[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            after[i].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }
        if (!statesMatch) { *reason = "native_target_state_changed"; __leave; }
        auto commandCount = reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(current.commandContext) + 8);
        *commandsStarted = true;
        ++*commandCount;
        current.commandList->ResourceBarrier(count, before);
        progress->initialBarrierCompleted = true;
        for (unsigned int i = 0; i < count; ++i) {
            D3D12_TEXTURE_COPY_LOCATION source{}; D3D12_TEXTURE_COPY_LOCATION destination{};
            source.pResource = targets[i]->resource; source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destination.pResource = readbacks[i]; destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            destination.PlacedFootprint = layouts[i];
            ++*commandCount;
            current.commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        }
        ++*commandCount;
        progress->restorationAttempted = true;
        current.commandList->ResourceBarrier(count, after);
        progress->restorationCompleted = true;
        copied = true; *reason = "native_target_copies_recorded";
    } __finally {
        if (progress->initialBarrierCompleted && !progress->restorationAttempted) {
            progress->restorationAttempted = true; progress->emergencyRestorationAttempted = true;
            progress->restorationCompleted = RRAlbedoNativeEmergencyRestore(current.commandList,
                current.commandContext, count, after, &progress->restorationFault);
        }
        if (recordingLocked) RRGuideReleaseNativeLock(submission->recordingLock);
        if (locked[1]) RRGuideReleaseNativeLock(trackers[1]);
        if (locked[0]) RRGuideReleaseNativeLock(trackers[0]);
    }
    return copied;
}
static bool RRAlbedoNativeCopyAfterJoined(const RRAlbedoNativeAPI* api,
    RRAlbedoNativeTarget* const* targets, unsigned int count, ID3D12Resource* const* readbacks,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, const RRGuideInputSnapshot* expected,
    const char** reason, bool* commandsStarted, DWORD* fault) noexcept {
    if (!reason || !commandsStarted || !fault) return false;
    *reason = "native_copy_not_started"; *commandsStarted = false; *fault = 0;
    RRGuideCopyProgress progress{};
    RRGuideInputSnapshot submission{};
    __try {
        return RRAlbedoNativeCopyAfterJoinedImpl(api, targets, count, readbacks, layouts, expected, &submission,
            reason, commandsStarted, &progress);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        if (!*commandsStarted) *reason = "native_copy_preflight_exception";
        else if (!progress.initialBarrierCompleted) *reason = "native_copy_initial_barrier_ambiguous";
        else if (progress.restorationCompleted) *reason = "native_copy_exception_states_restored";
        else if (progress.emergencyRestorationAttempted) *reason = "native_copy_emergency_restore_ambiguous";
        else *reason = "native_copy_final_barrier_ambiguous";
        return false;
    }
}
