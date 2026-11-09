#pragma once
#include "rr_dimensions.h"
// RR Guide G1: one-frame primary G-buffer snapshot for owned guide-image work.
// Include after camera_capture.h. This header records only source copies in
// Control's current list: no descriptor heap, root, PSO, NGX or FG changes.

struct RRGuideInputTexture {
    void* nativeTexture;
    ID3D12Resource* resource;
    D3D12_RESOURCE_DESC desc;
    void* stateTracker;
    unsigned int trackedState;
    unsigned int nativeFlags;
    unsigned int manualStateDisabled;
};
struct RRGuideInputSnapshot {
    RRGuideInputTexture gbuffer1;
    RRGuideInputTexture gbuffer2;
    CameraSnapshot camera;
    unsigned long long engineFrame;
    unsigned long long presentToken;
    DWORD captureThreadId;
    DWORD staticTlsIndex;
    void* staticTlsBlock;
    void* commandContext;
    void* recordingLock;
    void* engineQueue;
    ID3D12GraphicsCommandList* commandList;
    ID3D12CommandQueue* queue; // borrowed engine Direct queue interface
    DWORD fault;
};
// Recording progress is POD and survives SEH unwinding into the public wrapper.
struct RRGuideCopyProgress {
    bool initialBarrierCompleted;
    bool restorationAttempted;
    bool restorationCompleted;
    bool emergencyRestorationAttempted;
    DWORD restorationFault;
};
static constexpr size_t kRRGuideGBuffer1ShaderRva = 0x12962E0;
static constexpr size_t kRRGuideGBuffer2ShaderRva = 0x1296308;
static constexpr size_t kRREnvBRDFShaderRva = 0x1290A20;
static constexpr size_t kRRGuideDirectQueueRva = 0x111C38;
static constexpr size_t kRRGuideRecordingLockRva = 0x111C20;

using RRGuideGetNativeTextureFn = void* (*)(void*);
static RRGuideGetNativeTextureFn rrGuideGetNativeTexture = nullptr;
static bool RRGuideInitializeInputs(HMODULE d3d) noexcept {
    rrGuideGetNativeTexture = d3d ? reinterpret_cast<RRGuideGetNativeTextureFn>(
        GetProcAddress(d3d, "?getNativeTexture@ShaderTexture@d3d@@QEAAPEAVNativeTexture@2@XZ")) : nullptr;
    return rrGuideGetNativeTexture != nullptr;
}

// Every structure in SEH leaves is POD. Never add std::string/RAII here.
static bool RRGuideReadRendererContext(RRGuideInputSnapshot* out, const char** reason) noexcept {
    __try {
        if (!verifiedD3d || !out) { *reason = "d3d_unavailable"; return false; }
        auto d3d = reinterpret_cast<unsigned char*>(verifiedD3d);
        out->staticTlsIndex = *reinterpret_cast<const DWORD*>(d3d + kD3dTlsIndexRva);
        if (out->staticTlsIndex > 4095) { *reason = "static_tls_index_out_of_range"; return false; }
        // Original instructions use GS:[0x58], the loader's static TLS array.
        // TlsGetValue addresses dynamic TLS and is not equivalent.
        auto tlsArray = reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        if (!tlsArray) { *reason = "static_tls_array_missing"; return false; }
        auto tls = tlsArray[out->staticTlsIndex];
        if (!tls) { *reason = "static_tls_block_missing"; return false; }
        out->staticTlsBlock = tls;
        auto localContext = *reinterpret_cast<void**>(tls + 8);
        auto localLock = *reinterpret_cast<void**>(tls + 0x10);
        out->recordingLock = *reinterpret_cast<void**>(d3d + kRRGuideRecordingLockRva);
        out->engineQueue = *reinterpret_cast<void**>(tls + 0x18);
        const auto directQueue = *reinterpret_cast<void**>(d3d + kRRGuideDirectQueueRva);
        // Accept only the renderer context that originalPresent closes and
        // submits. Worker command-list completion has a separate schedule.
        if (localContext || !localLock || localLock != out->recordingLock) {
            *reason = "not_renderer_recording_context"; return false;
        }
        if (!directQueue || out->engineQueue != directQueue) {
            *reason = "not_engine_direct_queue"; return false;
        }
        out->commandContext = *reinterpret_cast<void**>(d3d + kD3dFallbackContextPointerRva);
        if (!out->commandContext) { *reason = "command_context_missing"; return false; }
        out->commandList = *reinterpret_cast<ID3D12GraphicsCommandList**>(
            reinterpret_cast<unsigned char*>(out->commandContext) + 0x18);
        out->queue = *reinterpret_cast<ID3D12CommandQueue**>(
            reinterpret_cast<unsigned char*>(directQueue) + 0x10);
        if (!out->commandList || !out->queue ||
            out->commandList->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT ||
            out->queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT) {
            *reason = "invalid_direct_list_or_queue"; return false;
        }
        out->captureThreadId = GetCurrentThreadId();
        out->engineFrame = *reinterpret_cast<const unsigned long long*>(d3d + kEngineFrameCounterRva);
        out->presentToken = presentCount.load();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode(); *reason = "context_read_exception"; return false;
    }
}
static bool RRGuideReadTexture(size_t shaderRva, RRGuideInputTexture* out,
                               const char** reason, DWORD* fault) noexcept {
    __try {
        if (!verifiedRenderer || !rrGuideGetNativeTexture) {
            *reason = "texture_getter_unavailable"; return false;
        }
        auto native = reinterpret_cast<unsigned char*>(rrGuideGetNativeTexture(
            reinterpret_cast<unsigned char*>(verifiedRenderer) + shaderRva));
        out->nativeTexture = native;
        if (!native) { *reason = "gbuffer_not_bound"; return false; }
        out->nativeFlags = *reinterpret_cast<const unsigned int*>(native + 0x38);
        out->manualStateDisabled = native[0x68];
        if ((out->nativeFlags & 0x30u) || out->manualStateDisabled) {
            *reason = "unsupported_native_state_mode"; return false;
        }
        out->resource = *reinterpret_cast<ID3D12Resource**>(native + 0x88);
        if (!out->resource) { *reason = "gbuffer_resource_missing"; return false; }
        out->desc = out->resource->GetDesc();
        if (out->desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            out->desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM ||
            out->desc.MipLevels != 1 || out->desc.DepthOrArraySize != 1 ||
            out->desc.SampleDesc.Count != 1 || out->desc.SampleDesc.Quality != 0 ||
            out->desc.Width < 64 || out->desc.Height < 64 ||
            out->desc.Width > 8192 || out->desc.Height > 8192 ||
            out->desc.Width * out->desc.Height > control_rr::MaxRenderPixels ||
            (out->desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS |
                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))) {
            *reason = "unsupported_gbuffer_description"; return false;
        }
        auto tracker = *reinterpret_cast<unsigned char**>(native + 0x50);
        if (!tracker) tracker = native + 0x40;
        out->stateTracker = tracker;
        out->trackedState = *reinterpret_cast<const unsigned int*>(tracker + 0x20);
        // The selected post-DXR window has complete SRV inputs. Reject any
        // UAV/RT/copy/unknown state instead of guessing another producer point.
        if (!out->trackedState || (out->trackedState & ~0xC0u)) {
            *reason = "gbuffer_not_in_consuming_state"; return false;
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode(); *reason = "texture_read_exception"; return false;
    }
}
static DXGI_FORMAT RRGuideEnvViewFormat(DXGI_FORMAT format) noexcept {
    switch(format) {
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R16G16_TYPELESS:return DXGI_FORMAT_R16G16_FLOAT;
    case DXGI_FORMAT_R32G32_TYPELESS:return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:return format;
    }
}
static bool RRGuideReadEnvBRDF(RRGuideInputTexture* out,const char** reason,DWORD* fault) noexcept {
    if(!out||!reason||!fault)return false;*out={};
    __try {
        if(!verifiedRenderer||!rrGuideGetNativeTexture){*reason="envbrdf_getter_unavailable";return false;}
        auto native=reinterpret_cast<unsigned char*>(rrGuideGetNativeTexture(
            reinterpret_cast<unsigned char*>(verifiedRenderer)+kRREnvBRDFShaderRva));
        out->nativeTexture=native;
        if(!native){*reason="envbrdf_not_bound";return false;}
        out->nativeFlags=*reinterpret_cast<const unsigned int*>(native+0x38);
        out->manualStateDisabled=native[0x68];
        if((out->nativeFlags&0x30u)||out->manualStateDisabled){*reason="envbrdf_unsupported_state_mode";return false;}
        out->resource=*reinterpret_cast<ID3D12Resource**>(native+0x88);
        if(!out->resource){*reason="envbrdf_resource_missing";return false;}
        out->desc=out->resource->GetDesc();
        const DXGI_FORMAT viewFormat=RRGuideEnvViewFormat(out->desc.Format);
        const bool formatOkay=viewFormat==DXGI_FORMAT_R16G16B16A16_FLOAT||
            viewFormat==DXGI_FORMAT_R16G16_FLOAT||viewFormat==DXGI_FORMAT_R32G32_FLOAT||
            viewFormat==DXGI_FORMAT_R32G32B32A32_FLOAT||viewFormat==DXGI_FORMAT_R11G11B10_FLOAT||
            viewFormat==DXGI_FORMAT_R8G8B8A8_UNORM;
        if(out->desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!formatOkay||
           !out->desc.Width||!out->desc.Height||out->desc.Width>4096||out->desc.Height>4096||
           out->desc.DepthOrArraySize!=1||out->desc.SampleDesc.Count!=1||
           (out->desc.Flags&(D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE|D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))){
            *reason="envbrdf_description_unsupported";return false;
        }
        auto tracker=*reinterpret_cast<unsigned char**>(native+0x50);if(!tracker)tracker=native+0x40;
        out->stateTracker=tracker;out->trackedState=*reinterpret_cast<const unsigned int*>(tracker+0x20);
        if(!(out->trackedState&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)||
           (out->trackedState&~(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))){
            *reason="envbrdf_not_compute_readable";return false;
        }
        *reason="ready";return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();*reason="envbrdf_read_exception";return false;}
}
static bool RRGuideResourcesShareDevice(const RRGuideInputSnapshot* in,
                                       ID3D12Resource* destination1,
                                       ID3D12Resource* destination2) noexcept {
    ID3D12Device* devices[5]{};
    bool good = false;
    __try {
        HRESULT hr = in->commandList->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[0]));
        if (FAILED(hr) || !devices[0]) __leave;
        hr = in->gbuffer1.resource->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[1]));
        if (FAILED(hr) || devices[1] != devices[0]) __leave;
        hr = in->gbuffer2.resource->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[2]));
        if (FAILED(hr) || devices[2] != devices[0]) __leave;
        if (destination1) {
            hr = destination1->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[3]));
            if (FAILED(hr) || devices[3] != devices[0]) __leave;
        }
        if (destination2) {
            hr = destination2->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[4]));
            if (FAILED(hr) || devices[4] != devices[0]) __leave;
        }
        good = true;
    } __finally {
        for (unsigned int i = 0; i < 5; ++i) if (devices[i]) devices[i]->Release();
    }
    return good;
}
static bool RRGuideReadInputsImpl(RRGuideInputSnapshot* out, const char** reason) noexcept {
    if (!out || !reason) return false;
    memset(out, 0, sizeof(*out));
    *reason = "unknown";
    if (!RRGuideReadRendererContext(out, reason)) return false;
    if (!RRGuideReadTexture(kRRGuideGBuffer1ShaderRva, &out->gbuffer1, reason, &out->fault) ||
        !RRGuideReadTexture(kRRGuideGBuffer2ShaderRva, &out->gbuffer2, reason, &out->fault)) return false;
    if (out->gbuffer1.resource == out->gbuffer2.resource ||
        out->gbuffer1.stateTracker == out->gbuffer2.stateTracker ||
        out->gbuffer1.desc.Width != out->gbuffer2.desc.Width ||
        out->gbuffer1.desc.Height != out->gbuffer2.desc.Height) {
        *reason = "gbuffer_identity_or_extent_mismatch"; return false;
    }
    DWORD cameraFault = 0;
    if (!ReadCamera(&out->camera, &cameraFault, reason)) {
        out->fault = cameraFault; return false;
    }
    if (out->camera.engineFrame != out->engineFrame) {
        *reason = "camera_frame_mismatch"; return false;
    }
    for (unsigned int i = 0; i < 12; ++i) {
        if (!std::isfinite(out->camera.viewToWorld[i]) || !std::isfinite(out->camera.worldToView[i])) {
            *reason = "camera_nonfinite"; return false;
        }
    }
    if (!RRGuideResourcesShareDevice(out, nullptr, nullptr)) {
        *reason = "source_device_mismatch"; return false;
    }
    *reason = "ready";
    return true;
}
// Contain an unexpected borrowed-resource/device read fault before recording.
static bool RRGuideReadInputs(RRGuideInputSnapshot* out, const char** reason) noexcept {
    if (!out || !reason) return false;
    __try {
        return RRGuideReadInputsImpl(out, reason);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        *reason = "input_read_exception";
        return false;
    }
}

// Native recursive SRW layout: SRW at +0, owner thread +8, recursion +0xC.
// Try-acquire lets this one-off capture skip contention without stalling.
static bool RRGuideTryAcquireNativeLock(void* object) noexcept {
    auto bytes = reinterpret_cast<unsigned char*>(object);
    auto owner = reinterpret_cast<DWORD*>(bytes + 8);
    auto recursion = reinterpret_cast<unsigned int*>(bytes + 0xC);
    const DWORD thread = GetCurrentThreadId();
    if (*owner != thread) {
        if (!TryAcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(bytes))) return false;
        *owner = thread;
    }
    ++*recursion;
    return true;
}
static void RRGuideReleaseNativeLock(void* object) noexcept {
    auto bytes = reinterpret_cast<unsigned char*>(object);
    auto recursion = reinterpret_cast<unsigned int*>(bytes + 0xC);
    if (--*recursion == 0) {
        *reinterpret_cast<DWORD*>(bytes + 8) = 0xFFFFFFFFu;
        ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(bytes));
    }
}
static bool RRGuideDestinationMatches(ID3D12Resource* destination,
                                     const RRGuideInputTexture& source) noexcept {
    if (!destination || destination == source.resource) return false;
    const D3D12_RESOURCE_DESC d = destination->GetDesc();
    return d.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        d.Width == source.desc.Width && d.Height == source.desc.Height &&
        d.Format == source.desc.Format && d.MipLevels == 1 &&
        d.DepthOrArraySize == 1 && d.SampleDesc.Count == 1 && d.SampleDesc.Quality == 0 &&
        d.Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN && d.Flags == D3D12_RESOURCE_FLAG_NONE;
}
// Called only when the initial source transition returned and no restoration
// call has been attempted. A fault is contained here, and is never retried.
static bool RRGuideEmergencyRestoreSources(ID3D12GraphicsCommandList* commandList,
                                           void* commandContext,
                                           const D3D12_RESOURCE_BARRIER* barriers,
                                           DWORD* fault) noexcept {
    *fault = 0;
    __try {
        auto count = reinterpret_cast<unsigned long long*>(
            reinterpret_cast<unsigned char*>(commandContext) + 8);
        ++*count;
        commandList->ResourceBarrier(2, barriers);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        return false;
    }
}

// Caller-owned destinations start in COPY_DEST and remain COPY_DEST. Retain
// them until the downstream ordered submission's fence completes. Native source
// pointers are borrowed only for this synchronous read/copy, not later access.
// The public wrapper distinguishes a pre-recording failure from any partial
// recording fault. The latter requires quarantine until completion is known.
static bool RRGuideCopyInputsImpl(const RRGuideInputSnapshot* snapshot,
                             ID3D12Resource* destination1,
                             ID3D12Resource* destination2,
                             const char** reason, bool* commandsStarted,
                             RRGuideCopyProgress* progress) noexcept {
    if (!snapshot || !reason || !destination1 || !destination2 || destination1 == destination2 ||
        destination1 == snapshot->gbuffer2.resource || destination2 == snapshot->gbuffer1.resource) return false;
    *reason = "copy_not_started";
    if (GetCurrentThreadId() != snapshot->captureThreadId) {
        *reason = "capture_thread_changed"; return false;
    }
    if (!RRGuideDestinationMatches(destination1, snapshot->gbuffer1) ||
        !RRGuideDestinationMatches(destination2, snapshot->gbuffer2) ||
        !RRGuideResourcesShareDevice(snapshot, destination1, destination2)) {
        *reason = "copy_destination_mismatch"; return false;
    }
    void* first = snapshot->gbuffer1.stateTracker;
    void* second = snapshot->gbuffer2.stateTracker;
    if (reinterpret_cast<uintptr_t>(first) > reinterpret_cast<uintptr_t>(second)) {
        void* temporary = first; first = second; second = temporary;
    }
    bool lockedFirst = false, lockedSecond = false, lockedRecording = false;
    bool copied = false;
    RRGuideInputSnapshot current{};
    D3D12_RESOURCE_BARRIER barriers[2]{};
    D3D12_RESOURCE_BARRIER restoreBarriers[2]{};
    __try {
        lockedFirst = RRGuideTryAcquireNativeLock(first);
        if (!lockedFirst) { *reason = "gbuffer_tracker_busy"; __leave; }
        lockedSecond = RRGuideTryAcquireNativeLock(second);
        if (!lockedSecond) { *reason = "gbuffer_tracker_busy"; __leave; }
        lockedRecording = RRGuideTryAcquireNativeLock(snapshot->recordingLock);
        if (!lockedRecording) { *reason = "recording_context_busy"; __leave; }
        if (!RRGuideReadInputs(&current, reason)) __leave;
        if (current.commandContext != snapshot->commandContext ||
            current.commandList != snapshot->commandList ||
            current.recordingLock != snapshot->recordingLock ||
            current.engineQueue != snapshot->engineQueue || current.queue != snapshot->queue ||
            current.engineFrame != snapshot->engineFrame || current.presentToken != snapshot->presentToken ||
            current.gbuffer1.nativeTexture != snapshot->gbuffer1.nativeTexture ||
            current.gbuffer2.nativeTexture != snapshot->gbuffer2.nativeTexture ||
            current.gbuffer1.resource != snapshot->gbuffer1.resource ||
            current.gbuffer2.resource != snapshot->gbuffer2.resource ||
            current.gbuffer1.stateTracker != snapshot->gbuffer1.stateTracker ||
            current.gbuffer2.stateTracker != snapshot->gbuffer2.stateTracker ||
            current.gbuffer1.trackedState != snapshot->gbuffer1.trackedState ||
            current.gbuffer2.trackedState != snapshot->gbuffer2.trackedState ||
            memcmp(current.camera.viewToWorld, snapshot->camera.viewToWorld, sizeof(current.camera.viewToWorld)) != 0) {
            *reason = "capture_inputs_changed"; __leave;
        }
        const RRGuideInputTexture* sources[2] = {&current.gbuffer1, &current.gbuffer2};
        ID3D12Resource* destinations[2] = {destination1, destination2};
        for (unsigned int i = 0; i < 2; ++i) {
            barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[i].Transition.pResource = sources[i]->resource;
            barriers[i].Transition.Subresource = 0;
            barriers[i].Transition.StateBefore = static_cast<D3D12_RESOURCE_STATES>(sources[i]->trackedState);
            barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            restoreBarriers[i] = barriers[i];
            restoreBarriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            restoreBarriers[i].Transition.StateAfter = static_cast<D3D12_RESOURCE_STATES>(sources[i]->trackedState);
        }
        // No fallible allocation/resource acquisition after recording begins.
        auto count = reinterpret_cast<unsigned long long*>(
            reinterpret_cast<unsigned char*>(current.commandContext) + 8);
        *commandsStarted = true;
        ++*count;
        current.commandList->ResourceBarrier(2, barriers);
        progress->initialBarrierCompleted = true;
        for (unsigned int i = 0; i < 2; ++i) {
            D3D12_TEXTURE_COPY_LOCATION source{};
            source.pResource = sources[i]->resource;
            source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION destination{};
            destination.pResource = destinations[i];
            destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            ++*count;
            current.commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        }
        ++*count;
        progress->restorationAttempted = true;
        current.commandList->ResourceBarrier(2, restoreBarriers);
        progress->restorationCompleted = true;
        // Source tracker states stay untouched: paired barriers restore them.
        copied = true;
        *reason = "copies_recorded";
    } __finally {
        // Copy faults leave both source states known to be COPY_SOURCE. Restore
        // them before releasing native locks. Initial/final barrier faults are
        // ambiguous and must not be retried against an unknown current state.
        if (progress->initialBarrierCompleted && !progress->restorationAttempted) {
            progress->restorationAttempted = true;
            progress->emergencyRestorationAttempted = true;
            progress->restorationCompleted = RRGuideEmergencyRestoreSources(
                current.commandList, current.commandContext, restoreBarriers,
                &progress->restorationFault);
        }
        if (lockedRecording) RRGuideReleaseNativeLock(snapshot->recordingLock);
        if (lockedSecond) RRGuideReleaseNativeLock(second);
        if (lockedFirst) RRGuideReleaseNativeLock(first);
    }
    return copied;
}

// commandsStarted remains true after any attempted command recording, including
// SEH failure. A false result with this flag true must quarantine destinations.
// Both outputs are required so callers cannot accidentally discard that state.
static bool RRGuideCopyInputs(const RRGuideInputSnapshot* snapshot,
                             ID3D12Resource* destination1,
                             ID3D12Resource* destination2,
                             const char** reason, bool* commandsStarted,
                             DWORD* recordingFault) noexcept {
    if (!reason || !commandsStarted || !recordingFault) return false;
    *reason = "copy_not_started";
    *commandsStarted = false;
    *recordingFault = 0;
    RRGuideCopyProgress progress{};
    __try {
        return RRGuideCopyInputsImpl(snapshot, destination1, destination2, reason, commandsStarted, &progress);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *recordingFault = GetExceptionCode();
        if (!*commandsStarted) *reason = "copy_preflight_exception";
        else if (!progress.initialBarrierCompleted) *reason = "copy_initial_barrier_ambiguous";
        else if (progress.restorationCompleted) *reason = "copy_exception_source_states_restored";
        else if (progress.emergencyRestorationAttempted) *reason = "copy_emergency_restore_ambiguous";
        else if (progress.restorationAttempted) *reason = "copy_final_barrier_ambiguous";
        else *reason = "copy_recording_exception";
        return false;
    }
}
