// Control Steam build 21225456. Streamline 2.14.1 selectable DLSS-G Multi Frame Generation + overlay.
// v1.0.0 is the first public release and freezes the v0.8.26 runtime-proven FG core.
 // persistent user settings plus a Control-native overlay with improved bottom spacing and effective multiplier/HDR status.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>
#include <new>
#include "target.h"

static_assert(sizeof(void*) == 8, "Build x64 only");
static HMODULE selfModule = nullptr, realDxgi = nullptr;
static INIT_ONCE loadOnce = INIT_ONCE_STATIC_INIT, probeOnce = INIT_ONCE_STATIC_INIT;
static SRWLOCK logLock = SRWLOCK_INIT;
static HANDLE logFile = INVALID_HANDLE_VALUE;
static LARGE_INTEGER frequency{};
static std::atomic<unsigned long long> aaCount{0}, presentCount{0}, beginCount{0}, hudCount{0};
static std::atomic<unsigned long long> aaFailures{0}, sampleCount{0};
static std::atomic<unsigned long long> baselineSampleCount{0}, transitionSampleCount{0};
static std::atomic<unsigned long long> traceCount{0}, hudRenderTraceCount{0}, hudPathSamples{0};
static std::atomic<unsigned long long> hudRenderCount{0}, lastAaObservedPresent{0};
static std::atomic<unsigned long long> hudTargetFrameSamples{0}, hudTargetRecords{0};
static std::atomic<unsigned long long> pathTraceUntilPresent{0};
using AA = bool (*)(void*, void*, void*, void*, void*, void*, void*, void*, void*, bool, double, double, float, float, float);
using VoidFn = void (*)();
using MemberVoidFn = void (*)(void*);
using RttCtorFn = void* (*)(void*, const void*, const void*);
using BoolFn = bool (*)();
static AA originalAA = nullptr;
static VoidFn originalPresent = nullptr, originalBegin = nullptr;
static MemberVoidFn originalHUD = nullptr, originalHUDRender = nullptr;
static RttCtorFn originalRttCtor = nullptr;
static BoolFn originalIsHDREnabled = nullptr;
static thread_local unsigned int nativeHudDepth = 0;
static thread_local unsigned int hudRttOrdinal = 0;
static thread_local bool hudTargetTrace = false;
static thread_local const void* hudPrimaryNative = nullptr;
static thread_local ID3D12Resource* hudPrimaryResource = nullptr;
static thread_local D3D12_RESOURCE_DESC hudPrimaryDesc{};
static thread_local unsigned int hudPrimaryTrackedState = 0;
static thread_local unsigned int hudPrimaryStateKnown = 0;
static HMODULE verifiedD3d = nullptr, verifiedRenderer = nullptr;

static void Log(const char* format, ...) noexcept {
    DWORD savedError = GetLastError();
    if (logFile != INVALID_HANDLE_VALUE) {
        char body[2048], line[2304];
        va_list args;
        va_start(args, format);
        _vsnprintf_s(body, sizeof(body), _TRUNCATE, format, args);
        va_end(args);
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        int n = _snprintf_s(line, sizeof(line), _TRUNCATE, "qpc=%lld tid=%lu %s\r\n", now.QuadPart, GetCurrentThreadId(), body);
        if (n > 0) {
            AcquireSRWLockExclusive(&logLock);
            DWORD written = 0;
            WriteFile(logFile, line, static_cast<DWORD>(n), &written, nullptr);
            ReleaseSRWLockExclusive(&logLock);
        }
    }
    SetLastError(savedError);
}

static std::wstring ModulePath(HMODULE module) {
    std::vector<wchar_t> path(32768);
    DWORD n = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (!n || n >= path.size()) return {};
    return std::wstring(path.data(), n);
}

static bool HashMatches(HMODULE module, const char* expected) {
    std::wstring path = ModulePath(module);
    if (path.empty()) return false;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0, received = 0;
    bool good = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (good) good = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &received, 0) >= 0;
    // Allocation stays bounded even if an unexpected crypto provider is present.
    if (objectSize > 1024 * 1024) good = false;
    std::vector<UCHAR> object(good ? objectSize : 0);
    if (good) good = BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) >= 0;
    UCHAR buffer[65536], digest[32];
    while (good) {
        DWORD n = 0;
        if (!ReadFile(file, buffer, sizeof(buffer), &n, nullptr)) { good = false; break; }
        if (!n) break;
        good = BCryptHashData(hash, buffer, n, 0) >= 0;
    }
    if (good) good = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    if (!good) return false;
    char hex[65]{};
    for (size_t i = 0; i < sizeof(digest); ++i) sprintf_s(hex + i * 2, 3, "%02X", digest[i]);
    return strcmp(hex, expected) == 0;
}

#include "streamline_bridge.h"
#include "hdr10_bridge.h"
#include "fg_overlay.h"

static void OpenLog() {
    wchar_t base[32768]{};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, _countof(base));
    if (!n || n >= _countof(base)) return;
    std::wstring dir = std::wstring(base) + L"\\ControlFGProbe";
    if (!CreateDirectoryW(dir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return;
    SYSTEMTIME time{};
    GetSystemTime(&time);
    wchar_t name[160];
    swprintf_s(name, L"\\probe-%04u%02u%02u-%02u%02u%02u-%03u-%lu.log",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, GetCurrentProcessId());
    logFile = CreateFileW((dir + name).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    QueryPerformanceFrequency(&frequency);
    Log("PROBE v1.0.0 target_steam_build=21225456 frequency=%lld source_revision=r1 fg_activation=resource_gated presentation_proxy=active common_constants=active frame_sync=active resource_tags=active backbuffer_index=every_present fg_baseline=working hdr10_bridge=transition_aware hdr_transition_watch=dormant_rgb10_to_fp16 mfg_mode=fixed_plus_native_dynamic default_multiplier=4x generated_frames_requested=3 target_frames_presented=4 max_selector=6x dynamic_mode=native_eDynamic dynamic_auto_target=explicit_game_monitor_refresh dynamic_manual_target=30-1000_fps dynamic_vsync_policy=syncinterval0_while_active dynamic_reflex_limiter=target_fps dynamic_pcl_simulation_start=control_beginframe_after_sleep dynamic_target_ui=auto_manual_thick_slider overlay=win32_layered_control_native_menu persistence=localappdata_ini overlay_status=selected,effective,current_fps,hdr,capability overlay_title=embedded_control_fg_logo_control_native overlay_font=bahnschrift_semicondensed overlay_selected=white_fill_black_text overlay_sections=control_red selector=off,dynamic,2x,3x,4x,5x,6x gpu_policy=rtx40_off_plus_2x_only transition_telemetry=segment_confirmed streamline_sdk=2.14.1", frequency.QuadPart);
    Log("LIMITS v1.0.0 public release; the v0.8.26 FG/HDR/Dynamic core remains frozen after runtime PASS. This build keeps the Control-native overlay and adds an RTX 40-series policy gate: Off and 2x remain selectable; Dynamic and 3x-6x are disabled and runtime-blocked. The v0.8.26 FG/HDR/Dynamic core remains otherwise unchanged. No generation, timing, camera, HUD, HDR conversion, or Control binary hooks are changed.");
}

// +0x88 was observed at multiple resource loads in the hash-locked doAntiAliasing
// body (RVA 0x1F990). No engine object fields are written. Keep SEH in this leaf
// function: no C++ objects requiring unwinding may be introduced here.
static bool Describe(void* texture, void** resource, D3D12_RESOURCE_DESC* desc, DWORD* fault) noexcept {
    *resource = nullptr;
    *fault = 0;
    __try {
        if (!texture) return false;
        auto p = *reinterpret_cast<ID3D12Resource**>(static_cast<unsigned char*>(texture) + 0x88);
        *resource = p;
        if (!p) return false;
        *desc = p->GetDesc();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        return false;
    }
}


struct NativeTextureStateSnapshot {
    void* resource;
    D3D12_RESOURCE_DESC desc;
    unsigned int trackedState;
    unsigned int stateKnown;
    unsigned int externalTracker;
    DWORD fault;
};

// Build-locked read-only state snapshot. Exact d3d disassembly shows the state
// tracker is *(NativeTexture+0x50), falling back to inline storage +0x40, with
// its D3D12_RESOURCE_STATES value at tracker+0x20. v0.7 carries this forward
// but never writes the state or resource.
static bool ReadNativeTextureState(const void* texture, NativeTextureStateSnapshot* out) noexcept {
    memset(out, 0, sizeof(*out));
    __try {
        if (!texture) return false;
        auto base = reinterpret_cast<const unsigned char*>(texture);
        auto resource = *reinterpret_cast<ID3D12Resource* const*>(base + 0x88);
        out->resource = resource;
        if (resource) out->desc = resource->GetDesc();
        auto tracker = *reinterpret_cast<const unsigned char* const*>(base + kNativeTextureStateOwnerOffset);
        if (tracker) out->externalTracker = 1;
        else tracker = base + kNativeTextureInlineStateOffset;
        out->trackedState = *reinterpret_cast<const unsigned int*>(tracker + kNativeTextureTrackedStateOffset);
        out->stateKnown = 1;
        return resource != nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        return false;
    }
}


struct EngineCommandContextSnapshot {
    DWORD tlsIndex;
    void* tlsBlock;
    void* context;
    void* commandList;
    unsigned int sourceTls;
    unsigned int commandType;
    HRESULT commandDeviceHr;
    HRESULT resourceDeviceHr;
    void* commandDevice;
    void* resourceDevice;
    unsigned int deviceMatch;
    DWORD fault;
};

// Read-only reconstruction of the command-context path used by the hash-locked
// d3d NativeTextureUtil::copy implementation. No command-list method that records
// GPU work is called: v0.7 only uses GetType and GetDevice.
static bool TryReadEngineCommandContextCandidate(unsigned char* context, unsigned int sourceTls,
                                                  EngineCommandContextSnapshot* out,
                                                  ID3D12Device** commandDeviceOut) noexcept {
    if (!context || reinterpret_cast<uintptr_t>(context) <= 0xFFFFu) return false;
    __try {
        auto commandList = *reinterpret_cast<ID3D12GraphicsCommandList**>(
            context + kD3dContextGraphicsCommandListOffset);
        if (!commandList || reinterpret_cast<uintptr_t>(commandList) <= 0xFFFFu) return false;
        const auto type = commandList->GetType();
        if (type != D3D12_COMMAND_LIST_TYPE_DIRECT &&
            type != D3D12_COMMAND_LIST_TYPE_BUNDLE &&
            type != D3D12_COMMAND_LIST_TYPE_COMPUTE &&
            type != D3D12_COMMAND_LIST_TYPE_COPY) return false;
        ID3D12Device* device = nullptr;
        const HRESULT hr = commandList->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&device));
        if (FAILED(hr) || !device) return false;
        out->context = context;
        out->sourceTls = sourceTls;
        out->commandList = commandList;
        out->commandType = unsigned(type);
        out->commandDeviceHr = hr;
        out->commandDevice = device;
        *commandDeviceOut = device;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool ReadEngineCommandContext(ID3D12Resource* candidate, EngineCommandContextSnapshot* out) noexcept {
    memset(out, 0, sizeof(*out));
    out->commandType = 0xFFFFFFFFu;
    out->commandDeviceHr = E_FAIL;
    out->resourceDeviceHr = E_FAIL;
    const DWORD saved = GetLastError();
    ID3D12Device* commandDevice = nullptr;
    ID3D12Device* resourceDevice = nullptr;
    bool good = false;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        if (!base) { SetLastError(saved); return false; }
        out->tlsIndex = *reinterpret_cast<const DWORD*>(base + kD3dTlsIndexRva);
        out->tlsBlock = TlsGetValue(out->tlsIndex);

        bool tlsGood = false;
        const uintptr_t tlsValue = reinterpret_cast<uintptr_t>(out->tlsBlock);
        if (tlsValue > 0xFFFFu) {
            unsigned char* tlsContext = nullptr;
            __try {
                tlsContext = *reinterpret_cast<unsigned char**>(
                    reinterpret_cast<unsigned char*>(out->tlsBlock) + kD3dTlsBlockContextPointerOffset);
            } __except (EXCEPTION_EXECUTE_HANDLER) { tlsContext = nullptr; }
            tlsGood = TryReadEngineCommandContextCandidate(tlsContext, 1, out, &commandDevice);
            if (!tlsGood && tlsContext) {
                Log("HUD_COMMAND_CONTEXT_TLS_REJECT tls_index=%lu tls_block=%p tls_context=%p reason=invalid_command_context",
                    static_cast<unsigned long>(out->tlsIndex), out->tlsBlock, tlsContext);
            }
        }
        if (!tlsGood) {
            auto globalContext = *reinterpret_cast<unsigned char**>(base + kD3dFallbackContextPointerRva);
            good = TryReadEngineCommandContextCandidate(globalContext, 0, out, &commandDevice);
        } else good = true;

        if (candidate) {
            out->resourceDeviceHr = candidate->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&resourceDevice));
            out->resourceDevice = resourceDevice;
        }
        out->deviceMatch = unsigned(commandDevice && resourceDevice && commandDevice == resourceDevice);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        good = false;
    }
    __try {
        if (resourceDevice) resourceDevice->Release();
        if (commandDevice) commandDevice->Release();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!out->fault) out->fault = GetExceptionCode();
        good = false;
    }
    SetLastError(saved);
    return good;
}

#include "semantic_capture.h"
#include "camera_capture.h"
#include "streamline_frame.h"

static void ExtendPathTrace(unsigned long long present, unsigned long long frames) noexcept {
    const unsigned long long target = present + frames;
    unsigned long long current = pathTraceUntilPresent.load();
    while (current < target && !pathTraceUntilPresent.compare_exchange_weak(current, target)) {}
}

static bool ShouldPathTrace(unsigned long long present) noexcept {
    return aaCount.load() > 0 && present <= pathTraceUntilPresent.load();
}

static void CaptureTextureArguments(unsigned long long call, void* const* textures,
                                    void** sampledResources, const char* phase) noexcept {
    for (unsigned i = 0; i < 9; ++i) {
        void* resource = nullptr;
        D3D12_RESOURCE_DESC desc{};
        DWORD fault = 0;
        if (Describe(textures[i], &resource, &desc, &fault)) {
            sampledResources[i] = resource;
            Log("RESOURCE call=%llu phase=%s slot=%u wrapper=%p resource=%p dimension=%u width=%llu height=%u depth_or_array=%u mips=%u format=%u samples=%u flags=%u",
                call, phase, i+1, textures[i], resource, unsigned(desc.Dimension), desc.Width,
                desc.Height, unsigned(desc.DepthOrArraySize), unsigned(desc.MipLevels),
                unsigned(desc.Format), desc.SampleDesc.Count, unsigned(desc.Flags));
        } else {
            Log("RESOURCE_UNAVAILABLE call=%llu phase=%s slot=%u wrapper=%p resource=%p exception=0x%08lX",
                call, phase, i+1, textures[i], resource, fault);
        }
    }
}

static bool HookAA(void* t1, void* t2, void* t3, void* t4, void* t5, void* t6,
                   void* t7, void* t8, void* t9, bool flag, double d1, double d2,
                   float f1, float f2, float f3) {
    const auto call = ++aaCount;
    const auto currentPresent = presentCount.load();
    const auto previousPresent = lastAaObservedPresent.exchange(currentPresent);
    const auto gapPresents = (previousPresent && currentPresent > previousPresent) ? currentPresent - previousPresent : 0;
    const bool resumeGap = previousPresent != 0 && gapPresents > 2;
    const bool startupCandidate = call <= 12;
    const bool periodicCandidate = (call % 240) == 0;
    const bool baselineSample = (startupCandidate || periodicCandidate) && baselineSampleCount.load() < 32;
    const bool resumeSample = resumeGap && transitionSampleCount.load() < 32;
    bool sample = baselineSample || resumeSample;

    if (baselineSample) ++baselineSampleCount;
    if (resumeSample) ++transitionSampleCount;
    if (sample) ++sampleCount;

    unsigned long long engineFrame = 0;
    DWORD frameFault = 0;
    const bool engineFrameKnown = ReadEngineFrameSafe(&engineFrame, &frameFault);
    RememberAaFrame(call, currentPresent, engineFrame);
    if (call == 1 || resumeGap) ExtendPathTrace(currentPresent, 12);

    CameraSnapshot slCamera{};
    DWORD slCameraFault = 0;
    const char* slCameraReason = nullptr;
    const bool slCameraKnown = ReadCamera(&slCamera, &slCameraFault, &slCameraReason);

    void* textures[] = {t1,t2,t3,t4,t5,t6,t7,t8,t9};
    void* sampledResources[9]{};
    const char* preReason = resumeGap ? "resume_gap" : startupCandidate ? "startup" : "periodic";
    DWORD saved = GetLastError();
    if (sample) {
        CaptureCamera(call, "pre_aa");
        Log("AA_ENTER call=%llu begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu flag=%u d1=%.9g d2=%.9g f1=%.9g f2=%.9g f3=%.9g gap_presents=%llu sample_reason_pre=%s engine_frame=%llu engine_frame_known=%u engine_frame_exception=0x%08lX",
            call, beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), flag ? 1u : 0u,
            d1, d2, double(f1), double(f2), double(f3), gapPresents, preReason,
            engineFrame, unsigned(engineFrameKnown), frameFault);
        CaptureTextureArguments(call, textures, sampledResources, "pre_aa");
    }
    SetLastError(saved);

    const bool result = originalAA(t1,t2,t3,t4,t5,t6,t7,t8,t9,flag,d1,d2,f1,f2,f3);
    MarkControlDLSSReady(call, result);

    // Reset is an NGX input populated inside the original DLSS wrapper, so read
    // it only after that wrapper returns. A pre-call read could be stale.
    int resetValue = 0;
    unsigned int resetStatus = 0;
    DWORD resetFault = 0;
    const bool resetQueried = ReadNgxReset(&resetValue, &resetStatus, &resetFault);
    const bool resetKnown = resetQueried && resetStatus == 1;
    const bool resetFrame = resetKnown && resetValue != 0;
    bool resetTransitionAdded = false;
    if (resetFrame && !resumeSample && transitionSampleCount.load() < 32) {
        ++transitionSampleCount;
        resetTransitionAdded = true;
    }
    if (resetFrame) ExtendPathTrace(currentPresent, 12);

    if (resetFrame && !sample && resetTransitionAdded) {
        sample = true;
        ++sampleCount;
        CaptureCamera(call, "post_aa_reset");
        Log("AA_POST_SAMPLE call=%llu reason=reset begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu engine_frame=%llu",
            call, beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), engineFrame);
        CaptureTextureArguments(call, textures, sampledResources, "post_aa_reset");
    }

    const char* sampleReason = resetFrame && resumeGap ? "reset_resume" :
                               resetFrame ? "reset" :
                               resumeGap ? "resume_gap" : preReason;
    if (resetFrame || resumeGap) {
        Log("AA_TRANSITION call=%llu reason=%s gap_presents=%llu reset_known=%u reset=%d reset_status=0x%08X reset_exception=0x%08lX begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu engine_frame=%llu",
            call, sampleReason, gapPresents, unsigned(resetKnown), resetValue, resetStatus, resetFault,
            beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), engineFrame);
    } else if ((!resetQueried || resetStatus != 1) && call <= 8) {
        Log("NGX_RESET_POST_UNAVAILABLE call=%llu queried=%u status=0x%08X exception=0x%08lX",
            call, unsigned(resetQueried), resetStatus, resetFault);
    }

    if (result) {
        SubmitSLCommonConstantsForAA(call, currentPresent, slCameraKnown, slCamera, resetKnown, resetValue);
        SubmitSLDepthMotionTagsForAA(call, currentPresent, textures, _countof(textures));
        if (!slCameraKnown && (call <= 12 || (call % 240) == 0)) {
            Log("SL_CAMERA_FRAME_UNAVAILABLE call=%llu present=%llu reason=%s exception=0x%08lX",
                call, currentPresent, slCameraReason ? slCameraReason : "unknown", slCameraFault);
        }
    }

    if (sample && result) CaptureSemantic(call, sampledResources, sampleReason);
    if (!result) {
        ++aaFailures;
        ExtendPathTrace(currentPresent, 12);
    }
    if (sample || (!result && aaFailures.load() <= 20)) {
        Log("AA_RETURN call=%llu success=%u failures=%llu reset_known=%u reset=%d gap_presents=%llu sample_reason=%s",
            call, result ? 1u : 0u, aaFailures.load(), unsigned(resetKnown), resetValue, gapPresents, sampleReason);
    }
    return result;
}

static bool TraceFrame(unsigned long long count) noexcept {
    const auto aa = aaCount.load();
    return (count <= 12 || (aa > 0 && aa <= 12)) && ++traceCount <= 128;
}

static void HookBegin() {
    auto count = ++beginCount;
    PrepareSLFrameToken(count);
    // This is Control's existing one-per-frame renderer begin boundary. The frame
    // token and Reflex sleep have already been issued above, matching NVIDIA's
    // documented ordering for the start-of-simulation/frame marker.
    MarkSLSimulationStart(count);
    bool trace = TraceFrame(count) || ShouldPathTrace(presentCount.load());
    if (trace) Log("BEGIN_ENTER count=%llu aa=%llu present=%llu hud_render=%llu", count, aaCount.load(), presentCount.load(), hudRenderCount.load());
    originalBegin();
    if (trace) Log("BEGIN_RETURN count=%llu aa=%llu present=%llu hud_render=%llu", count, aaCount.load(), presentCount.load(), hudRenderCount.load());
}

static void HookPresent() {
    auto count = ++presentCount;
    const bool windowTrace = ShouldPathTrace(count);
    const bool periodicTrace = aaCount.load() > 0 && (count % 121) == 0;
    const bool captureSwap = aaCount.load() > 0 && (windowTrace || periodicTrace) && swapSamples.load() < 160;
    bool enumerateAll = false;
    if (captureSwap) {
        ++swapSamples;
        if (swapFullEnumerations.load() < 2) {
            ++swapFullEnumerations;
            enumerateAll = true;
        }
        Log("SWAP_TRACE_TRIGGER present=%llu reason=%s sample=%llu enumerate_all=%u",
            count, windowTrace ? "transition_window" : "periodic_odd_cadence", swapSamples.load(), unsigned(enumerateAll));
        CaptureSwapChain(count, "pre_present", enumerateAll);
    }

    bool trace = TraceFrame(count) || windowTrace;
    if (trace) Log("PRESENT_ENTER count=%llu aa=%llu begin=%llu hud_dispatch=%llu hud_render=%llu", count, aaCount.load(), beginCount.load(), hudCount.load(), hudRenderCount.load());
    // DLSS-G's D3D path requires the application-facing proxied swap chain to
    // service GetCurrentBackBufferIndex every frame. Do this before the frame
    // gate and native Present so the first enabled FG frame satisfies it too.
    TouchSLCurrentBackBufferIndex(count);
    sl::FrameToken* slPresentToken = BeginSLPresentFrame(count);
    originalPresent();
    EndSLPresentFrame(count, slPresentToken);
    if (trace) Log("PRESENT_RETURN count=%llu aa=%llu begin=%llu hud_dispatch=%llu hud_render=%llu", count, aaCount.load(), beginCount.load(), hudCount.load(), hudRenderCount.load());
    TryConfigureSLNativeDeviceDeferred("post_present_after_control_dlss");

    if (captureSwap) CaptureSwapChain(count, "post_present", false);
    ProbeSLPresentState(count);
    if (count % 120 == 0 && count <= 36000) {
        Log("COUNTERS begin=%llu present=%llu aa=%llu aa_failures=%llu hud_dispatch=%llu hud_render=%llu samples=%llu transition_samples=%llu swap_samples=%llu hud_target_frames=%llu hud_target_records=%llu backbuffer_index_successes=%llu backbuffer_index_failures=%llu fg_ready_presents=%llu fg_enabled_presents=%llu generated_state_samples=%llu",
            beginCount.load(), count, aaCount.load(), aaFailures.load(), hudCount.load(), hudRenderCount.load(),
            sampleCount.load(), transitionSampleCount.load(), swapSamples.load(), hudTargetFrameSamples.load(), hudTargetRecords.load(),
            slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load(), slFgFrameReadyPresents.load(),
            slFgEnabledPresents.load(), slFgGeneratedStateSamples.load());
    }
}

static void HookHUD(void* object) {
    auto count = ++hudCount;
    bool trace = TraceFrame(count) || ShouldPathTrace(presentCount.load());
    if (trace) Log("HUD_DISPATCH_ENTER count=%llu object=%p begin=%llu present=%llu aa=%llu hud_render=%llu", count, object, beginCount.load(), presentCount.load(), aaCount.load(), hudRenderCount.load());
    originalHUD(object);
    if (trace) Log("HUD_DISPATCH_RETURN count=%llu begin=%llu present=%llu aa=%llu hud_render=%llu", count, beginCount.load(), presentCount.load(), aaCount.load(), hudRenderCount.load());
}

// v0.7 retains the exact two-NativeTexture RenderToTexture constructor imported
// by the hash-locked renderer. We only inspect calls while inside native HUD
// slot 17. Static callsites are RVA 0x134702 (primary scene/final target) and
// RVA 0x134870 (conditional separate HUD/UI target).
static void* HookRenderToTextureCtor(void* self, const void* color, const void* depth) {
    unsigned int ordinal = 0;
    if (nativeHudDepth) ordinal = ++hudRttOrdinal;

    // The first two constructors inside native HUD rendering are build-locked
    // callsites.  v0.8.14 proved ordinal 1 carries the full-resolution final
    // scene before UI and ordinal 2 aliases that same resource immediately
    // before the UI path.  Capture those two every current DLSS frame; keep the
    // verbose diagnostic logging bounded separately.
    if (nativeHudDepth && (ordinal == 1 || ordinal == 2)) {
        NativeTextureStateSnapshot c{}, d{};
        const bool colorOk = ReadNativeTextureState(color, &c);
        const bool depthOk = hudTargetTrace ? ReadNativeTextureState(depth, &d) : false;
        const auto currentPresent = presentCount.load();
        const bool currentDlssFrame = aaCount.load() > 0 && lastAaObservedPresent.load() == currentPresent;

        if (ordinal == 1) {
            hudPrimaryNative = color;
            hudPrimaryResource = colorOk ? static_cast<ID3D12Resource*>(c.resource) : nullptr;
            hudPrimaryDesc = c.desc;
            hudPrimaryTrackedState = c.trackedState;
            hudPrimaryStateKnown = c.stateKnown;
        } else {
            unsigned long long engineFrame = 0;
            DWORD frameFault = 0;
            ReadEngineFrameSafe(&engineFrame, &frameFault);
            EngineCommandContextSnapshot cmd{};
            const bool commandKnown = ReadEngineCommandContext(hudPrimaryResource, &cmd);
            const unsigned int direct = unsigned(commandKnown && cmd.commandType == D3D12_COMMAND_LIST_TYPE_DIRECT);
            const unsigned int aliasPrimary = unsigned(c.resource && hudPrimaryResource && c.resource == hudPrimaryResource);

            if (currentDlssFrame) {
                SubmitSLHUDLessTagForFrame(currentPresent, engineFrame, hudPrimaryResource,
                    hudPrimaryDesc, hudPrimaryTrackedState, hudPrimaryStateKnown != 0,
                    reinterpret_cast<ID3D12GraphicsCommandList*>(cmd.commandList), direct != 0, cmd.deviceMatch != 0);
            }

            if (hudTargetTrace && hudTargetRecords.load() < 224) {
                Log("HUD_COMMAND_CONTEXT present=%llu aa=%llu engine_frame=%llu tls_index=%lu tls_block=%p context=%p context_source=%s command_list=%p command_known=%u command_type=%u command_is_direct=%u command_device=%p command_device_hr=0x%08lX candidate_device=%p candidate_device_hr=0x%08lX device_match=%u exception=0x%08lX",
                    currentPresent, aaCount.load(), engineFrame, static_cast<unsigned long>(cmd.tlsIndex), cmd.tlsBlock, cmd.context,
                    cmd.sourceTls ? "tls" : "global_fallback", cmd.commandList, unsigned(commandKnown), cmd.commandType, direct,
                    cmd.commandDevice, static_cast<unsigned long>(cmd.commandDeviceHr), cmd.resourceDevice, static_cast<unsigned long>(cmd.resourceDeviceHr),
                    cmd.deviceMatch, cmd.fault);
                const void* knownBackbuffer = lastSwapBackbuffer.load();
                const void* dlssOutput = lastDlssOutput.load();
                Log("HUD_PRE_UI_CANDIDATE present=%llu aa=%llu engine_frame=%llu primary_native=%p primary_resource=%p width=%llu height=%u format=%u flags=%u tracked_state=0x%X state_known=%u second_resource=%p second_alias_primary=%u current_backbuffer=%p is_current_backbuffer=%u dlss_output=%p is_dlss_output=%u command_list=%p command_is_direct=%u device_match=%u current_dlss_frame=%u",
                    currentPresent, aaCount.load(), engineFrame, hudPrimaryNative, hudPrimaryResource, hudPrimaryDesc.Width, hudPrimaryDesc.Height,
                    unsigned(hudPrimaryDesc.Format), unsigned(hudPrimaryDesc.Flags), hudPrimaryTrackedState, hudPrimaryStateKnown, c.resource, aliasPrimary,
                    knownBackbuffer, unsigned(hudPrimaryResource && knownBackbuffer && hudPrimaryResource == knownBackbuffer), dlssOutput,
                    unsigned(hudPrimaryResource && dlssOutput && hudPrimaryResource == dlssOutput), cmd.commandList, direct, cmd.deviceMatch, unsigned(currentDlssFrame));
            }
        }

        if (hudTargetTrace && hudTargetRecords.load() < 224) {
            ++hudTargetRecords;
            const char* role = ordinal == 1 ? "primary_scene_or_final" : "separate_hud_ui";
            const void* knownBackbuffer = lastSwapBackbuffer.load();
            const void* dlssOutput = lastDlssOutput.load();
            unsigned long long engineFrame = 0;
            DWORD frameFault = 0;
            const bool engineFrameKnown = ReadEngineFrameSafe(&engineFrame, &frameFault);
            Log("HUD_RTT_TARGET ordinal=%u role=%s self=%p color_native=%p color_resource=%p color_ok=%u width=%llu height=%u format=%u flags=%u tracked_state=0x%X state_known=%u external_tracker=%u exception=0x%08lX depth_native=%p depth_resource=%p depth_ok=%u depth_format=%u depth_state=0x%X depth_state_known=%u is_current_backbuffer=%u backbuffer_index=%u backbuffer_present=%llu is_dlss_output=%u present=%llu aa=%llu engine_frame=%llu engine_frame_known=%u engine_frame_exception=0x%08lX current_dlss_frame=%u",
                ordinal, role, self, color, c.resource, unsigned(colorOk), c.desc.Width, c.desc.Height, unsigned(c.desc.Format), unsigned(c.desc.Flags),
                c.trackedState, c.stateKnown, c.externalTracker, c.fault, depth, d.resource, unsigned(depthOk), unsigned(d.desc.Format), d.trackedState, d.stateKnown,
                unsigned(c.resource && knownBackbuffer && c.resource == knownBackbuffer), lastSwapBackbufferIndex.load(), lastSwapBackbufferPresent.load(),
                unsigned(c.resource && dlssOutput && c.resource == dlssOutput), currentPresent, aaCount.load(), engineFrame, unsigned(engineFrameKnown), frameFault,
                unsigned(currentDlssFrame));
        }
    }
    return originalRttCtor(self, color, depth);
}

// This is the execution target of the packet queued by renderHUD. Static v0.4
// evidence maps primary RendererX86 vtable slot 17 to RVA 0x1343B0. Hooking it
// gives the renderer-thread CPU boundary; it still does not prove GPU completion.
static void HookHUDRender(void* object) {
    auto count = ++hudRenderCount;
    const auto present = presentCount.load();
    const bool windowTrace = ShouldPathTrace(present);
    const bool trace = (windowTrace || count <= 16) && ++hudRenderTraceCount <= 96;
    const bool targetTrace = aaCount.load() > 0 && (windowTrace || hudTargetFrameSamples.load() < 64);
    if (targetTrace && hudTargetFrameSamples.load() < 96) ++hudTargetFrameSamples;
    unsigned long long engineFrame = 0;
    DWORD frameFault = 0;
    const bool engineFrameKnown = trace ? ReadEngineFrameSafe(&engineFrame, &frameFault) : false;
    if (trace) {
        Log("HUD_RENDER_ENTER count=%llu object=%p engine_frame=%llu engine_frame_known=%u begin=%llu present=%llu aa=%llu hud_dispatch=%llu",
            count, object, engineFrame, unsigned(engineFrameKnown), beginCount.load(), present, aaCount.load(), hudCount.load());
    }
    if (targetTrace && hudPathSamples.load() < 64) {
        ++hudPathSamples;
        CaptureSwapChain(present, "hud_render_enter", false);
    }
    ++nativeHudDepth;
    hudRttOrdinal = 0;
    hudPrimaryNative = nullptr;
    hudPrimaryResource = nullptr;
    hudPrimaryDesc = {};
    hudPrimaryTrackedState = 0;
    hudPrimaryStateKnown = 0;
    hudTargetTrace = targetTrace;
    originalHUDRender(object);
    if (targetTrace) {
        Log("HUD_RTT_FRAME_SUMMARY hud_render=%llu present=%llu aa=%llu constructors=%u last_backbuffer=%p backbuffer_index=%u dlss_output=%p",
            count, presentCount.load(), aaCount.load(), hudRttOrdinal, lastSwapBackbuffer.load(), lastSwapBackbufferIndex.load(), lastDlssOutput.load());
    }
    hudTargetTrace = false;
    hudRttOrdinal = 0;
    --nativeHudDepth;
    if (trace) {
        Log("HUD_RENDER_RETURN count=%llu engine_frame=%llu begin=%llu present=%llu aa=%llu hud_dispatch=%llu",
            count, engineFrame, beginCount.load(), presentCount.load(), aaCount.load(), hudCount.load());
    }
}

struct Patch { void** slot; void* original; void* hook; const char* label; };

// Traverses the ordinary import table of an already loaded, hash-verified PE.
// This does not alter code bytes or guess internal function addresses.
static void** FindImport(HMODULE module, const char* dll, const char* name) {
    auto base = reinterpret_cast<unsigned char*>(module);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return nullptr;
    const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress || !directory.Size) return nullptr;
    auto entry = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
    for (size_t i = 0; i < directory.Size / sizeof(*entry) && entry[i].Name; ++i) {
        if (_stricmp(reinterpret_cast<char*>(base + entry[i].Name), dll)) continue;
        if (!entry[i].OriginalFirstThunk || !entry[i].FirstThunk) return nullptr;
        auto names = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + entry[i].OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + entry[i].FirstThunk);
        for (size_t j = 0; names[j].u1.AddressOfData; ++j) {
            if (IMAGE_SNAP_BY_ORDINAL64(names[j].u1.Ordinal)) continue;
            auto symbol = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[j].u1.AddressOfData);
            if (!strcmp(symbol->Name, name)) return reinterpret_cast<void**>(&addresses[j].u1.Function);
        }
    }
    return nullptr;
}

static void** FindImportOrdinal(HMODULE module, const char* dll, unsigned short ordinal) {
    auto base = reinterpret_cast<unsigned char*>(module);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return nullptr;
    const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress || !directory.Size) return nullptr;
    auto entry = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
    for (size_t i = 0; i < directory.Size / sizeof(*entry) && entry[i].Name; ++i) {
        if (_stricmp(reinterpret_cast<char*>(base + entry[i].Name), dll)) continue;
        if (!entry[i].OriginalFirstThunk || !entry[i].FirstThunk) return nullptr;
        auto names = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + entry[i].OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + entry[i].FirstThunk);
        for (size_t j = 0; names[j].u1.AddressOfData; ++j) {
            if (!IMAGE_SNAP_BY_ORDINAL64(names[j].u1.Ordinal)) continue;
            if (IMAGE_ORDINAL64(names[j].u1.Ordinal) == ordinal)
                return reinterpret_cast<void**>(&addresses[j].u1.Function);
        }
    }
    return nullptr;
}

static bool Exchange(Patch& p, bool install) {
    DWORD old = 0;
    if (!VirtualProtect(p.slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
    void* expected = install ? p.original : p.hook;
    void* next = install ? p.hook : p.original;
    void* previous = InterlockedCompareExchangePointer(p.slot, next, expected);
    DWORD ignored = 0;
    if (!VirtualProtect(p.slot, sizeof(void*), old, &ignored))
        Log("PROTECTION_RESTORE_FAILED label=%s error=%lu", p.label, GetLastError());
    return previous == expected;
}

static BOOL CALLBACK Configure(PINIT_ONCE, PVOID, PVOID*) noexcept {
    try {
        OpenLog();
        if (logFile == INVALID_HANDLE_VALUE) return TRUE; // No logging => no hooks.
        auto exe = GetModuleHandleW(nullptr);
        auto d3d = GetModuleHandleW(L"d3d_rmdwin10_f.dll");
        auto renderer = GetModuleHandleW(L"renderer_rmdwin10_f.dll");
        if (!exe || !d3d || !renderer) { Log("PROBE_DISABLED required_module_missing"); return TRUE; }
        if (!HashMatches(exe, kExeHash) || !HashMatches(d3d, kD3dHash) || !HashMatches(renderer, kRendererHash)) {
            Log("PROBE_DISABLED target_hash_mismatch_or_file_unreadable"); return TRUE;
        }
        Log("TARGET_HASHES_MATCH");
        verifiedD3d = d3d;
        verifiedRenderer = renderer;
        if (!ResolveSemanticGetters(d3d)) {
            Log("PROBE_DISABLED semantic_getter_missing"); return TRUE;
        }
        originalAA = reinterpret_cast<AA>(GetProcAddress(d3d, kAASymbol));
        originalPresent = reinterpret_cast<VoidFn>(GetProcAddress(d3d, kPresentSymbol));
        originalBegin = reinterpret_cast<VoidFn>(GetProcAddress(d3d, kBeginSymbol));
        originalHUD = reinterpret_cast<MemberVoidFn>(GetProcAddress(renderer, kHUDSymbol));
        originalRttCtor = reinterpret_cast<RttCtorFn>(GetProcAddress(d3d, kRenderToTextureCtorSymbol));
        originalIsHDREnabled = reinterpret_cast<BoolFn>(GetProcAddress(d3d, kIsHdrEnabledSymbol));
        if (!originalRttCtor || !originalIsHDREnabled) {
            Log("PROBE_DISABLED required_d3d_export_missing rtt_ctor=%p hdr_query=%p", originalRttCtor, originalIsHDREnabled); return TRUE;
        }
        auto rendererBase = reinterpret_cast<unsigned char*>(renderer);
        auto nativeHudSlot = reinterpret_cast<void**>(rendererBase + kRendererVtableRva + kRendererHudVtableSlot * sizeof(void*));
        originalHUDRender = reinterpret_cast<MemberVoidFn>(rendererBase + kRendererHudTargetRva);

        auto d3d12CreateSlot = FindImportOrdinal(d3d, "d3d12.dll", kD3D12CreateDeviceOrdinal);
        auto d3d12Module = GetModuleHandleW(L"d3d12.dll");
        auto d3d12NamedCreate = d3d12Module ? reinterpret_cast<void*>(GetProcAddress(d3d12Module, "D3D12CreateDevice")) : nullptr;
        slOriginalD3D12CreateDevice = d3d12CreateSlot ? reinterpret_cast<D3D12CreateDeviceFn>(*d3d12CreateSlot) : nullptr;
        Log("D3D12_CREATE_DEVICE_TARGET slot=%p original=%p named_export=%p ordinal=%u expected_iat_rva=0x%zX",
            d3d12CreateSlot, reinterpret_cast<void*>(slOriginalD3D12CreateDevice), d3d12NamedCreate,
            unsigned(kD3D12CreateDeviceOrdinal), kD3D12CreateDeviceImportRva);
        if (!d3d12CreateSlot || !slOriginalD3D12CreateDevice || !d3d12NamedCreate ||
            reinterpret_cast<void*>(slOriginalD3D12CreateDevice) != d3d12NamedCreate) {
            Log("PROBE_DISABLED d3d12_create_device_import_mismatch"); return TRUE;
        }
        if (!ValidateControlNgxInitCallsite(d3d)) {
            Log("PROBE_DISABLED control_ngx_init_callsite_mismatch"); return TRUE;
        }
        if (!ValidateControlCommandQueueCtorCallsite(d3d)) {
            Log("PROBE_DISABLED control_command_queue_ctor_callsite_mismatch"); return TRUE;
        }

        Patch patches[] = {
            {FindImport(renderer, "d3d_rmdwin10_f.dll", kAASymbol), reinterpret_cast<void*>(originalAA), reinterpret_cast<void*>(&HookAA), "DLSS_AA"},
            {FindImport(renderer, "d3d_rmdwin10_f.dll", kPresentSymbol), reinterpret_cast<void*>(originalPresent), reinterpret_cast<void*>(&HookPresent), "PRESENT"},
            {FindImport(renderer, "d3d_rmdwin10_f.dll", kBeginSymbol), reinterpret_cast<void*>(originalBegin), reinterpret_cast<void*>(&HookBegin), "BEGIN"},
            {FindImport(exe, "renderer_rmdwin10_f.dll", kHUDSymbol), reinterpret_cast<void*>(originalHUD), reinterpret_cast<void*>(&HookHUD), "HUD_DISPATCH"},
            {nativeHudSlot, reinterpret_cast<void*>(originalHUDRender), reinterpret_cast<void*>(&HookHUDRender), "HUD_RENDER"},
            {FindImport(renderer, "d3d_rmdwin10_f.dll", kRenderToTextureCtorSymbol), reinterpret_cast<void*>(originalRttCtor), reinterpret_cast<void*>(&HookRenderToTextureCtor), "HUD_RTT_CTOR"},
            {d3d12CreateSlot, reinterpret_cast<void*>(slOriginalD3D12CreateDevice), reinterpret_cast<void*>(&HookD3D12CreateDevice), "D3D12_CREATE_DEVICE"}
        };
        // All nine build-locked targets (seven pointer slots plus the exact Control
        // NGX and command-queue-constructor rel32 callsites) validate before the first target write.
        // The device hook is non-blocking during slInit (bootstrap_state=1), so
        // Streamline core bootstrap remains safely deferred until after factory return.
        for (auto& p : patches) {
            if (!p.slot || !p.original || *p.slot != p.original) {
                Log("PROBE_DISABLED unexpected_hook_target label=%s", p.label); return TRUE;
            }
        }
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&HookAA), &pinned)) {
            Log("PROBE_DISABLED module_pin_failed"); return TRUE;
        }
        if (!PrepareControlNgxInitRelay()) {
            Log("PROBE_DISABLED control_ngx_init_relay_unavailable"); return TRUE;
        }
        if (!PrepareControlCommandQueueCtorRelay()) {
            Log("PROBE_DISABLED control_command_queue_ctor_relay_unavailable");
            RollbackControlNgxInitCallHook();
            return TRUE;
        }
        size_t applied = 0;
        for (; applied < _countof(patches); ++applied) {
            if (!Exchange(patches[applied], true)) break;
            Log("HOOK_INSTALLED label=%s", patches[applied].label);
        }
        if (applied != _countof(patches)) {
            Log("PROBE_DISABLED installation_failed_rolling_back");
            while (applied) {
                --applied;
                if (!Exchange(patches[applied], false)) Log("ROLLBACK_FAILED label=%s", patches[applied].label);
            }
            RollbackControlCommandQueueCtorCallHook();
            RollbackControlNgxInitCallHook();
            return TRUE;
        }
        if (!InstallControlNgxInitCallHook()) {
            Log("PROBE_DISABLED control_ngx_init_hook_install_failed_rolling_back");
            while (applied) {
                --applied;
                if (!Exchange(patches[applied], false)) Log("ROLLBACK_FAILED label=%s", patches[applied].label);
            }
            RollbackControlCommandQueueCtorCallHook();
            RollbackControlNgxInitCallHook();
            return TRUE;
        }
        Log("HOOK_INSTALLED label=CONTROL_NGX_INIT_PROJECT");
        if (!InstallControlCommandQueueCtorCallHook()) {
            Log("PROBE_DISABLED control_command_queue_ctor_hook_install_failed_rolling_back");
            RollbackControlCommandQueueCtorCallHook();
            RollbackControlNgxInitCallHook();
            while (applied) {
                --applied;
                if (!Exchange(patches[applied], false)) Log("ROLLBACK_FAILED label=%s", patches[applied].label);
            }
            return TRUE;
        }
        Log("HOOK_INSTALLED label=CONTROL_COMMAND_QUEUE_CTOR");
        StartFGOverlay();
        Log("PROBE_ACTIVE hooks=9 fg_activation=resource_gated presentation_proxy=active common_constants=per_dlss_frame frame_tokens=begin_indexed pcl_present_markers=frame_gated reflex_mode=low_latency reflex_sleep=active streamline_sdk=2.14.1 streamline_bootstrap=deferred_post_native_factory bootstrap_state=%u semantic_capture=post_eval swapchain_capture=mode_change_aware camera_capture=pre_aa hud_render_hook=native_slot17 hud_rtt_hook=two_native_texture_ctor command_context=pre_ui_readonly_tls_sentinel_safe host_device=native queue_route=exact_constructor_only control_ngx_feature_path=ControlFGStreamline streamline_ngx_paths=runtime_plus_game device_bind=early_before_control_ngx factory_upgrade=presentation_only swapchain_upgrade=via_factory command_queue_proxy=exact_ctor_private_device features_requested=3 features=reflex,pcl,dlssg dlssg_loaded_expected=1 resource_tags=depth_mv_plus_hudless_sdr hdr10_resource_tags=depth_mv_only_optional_hudless_off backbuffer_index=every_present fg_baseline=working mfg_mode=fixed_plus_native_dynamic default_multiplier=4x max_selector=6x dynamic_mode=native_eDynamic dynamic_auto_target=explicit_game_monitor_refresh dynamic_manual_target=30-1000_fps dynamic_vsync_policy=syncinterval0_while_active dynamic_reflex_limiter=target_fps dynamic_target_ui=auto_manual_thick_slider overlay=win32_layered_control_native_menu persistence=localappdata_ini overlay_status=selected,effective,current_fps,hdr,capability overlay_title=embedded_control_fg_logo_control_native overlay_font=bahnschrift_semicondensed overlay_selected=white_fill_black_text overlay_sections=control_red selector=off,dynamic,2x,3x,4x,5x,6x gpu_policy=rtx40_off_plus_2x_only transition_telemetry=segment_confirmed hdr10_bridge=fp16_scrgb_shadow_to_rgb10_pq project_id=305914b8-cf5b-4535-8e53-5589bf8cefa5 device_set=%u hdr_query=enabled transition_capture=enabled", slBootstrapState.load(), slDeviceConfigured.load());
    } catch (...) {
        Log("PROBE_DISABLED initialization_exception");
    }
    return TRUE;
}

static BOOL CALLBACK LoadReal(PINIT_ONCE, PVOID, PVOID*) noexcept {
    wchar_t path[32768];
    UINT n = GetSystemDirectoryW(path, _countof(path));
    if (!n || n + 10 >= _countof(path)) return TRUE;
    wcscat_s(path, L"\\dxgi.dll");
    realDxgi = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (realDxgi == selfModule) realDxgi = nullptr;
    return TRUE;
}
static FARPROC Real(const char* name) noexcept {
    InitOnceExecuteOnce(&loadOnce, LoadReal, nullptr, nullptr);
    return realDxgi ? GetProcAddress(realDxgi, name) : nullptr;
}
static void StartProbe() noexcept { InitOnceExecuteOnce(&probeOnce, Configure, nullptr, nullptr); }

// Export aliases live in dxgi.def. No DXGI/D3D calls or hook work in DllMain.
extern "C" HRESULT WINAPI ProbeCreateDXGIFactory(REFIID iid, void** factory) {
    auto fn = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(Real("CreateDXGIFactory"));
    if (!fn) { if (factory) *factory = nullptr; return E_NOINTERFACE; }
    StartProbe();
    const HRESULT hr = fn(iid, factory);
    if (SUCCEEDED(hr)) {
        Log("SL_FACTORY_NATIVE entrypoint=CreateDXGIFactory factory=%p isolation=early_bind_native_interface bootstrap_state=%u", factory ? *factory : nullptr, slBootstrapState.load());
        InitializeStreamlineCoreAfterFactory("CreateDXGIFactory");
        const bool upgraded = UpgradeFactoryForPresentation(factory, "CreateDXGIFactory");
        const bool hdrWrapped = upgraded ? WrapFactoryForHdr10Bridge(factory, iid, "CreateDXGIFactory") : false;
        Log("SL_FACTORY_RETURN entrypoint=CreateDXGIFactory factory=%p proxy_upgrade=%u hdr_factory_wrap=%u", factory ? *factory : nullptr, unsigned(upgraded), unsigned(hdrWrapped));
    }
    return hr;
}
extern "C" HRESULT WINAPI ProbeCreateDXGIFactory1(REFIID iid, void** factory) {
    auto fn = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(Real("CreateDXGIFactory1"));
    if (!fn) { if (factory) *factory = nullptr; return E_NOINTERFACE; }
    StartProbe();
    const HRESULT hr = fn(iid, factory);
    if (SUCCEEDED(hr)) {
        Log("SL_FACTORY_NATIVE entrypoint=CreateDXGIFactory1 factory=%p isolation=early_bind_native_interface bootstrap_state=%u", factory ? *factory : nullptr, slBootstrapState.load());
        InitializeStreamlineCoreAfterFactory("CreateDXGIFactory1");
        const bool upgraded = UpgradeFactoryForPresentation(factory, "CreateDXGIFactory1");
        const bool hdrWrapped = upgraded ? WrapFactoryForHdr10Bridge(factory, iid, "CreateDXGIFactory1") : false;
        Log("SL_FACTORY_RETURN entrypoint=CreateDXGIFactory1 factory=%p proxy_upgrade=%u hdr_factory_wrap=%u", factory ? *factory : nullptr, unsigned(upgraded), unsigned(hdrWrapped));
    }
    return hr;
}
extern "C" HRESULT WINAPI ProbeCreateDXGIFactory2(UINT flags, REFIID iid, void** factory) {
    auto fn = reinterpret_cast<HRESULT(WINAPI*)(UINT, REFIID, void**)>(Real("CreateDXGIFactory2"));
    if (!fn) { if (factory) *factory = nullptr; return E_NOINTERFACE; }
    StartProbe();
    const HRESULT hr = fn(flags, iid, factory);
    if (SUCCEEDED(hr)) {
        Log("SL_FACTORY_NATIVE entrypoint=CreateDXGIFactory2 factory=%p isolation=early_bind_native_interface bootstrap_state=%u", factory ? *factory : nullptr, slBootstrapState.load());
        InitializeStreamlineCoreAfterFactory("CreateDXGIFactory2");
        const bool upgraded = UpgradeFactoryForPresentation(factory, "CreateDXGIFactory2");
        const bool hdrWrapped = upgraded ? WrapFactoryForHdr10Bridge(factory, iid, "CreateDXGIFactory2") : false;
        Log("SL_FACTORY_RETURN entrypoint=CreateDXGIFactory2 factory=%p proxy_upgrade=%u hdr_factory_wrap=%u", factory ? *factory : nullptr, unsigned(upgraded), unsigned(hdrWrapped));
    }
    return hr;
}
extern "C" HRESULT WINAPI ProbeDXGIGetDebugInterface1(UINT flags, REFIID iid, void** debug) {
    auto fn = reinterpret_cast<HRESULT(WINAPI*)(UINT, REFIID, void**)>(Real("DXGIGetDebugInterface1"));
    if (!fn) { if (debug) *debug = nullptr; return E_NOINTERFACE; }
    return fn(flags, iid, debug);
}
extern "C" HRESULT WINAPI ProbeDXGIDeclareAdapterRemovalSupport() {
    auto fn = reinterpret_cast<HRESULT(WINAPI*)()>(Real("DXGIDeclareAdapterRemovalSupport"));
    return fn ? fn() : E_NOINTERFACE;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) selfModule = instance;
    return TRUE;
}
