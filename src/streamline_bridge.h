#pragma once

// Streamline 2.14.1 selectable Multi Frame Generation path for Control v1.0.0.
// v1.0.0 public release freezes the v0.8.26 FG core that passed fixed 2x-6x, native Dynamic
// MFG, live HDR/SDR transitions, explicit Auto/manual target FPS, Dynamic
// SyncInterval handling, Reflex frame limiting, and PCL SimulationStart.
// This revision adds an RTX 40-series policy gate: Off and 2x remain available;
// Dynamic and fixed 3x-6x are UI-disabled and runtime-blocked.
#include <wintrust.h>
#include <softpub.h>
#include <sl.h>
#include <sl_pcl.h>
#include <sl_reflex.h>
#include <sl_dlss_g.h>

using D3D12CreateDeviceFn = HRESULT (WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
using SLUpgradeInterfaceFn = sl::Result(void**);

static HMODULE slInterposerModule = nullptr;
static std::wstring slRuntimeDirectory;
static std::wstring slGameDirectory;
static std::wstring slLogDirectory;
static const wchar_t* slPluginPaths[2]{};
static PFun_slInit* slInitApi = nullptr;
static PFun_slSetD3DDevice* slSetD3DDeviceApi = nullptr;
static PFun_slIsFeatureSupported* slIsFeatureSupportedApi = nullptr;
static PFun_slIsFeatureLoaded* slIsFeatureLoadedApi = nullptr;
static PFun_slGetNativeInterface* slGetNativeInterfaceApi = nullptr;
static SLUpgradeInterfaceFn* slUpgradeInterfaceApi = nullptr;
static PFun_slGetFeatureFunction* slGetFeatureFunctionApi = nullptr;
static PFun_slSetConstants* slSetConstantsApi = nullptr;
static PFun_slSetTagForFrame* slSetTagForFrameApi = nullptr;
static PFun_slGetNewFrameToken* slGetNewFrameTokenApi = nullptr;
static PFun_slPCLSetMarker* slPCLSetMarkerApi = nullptr;
static PFun_slReflexSetOptions* slReflexSetOptionsApi = nullptr;
static PFun_slReflexSleep* slReflexSleepApi = nullptr;
static PFun_slDLSSGSetOptions* slDLSSGSetOptionsApi = nullptr;
static PFun_slDLSSGGetState* slDLSSGGetStateApi = nullptr;
static constexpr const char* kControlProjectId = "305914b8-cf5b-4535-8e53-5589bf8cefa5";
static constexpr const char* kControlEngineVersion = "1.0";
static const sl::Feature slFeaturesToLoad[] = { sl::kFeatureReflex, sl::kFeaturePCL, sl::kFeatureDLSS_G };
static D3D12CreateDeviceFn slOriginalD3D12CreateDevice = nullptr;

// Minimal ABI-compatible subset of NGX definitions required only to augment the
// FeatureCommonInfo path list on Control's one build-locked Init_with_ProjectID call.
struct ControlNgxPathListInfo {
    const wchar_t* const* Path;
    unsigned int Length;
};
struct ControlNgxLoggingInfo {
    void* LoggingCallback;
    int MinimumLoggingLevel;
    bool DisableOtherLoggingSinks;
};
struct ControlNgxFeatureCommonInfo {
    ControlNgxPathListInfo PathListInfo;
    void* InternalData;
    ControlNgxLoggingInfo LoggingInfo;
};
static_assert(sizeof(ControlNgxFeatureCommonInfo) == 40, "Unexpected NGX FeatureCommonInfo ABI");

using ControlNgxInitProjectFn = unsigned int (*)(
    const char*, int, const char*, const wchar_t*, ID3D12Device*,
    const ControlNgxFeatureCommonInfo*, unsigned int);

static ControlNgxInitProjectFn controlOriginalNgxInitProject = nullptr;
static unsigned char* controlNgxInitCallsite = nullptr;
static void* controlNgxInitRelay = nullptr;
static unsigned char controlNgxInitOriginalCall[5]{};
static std::atomic<unsigned int> controlNgxPathAugmentCalls{0};

// 0 = not started, 1 = in progress, 2 = initialized, 3 = failed/disabled.
static std::atomic<unsigned int> slBootstrapState{0};
static std::atomic<unsigned int> slDeviceConfigured{0};
static std::atomic<unsigned int> slDeviceBindAttempted{0};
static std::atomic<unsigned int> slControlDlssReady{0};
static std::atomic<ID3D12Device*> slPendingNativeDevice{nullptr};
static std::atomic<ID3D12Device*> slPrivateDeviceProxy{nullptr};
static std::atomic<ID3D12Device*> slPrivateDeviceNativeIdentity{nullptr};
static std::atomic<unsigned int> slQueueRouteCalls{0};
static std::atomic<ID3D12CommandQueue*> slHdr10DirectQueue{nullptr};
static SRWLOCK slQueueRouteLock = SRWLOCK_INIT;

using ControlCommandQueueCtorFn = void* (*)(void*, unsigned int);
static ControlCommandQueueCtorFn controlOriginalCommandQueueCtor = nullptr;
static unsigned char* controlCommandQueueCtorCallsite = nullptr;
static void* controlCommandQueueCtorRelay = nullptr;
static unsigned char controlCommandQueueCtorOriginalCall[5]{};
static std::atomic<unsigned long long> slPresentStateSamples{0};
static std::atomic<unsigned int> slFgOptionsInitialized{0};
static std::atomic<unsigned int> slFgEnabledByApi{0};
static std::atomic<unsigned long long> slFgOptionsCalls{0};
static std::atomic<unsigned int> slFactoryUpgradeAttempts{0};
static std::atomic<unsigned int> slFactoryUpgradeSuccesses{0};
static std::atomic<unsigned int> slReflexOptionsReady{0};
// UINT32_MAX is the not-yet-applied sentinel. Reflex frameLimitUs=0 explicitly
// means no Reflex frame-rate limit, so zero must remain a normal cached value.
static std::atomic<unsigned int> slReflexAppliedFrameLimitUs{0xFFFFFFFFu};
static std::atomic<unsigned long long> slReflexOptionsCalls{0};
static std::atomic<unsigned long long> slDynamicReflexLimiterChanges{0};
static SRWLOCK slReflexOptionsLock = SRWLOCK_INIT;
static std::atomic<unsigned long long> slFrameTokenAttempts{0};
static std::atomic<unsigned long long> slFrameTokenSuccesses{0};
static std::atomic<unsigned long long> slReflexSleepCalls{0};
static std::atomic<unsigned long long> slSimulationMarkerAttempts{0};
static std::atomic<unsigned long long> slSimulationMarkerStarts{0};
static std::atomic<unsigned long long> slSimulationMarkerFailures{0};
static std::atomic<unsigned long long> slPresentMarkerStarts{0};
static std::atomic<unsigned long long> slPresentMarkerEnds{0};
static std::atomic<unsigned long long> slConstantsSuccesses{0};
static std::atomic<unsigned long long> slConstantsDuplicateSkips{0};
static std::atomic<unsigned long long> slResourceTagCalls{0};
static std::atomic<unsigned long long> slResourceTagSuccesses{0};
static std::atomic<unsigned long long> slResourceTagFailures{0};
static std::atomic<unsigned int> slFgMvecDepthWidth{0};
static std::atomic<unsigned int> slFgMvecDepthHeight{0};
static std::atomic<unsigned int> slFgDepthFormat{0};
static std::atomic<unsigned int> slFgMvecFormat{0};
static std::atomic<unsigned int> slFgColorWidth{0};
static std::atomic<unsigned int> slFgColorHeight{0};
static std::atomic<unsigned int> slFgColorFormat{0};
static std::atomic<unsigned int> slFgHudLessFormat{0};
static std::atomic<unsigned int> slFirstGeneratedFrameLogged{0};
static std::atomic<unsigned long long> slBackBufferIndexAttempts{0};
static std::atomic<unsigned long long> slBackBufferIndexSuccesses{0};
static std::atomic<unsigned long long> slBackBufferIndexFailures{0};
static std::atomic<unsigned int> slLastBackBufferIndex{0xFFFFFFFFu};
static std::atomic<unsigned long long> slFgFrameReadyPresents{0};
static std::atomic<unsigned long long> slFgEnabledPresents{0};
static std::atomic<unsigned long long> slFgGeneratedStateSamples{0};
static std::atomic<unsigned long long> slFgTargetMultiplierStateSamples{0};
static std::atomic<unsigned long long> slFgDynamicGeneratedStateSamples{0};
static std::atomic<unsigned int> slFgLastFramesPresented{0};
static std::atomic<unsigned long long> slFgLastStatePresent{0};
static std::atomic<unsigned int> slFgLastStateSelection{0xFFFFFFFFu};
static std::atomic<unsigned int> slFirstTargetMultiplierFrameLogged{0};
static std::atomic<unsigned int> slFirst4xFrameLogged{0};
static std::atomic<unsigned int> slFirst5xFrameLogged{0};
static std::atomic<unsigned int> slFirst6xFrameLogged{0};
static std::atomic<unsigned int> slFirstDynamicFrameLogged{0};
static std::atomic<unsigned int> slMfgCapabilityKnown{0};
static std::atomic<unsigned int> slMfgCapabilitySupported{0};
static std::atomic<unsigned int> slMfgMaxGenerated{0};
static std::atomic<unsigned long long> slMfgCapabilityChecks{0};
static std::atomic<unsigned int> slDynamicMfgCapabilityKnown{0};
static std::atomic<unsigned int> slDynamicMfgSupported{0};
static std::atomic<unsigned long long> slDynamicMfgCapabilityChecks{0};
// GPU-generation policy state. RTX 40-series cards are limited by product policy
// to standard 2x Frame Generation even if stale settings request a higher mode.
static std::atomic<unsigned int> slGpuClassKnown{0};
static std::atomic<unsigned int> slGpuIsRtx40Series{0};
// v0.8.20 retains v0.8.17 observability: prove that each DLSS/render-resolution transition
// starts a fresh FG segment and that generated frames return after the transition.
// These counters do not alter the working v0.8.16 frame-generation path.
static std::atomic<unsigned long long> slFgEnableTransitions{0};
static std::atomic<unsigned long long> slFgDisableTransitions{0};
static std::atomic<unsigned long long> slFgGeneratedSegments{0};
static std::atomic<unsigned long long> slFgResolutionTransitions{0};
static std::atomic<unsigned int> slFgGenerationConfirmedCurrentSegment{0};
static std::atomic<unsigned int> slFgLastEnabledMvecWidth{0};
static std::atomic<unsigned int> slFgLastEnabledMvecHeight{0};
static std::atomic<unsigned int> slFgMaxFramesPresentedObserved{0};
// Read-only overlay telemetry. Current FPS is derived from the application Present cadence
// multiplied by the latest observed DLSS-G presentation multiplier. Latency telemetry is
// intentionally not exposed by Control FG.
static std::atomic<unsigned int> slOverlayCurrentFpsMilli{0};
static std::atomic<unsigned long long> slSuppressedInfoCallbacks{0};

struct SLFrameSlot {
    std::atomic<unsigned long long> present{0};
    std::atomic<sl::FrameToken*> token{nullptr};
    std::atomic<unsigned int> constantsReady{0};
    std::atomic<unsigned int> tagMask{0};
};
static SLFrameSlot slFrameSlots[sl::MAX_FRAMES_IN_FLIGHT];
static const sl::ViewportHandle slFgViewport{0};
// Selection code 1 is reserved for native NVIDIA Dynamic MFG; fixed multipliers
// remain their literal values so existing 2x/3x/4x logic stays easy to audit.
static constexpr unsigned int kSLSelectionOff = 0;
static constexpr unsigned int kSLSelectionDynamic = 1;
static constexpr unsigned int kSLDefaultMultiplier = 4;
static constexpr unsigned int kSLMaxSelectableMultiplier = 6;
static std::atomic<unsigned int> slFgUserMultiplier{kSLDefaultMultiplier};
static std::atomic<unsigned int> slFgAppliedGeneratedFrames{0};
// 0=off, 1=fixed eOn, 2=native eDynamic.
static std::atomic<unsigned int> slFgAppliedMode{0};
static std::atomic<unsigned long long> slFgMultiplierChanges{0};

// Dynamic MFG target state.
// Manual target is whole FPS chosen from the overlay. A value of zero means Auto.
// Auto uses the monitor containing the Control window, measured in milli-Hz so
// 59.94/119.88/etc. can be preserved when QueryDisplayConfig reports them.
static constexpr unsigned int kSLDynamicTargetMinFps = 30;
static constexpr unsigned int kSLDynamicTargetMaxFps = 1000;
static constexpr unsigned int kSLDynamicTargetFallbackFps = 120;
static std::atomic<unsigned int> slFgDynamicManualTargetFps{0};
static std::atomic<unsigned int> slFgDynamicDetectedRefreshMilliHz{0};
static std::atomic<unsigned int> slFgAppliedDynamicTargetMilliFps{0};
static std::atomic<unsigned long long> slFgDynamicTargetChanges{0};
static std::atomic<unsigned long long> slFgDisplayRefreshChanges{0};

static unsigned int NormalizeFGMultiplier(unsigned int multiplier) noexcept {
    if (multiplier == kSLSelectionOff || multiplier == kSLSelectionDynamic ||
        (multiplier >= 2 && multiplier <= kSLMaxSelectableMultiplier)) return multiplier;
    return kSLDefaultMultiplier;
}

static bool IsFGDynamicSelection(unsigned int selection) noexcept {
    return selection == kSLSelectionDynamic;
}

static const char* GetFGSelectionName(unsigned int selection) noexcept {
    switch (NormalizeFGMultiplier(selection)) {
    case kSLSelectionOff: return "off";
    case kSLSelectionDynamic: return "dynamic";
    case 2: return "2x";
    case 3: return "3x";
    case 4: return "4x";
    case 5: return "5x";
    case 6: return "6x";
    default: return "4x";
    }
}

static const wchar_t* GetFGSelectionNameWide(unsigned int selection) noexcept {
    switch (NormalizeFGMultiplier(selection)) {
    case kSLSelectionOff: return L"Off";
    case kSLSelectionDynamic: return L"Dynamic";
    case 2: return L"2x";
    case 3: return L"3x";
    case 4: return L"4x";
    case 5: return L"5x";
    case 6: return L"6x";
    default: return L"4x";
    }
}

static unsigned int GetFGUserMultiplier() noexcept {
    return NormalizeFGMultiplier(slFgUserMultiplier.load(std::memory_order_acquire));
}

static unsigned int GetFGRequestedGeneratedFrames() noexcept {
    const unsigned int selection = GetFGUserMultiplier();
    return selection >= 2 ? selection - 1 : 0;
}

static unsigned int GetFGMaxSupportedMultiplier() noexcept {
    const unsigned int maxGenerated = slMfgMaxGenerated.load(std::memory_order_acquire);
    return maxGenerated ? maxGenerated + 1 : 0;
}

static bool IsFGSelectionAppliedForOverlay() noexcept {
    const unsigned int selected = GetFGUserMultiplier();
    const unsigned int appliedMode = slFgAppliedMode.load(std::memory_order_acquire);
    if (selected == kSLSelectionOff)
        return appliedMode == 0u && slFgEnabledByApi.load(std::memory_order_acquire) == 0u;
    if (selected == kSLSelectionDynamic)
        return appliedMode == 2u && slFgEnabledByApi.load(std::memory_order_acquire) != 0u;
    return appliedMode == 1u &&
           slFgAppliedGeneratedFrames.load(std::memory_order_acquire) + 1u == selected &&
           slFgEnabledByApi.load(std::memory_order_acquire) != 0u;
}

static unsigned int GetFGEffectiveMultiplierForOverlay() noexcept {
    const unsigned int selected = GetFGUserMultiplier();
    const unsigned int sampledSelection = slFgLastStateSelection.load(std::memory_order_acquire);
    if (sampledSelection != selected) return 0;
    if (!slFgEnabledByApi.load(std::memory_order_acquire)) return 0;
    return slFgLastFramesPresented.load(std::memory_order_acquire);
}

static unsigned long long GetFGLastStatePresentForOverlay() noexcept {
    return slFgLastStatePresent.load(std::memory_order_acquire);
}

static float GetFGCurrentFpsForOverlay() noexcept {
    return static_cast<float>(slOverlayCurrentFpsMilli.load(std::memory_order_acquire)) / 1000.0f;
}


static bool IsFGDynamicCapabilityKnown() noexcept {
    return slDynamicMfgCapabilityKnown.load(std::memory_order_acquire) != 0;
}

static bool IsFGDynamicMFGSupported() noexcept {
    return slDynamicMfgSupported.load(std::memory_order_acquire) != 0;
}

static unsigned int ClampFGDynamicManualTargetFps(unsigned int fps) noexcept {
    if (!fps) return 0;
    if (fps < kSLDynamicTargetMinFps) return kSLDynamicTargetMinFps;
    if (fps > kSLDynamicTargetMaxFps) return kSLDynamicTargetMaxFps;
    return fps;
}

static unsigned int GetFGDynamicManualTargetFps() noexcept {
    return slFgDynamicManualTargetFps.load(std::memory_order_acquire);
}

static unsigned int GetFGDynamicDetectedRefreshMilliHz() noexcept {
    return slFgDynamicDetectedRefreshMilliHz.load(std::memory_order_acquire);
}

static unsigned int GetFGDynamicResolvedTargetMilliFps() noexcept {
    const unsigned int manual = GetFGDynamicManualTargetFps();
    if (manual) return manual * 1000u;
    return GetFGDynamicDetectedRefreshMilliHz();
}

static float GetFGDynamicResolvedTargetFrameRate() noexcept {
    return static_cast<float>(GetFGDynamicResolvedTargetMilliFps()) / 1000.0f;
}

static const char* GetFGDynamicTargetPolicyName() noexcept {
    if (GetFGDynamicManualTargetFps()) return "manual";
    return GetFGDynamicDetectedRefreshMilliHz() ? "auto_detected" : "sdk_auto_fallback";
}

static unsigned int GetFGDynamicResolvedTargetRoundedFps() noexcept {
    const unsigned int milli = GetFGDynamicResolvedTargetMilliFps();
    return milli ? (milli + 500u) / 1000u : 0u;
}

static void SetFGDynamicDetectedRefreshMilliHz(unsigned int milliHz, const char* source) noexcept {
    if (milliHz && (milliHz < kSLDynamicTargetMinFps * 1000u || milliHz > kSLDynamicTargetMaxFps * 1000u)) milliHz = 0;
    const unsigned int previous = slFgDynamicDetectedRefreshMilliHz.exchange(milliHz, std::memory_order_acq_rel);
    if (previous != milliHz) {
        const unsigned long long ordinal = ++slFgDisplayRefreshChanges;
        Log("FG_DISPLAY_REFRESH previous_millihz=%u detected_millihz=%u detected_fps=%.3f source=%s changes=%llu dynamic_policy=%s apply=next_present_if_dynamic",
            previous, milliHz, static_cast<double>(milliHz) / 1000.0, source ? source : "unknown", ordinal, GetFGDynamicTargetPolicyName());
    }
}

static void SetFGDynamicManualTargetFps(unsigned int fps) noexcept {
    fps = ClampFGDynamicManualTargetFps(fps);
    const unsigned int previous = slFgDynamicManualTargetFps.exchange(fps, std::memory_order_acq_rel);
    if (previous != fps) {
        const unsigned long long ordinal = ++slFgDynamicTargetChanges;
        Log("FG_DYNAMIC_TARGET_SELECTION previous_policy=%s new_policy=%s previous_manual_fps=%u new_manual_fps=%u detected_fps=%.3f resolved_target_fps=%.3f changes=%llu apply=next_present",
            previous ? "manual" : "auto", fps ? "manual" : "auto", previous, fps,
            static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
            static_cast<double>(GetFGDynamicResolvedTargetFrameRate()), ordinal);
    }
}

static void AdjustFGDynamicManualTargetFps(int delta) noexcept {
    unsigned int base = GetFGDynamicManualTargetFps();
    if (!base) {
        base = GetFGDynamicResolvedTargetRoundedFps();
        if (!base) base = kSLDynamicTargetFallbackFps;
    }
    long long next = static_cast<long long>(base) + static_cast<long long>(delta);
    if (next < static_cast<long long>(kSLDynamicTargetMinFps)) next = kSLDynamicTargetMinFps;
    if (next > static_cast<long long>(kSLDynamicTargetMaxFps)) next = kSLDynamicTargetMaxFps;
    SetFGDynamicManualTargetFps(static_cast<unsigned int>(next));
}

static void SetFGUserMultiplier(unsigned int multiplier) noexcept {
    multiplier = NormalizeFGMultiplier(multiplier);
    const unsigned int previous = slFgUserMultiplier.exchange(multiplier, std::memory_order_acq_rel);
    if (previous != multiplier) {
        ++slFgMultiplierChanges;
        Log("FG_USER_SELECTION previous_selection=%s selected_selection=%s previous_code=%u selected_code=%u requested_generated=%u mode=%s changes=%llu apply=next_present",
            GetFGSelectionName(previous), GetFGSelectionName(multiplier), previous, multiplier,
            multiplier >= 2 ? multiplier - 1 : 0, IsFGDynamicSelection(multiplier) ? "dynamic" : (multiplier ? "fixed" : "off"),
            slFgMultiplierChanges.load());
    }
}

static bool IsFGRtx40Series() noexcept {
    return slGpuClassKnown.load(std::memory_order_acquire) != 0 &&
           slGpuIsRtx40Series.load(std::memory_order_acquire) != 0;
}

static bool IsFGSelectionAllowedByGpuPolicy(unsigned int selection) noexcept {
    selection = NormalizeFGMultiplier(selection);
    if (!IsFGRtx40Series()) return true;
    return selection == kSLSelectionOff || selection == 2u;
}

static void CacheFGGpuClassFromLuid(const LUID& luid) noexcept {
    if (slGpuClassKnown.load(std::memory_order_acquire)) return;

    using CreateFactory1Fn = HRESULT (WINAPI*)(REFIID, void**);
    auto createFactory1 = realDxgi ? reinterpret_cast<CreateFactory1Fn>(GetProcAddress(realDxgi, "CreateDXGIFactory1")) : nullptr;
    if (!createFactory1) {
        Log("FG_GPU_CLASS_DETECT success=0 reason=create_factory_unavailable adapter_luid=%08lX:%08lX",
            static_cast<unsigned long>(luid.HighPart), luid.LowPart);
        return;
    }

    IDXGIFactory1* factory = nullptr;
    const HRESULT factoryHr = createFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(factoryHr) || !factory) {
        Log("FG_GPU_CLASS_DETECT success=0 reason=create_factory_failed hr=0x%08lX adapter_luid=%08lX:%08lX",
            static_cast<unsigned long>(factoryHr), static_cast<unsigned long>(luid.HighPart), luid.LowPart);
        return;
    }

    bool found = false;
    bool rtx40 = false;
    wchar_t description[128]{};
    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT enumHr = factory->EnumAdapters1(index, &adapter);
        if (enumHr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(enumHr) || !adapter) continue;
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) &&
            desc.AdapterLuid.HighPart == luid.HighPart && desc.AdapterLuid.LowPart == luid.LowPart) {
            found = true;
            wcsncpy_s(description, _countof(description), desc.Description, _TRUNCATE);
            // Scope this policy to GeForce RTX 40-series desktop/laptop products.
            // This intentionally does not classify RTX 4000 Ada workstation SKUs as GeForce 40-series.
            rtx40 = desc.VendorId == 0x10DE && wcsstr(desc.Description, L"GeForce RTX 40") != nullptr;
            adapter->Release();
            break;
        }
        adapter->Release();
    }
    factory->Release();

    if (!found) {
        Log("FG_GPU_CLASS_DETECT success=0 reason=luid_not_found adapter_luid=%08lX:%08lX",
            static_cast<unsigned long>(luid.HighPart), luid.LowPart);
        return;
    }

    slGpuIsRtx40Series.store(rtx40 ? 1u : 0u, std::memory_order_release);
    slGpuClassKnown.store(1u, std::memory_order_release);
    Log("FG_GPU_CLASS_DETECT success=1 adapter=%ls rtx40_series=%u policy=%s",
        description, unsigned(rtx40), rtx40 ? "off_plus_2x_only" : "streamline_capability_driven");
}

static constexpr unsigned int kSLTagDepth = 1u << 0;
static constexpr unsigned int kSLTagMotionVectors = 1u << 1;
static constexpr unsigned int kSLTagHUDLess = 1u << 2;
static constexpr unsigned int kSLRequiredTagMask = kSLTagDepth | kSLTagMotionVectors;

static long long SLResultCode(sl::Result result) noexcept {
    return static_cast<long long>(static_cast<int32_t>(result));
}

static void SLLogCallback(sl::LogType type, const char* message) {
    // Streamline already writes its complete sl.log into the per-run directory that
    // Collect-Logs.cmd packages. Avoid duplicating every informational line into
    // the probe log now that the bring-up phase is complete. Keep warnings/errors
    // plus the interpolation state transition, which is our compact FG proof.
    const unsigned int logType = unsigned(type);
    const bool importantInfo = message && strstr(message, "DLSS-G interpolation state changed");
    if (logType != 0 || importantInfo) {
        Log("SL_LOG type=%u message=%s", logType, message ? message : "(null)");
    } else {
        ++slSuppressedInfoCallbacks;
    }
}

static bool TouchSLCurrentBackBufferIndex(unsigned long long present) noexcept {
    const DWORD saved = GetLastError();
    ++slBackBufferIndexAttempts;

    IDXGISwapChain* chain = nullptr;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto engineDevice = base ? *reinterpret_cast<unsigned char**>(base + kNativeDevicePointerRva) : nullptr;
        chain = engineDevice ? *reinterpret_cast<IDXGISwapChain**>(engineDevice + kSwapChainOffset) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++slBackBufferIndexFailures;
        if (present <= 8 || (present % 240) == 0)
            Log("SL_BACKBUFFER_INDEX present=%llu success=0 stage=chain_read exception=0x%08lX attempts=%llu successes=%llu failures=%llu",
                present, GetExceptionCode(), slBackBufferIndexAttempts.load(), slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load());
        SetLastError(saved);
        return false;
    }

    if (!chain) {
        ++slBackBufferIndexFailures;
        if (present <= 8 || (present % 240) == 0)
            Log("SL_BACKBUFFER_INDEX present=%llu success=0 stage=null_chain attempts=%llu successes=%llu failures=%llu",
                present, slBackBufferIndexAttempts.load(), slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load());
        SetLastError(saved);
        return false;
    }

    IDXGISwapChain3* chain3 = nullptr;
    const HRESULT qi = chain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&chain3));
    if (FAILED(qi) || !chain3) {
        ++slBackBufferIndexFailures;
        if (present <= 8 || (present % 240) == 0)
            Log("SL_BACKBUFFER_INDEX present=%llu success=0 stage=query_interface chain=%p hr=0x%08lX attempts=%llu successes=%llu failures=%llu",
                present, chain, static_cast<unsigned long>(qi), slBackBufferIndexAttempts.load(), slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load());
        SetLastError(saved);
        return false;
    }

    const UINT index = chain3->GetCurrentBackBufferIndex();
    chain3->Release();
    slLastBackBufferIndex.store(index, std::memory_order_release);
    ++slBackBufferIndexSuccesses;
    if (present <= 8 || (present % 240) == 0 || slBackBufferIndexSuccesses.load() <= 8) {
        Log("SL_BACKBUFFER_INDEX present=%llu success=1 chain=%p index=%u timing=before_present attempts=%llu successes=%llu failures=%llu",
            present, chain, index, slBackBufferIndexAttempts.load(), slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load());
    }
    SetLastError(saved);
    return true;
}

static std::wstring ParentDirectory(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring{} : path.substr(0, slash);
}

static bool FileExists(const std::wstring& path) noexcept {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static bool VerifyAuthenticode(const std::wstring& path, LONG* statusOut) noexcept {
    WINTRUST_FILE_INFO file{};
    file.cbStruct = sizeof(file);
    file.pcwszFilePath = path.c_str();

    WINTRUST_DATA data{};
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &file;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    data.dwProvFlags = 0;

    GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG status = WinVerifyTrust(nullptr, &policy, &data);
    data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &policy, &data);
    if (statusOut) *statusOut = status;
    return status == ERROR_SUCCESS;
}

static void InitializeStreamlineCoreAfterFactory(const char* trigger) noexcept {
    unsigned int expected = 0;
    if (!slBootstrapState.compare_exchange_strong(expected, 1)) {
        // Critical: never wait if Streamline re-enters our DXGI exports during slInit.
        if (expected == 1) Log("SL_BOOTSTRAP_REENTRY_SKIPPED trigger=%s state=in_progress", trigger ? trigger : "unknown");
        return;
    }

    Log("SL_BOOTSTRAP_BEGIN trigger=%s stage=post_native_factory features_requested=3 features=reflex,pcl,dlssg", trigger ? trigger : "unknown");
    try {
        const std::wstring modulePath = ModulePath(selfModule);
        const std::wstring gameDirectory = ParentDirectory(modulePath);
        if (gameDirectory.empty()) {
            Log("SL_DISABLED stage=path reason=self_module_directory_unavailable");
            slBootstrapState.store(3);
            return;
        }
        slGameDirectory = gameDirectory;
        slRuntimeDirectory = gameDirectory + L"\\ControlFGStreamline";
        slLogDirectory = gameDirectory;
        wchar_t localAppData[32768]{};
        const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, _countof(localAppData));
        if (n && n < _countof(localAppData)) {
            std::wstring base(localAppData, n);
            std::wstring root = base + L"\\ControlFGProbe";
            CreateDirectoryW(root.c_str(), nullptr);
            wchar_t slRunName[96]{};
            swprintf_s(slRunName, L"\\Streamline-v1.0.0-%lu", GetCurrentProcessId());
            slLogDirectory = root + slRunName;
            CreateDirectoryW(slLogDirectory.c_str(), nullptr);
        }

        // Feature-load isolation requires only the signed production runtime pieces
        // for common/PCL/Reflex/DLSS-G. DXGI factory/presentation is upgraded; the host device remains native, while Control's exact command-queue constructor temporarily routes through a private Streamline device proxy.
        const wchar_t* required[] = { L"sl.interposer.dll", L"sl.common.dll", L"sl.pcl.dll",
                                      L"sl.reflex.dll", L"sl.dlss_g.dll", L"nvngx_dlssg.dll" };
        for (const wchar_t* name : required) {
            const std::wstring path = slRuntimeDirectory + L"\\" + name;
            if (!FileExists(path)) {
                Log("SL_DISABLED stage=runtime_files reason=missing file=%ls", path.c_str());
                slBootstrapState.store(3);
                return;
            }
        }

        const std::wstring interposerPath = slRuntimeDirectory + L"\\sl.interposer.dll";
        LONG trust = TRUST_E_NOSIGNATURE;
        const bool trusted = VerifyAuthenticode(interposerPath, &trust);
        Log("SL_SIGNATURE file=sl.interposer.dll trusted=%u status=0x%08lX", unsigned(trusted), static_cast<unsigned long>(trust));
        if (!trusted) {
            Log("SL_DISABLED stage=signature reason=interposer_authenticode_failed");
            slBootstrapState.store(3);
            return;
        }

        Log("SL_LOAD_BEGIN path=%ls", interposerPath.c_str());
        slInterposerModule = LoadLibraryExW(interposerPath.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        Log("SL_LOAD_END module=%p error=%lu", slInterposerModule, slInterposerModule ? 0ul : GetLastError());
        if (!slInterposerModule) {
            Log("SL_DISABLED stage=load reason=LoadLibraryEx_failed error=%lu", GetLastError());
            slBootstrapState.store(3);
            return;
        }

        slInitApi = reinterpret_cast<PFun_slInit*>(GetProcAddress(slInterposerModule, "slInit"));
        slSetD3DDeviceApi = reinterpret_cast<PFun_slSetD3DDevice*>(GetProcAddress(slInterposerModule, "slSetD3DDevice"));
        slIsFeatureSupportedApi = reinterpret_cast<PFun_slIsFeatureSupported*>(GetProcAddress(slInterposerModule, "slIsFeatureSupported"));
        slIsFeatureLoadedApi = reinterpret_cast<PFun_slIsFeatureLoaded*>(GetProcAddress(slInterposerModule, "slIsFeatureLoaded"));
        slGetNativeInterfaceApi = reinterpret_cast<PFun_slGetNativeInterface*>(GetProcAddress(slInterposerModule, "slGetNativeInterface"));
        slUpgradeInterfaceApi = reinterpret_cast<SLUpgradeInterfaceFn*>(GetProcAddress(slInterposerModule, "slUpgradeInterface"));
        slGetFeatureFunctionApi = reinterpret_cast<PFun_slGetFeatureFunction*>(GetProcAddress(slInterposerModule, "slGetFeatureFunction"));
        slSetConstantsApi = reinterpret_cast<PFun_slSetConstants*>(GetProcAddress(slInterposerModule, "slSetConstants"));
        slSetTagForFrameApi = reinterpret_cast<PFun_slSetTagForFrame*>(GetProcAddress(slInterposerModule, "slSetTagForFrame"));
        slGetNewFrameTokenApi = reinterpret_cast<PFun_slGetNewFrameToken*>(GetProcAddress(slInterposerModule, "slGetNewFrameToken"));
        if (!slInitApi || !slSetD3DDeviceApi || !slIsFeatureSupportedApi || !slIsFeatureLoadedApi ||
            !slGetNativeInterfaceApi || !slUpgradeInterfaceApi || !slGetFeatureFunctionApi ||
            !slSetConstantsApi || !slSetTagForFrameApi || !slGetNewFrameTokenApi) {
            Log("SL_DISABLED stage=exports reason=required_core_export_missing slInit=%p slSetD3DDevice=%p slIsFeatureSupported=%p slIsFeatureLoaded=%p slGetNativeInterface=%p slUpgradeInterface=%p slGetFeatureFunction=%p slSetConstants=%p slSetTagForFrame=%p slGetNewFrameToken=%p",
                slInitApi, slSetD3DDeviceApi, slIsFeatureSupportedApi, slIsFeatureLoadedApi, slGetNativeInterfaceApi, slUpgradeInterfaceApi, slGetFeatureFunctionApi, slSetConstantsApi, slSetTagForFrameApi, slGetNewFrameTokenApi);
            slBootstrapState.store(3);
            return;
        }

        slPluginPaths[0] = slRuntimeDirectory.c_str();
        slPluginPaths[1] = slGameDirectory.c_str();
        Log("SL_NGX_SEARCH_PATHS count=2 path0=%ls path1=%ls purpose=make_dlssg_and_control_dlss_visible_before_device_bind",
            slPluginPaths[0], slPluginPaths[1]);
        sl::Preferences preferences{};
        preferences.logLevel = sl::LogLevel::eDefault;
        preferences.pathsToPlugins = slPluginPaths;
        preferences.numPathsToPlugins = 2;
        preferences.pathToLogsAndData = slLogDirectory.c_str();
        preferences.logMessageCallback = &SLLogCallback;
        preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking |
                            sl::PreferenceFlags::eDisableDebugText |
                            sl::PreferenceFlags::eUseManualHooking |
                            sl::PreferenceFlags::eUseFrameBasedResourceTagging;
        preferences.featuresToLoad = slFeaturesToLoad;
        preferences.numFeaturesToLoad = _countof(slFeaturesToLoad);
        preferences.applicationId = 0;
        preferences.projectId = kControlProjectId;
        preferences.engine = sl::EngineType::eCustom;
        preferences.engineVersion = kControlEngineVersion;
        preferences.renderAPI = sl::RenderAPI::eD3D12;

        Log("SL_IDENTITY project_id=%s engine=custom engine_version=%s source=control_ngx_init_project_id", kControlProjectId, kControlEngineVersion);
        Log("SL_INIT_BEGIN sdk=2.14.1 features_requested=3 features=reflex,pcl,dlssg trigger=%s", trigger ? trigger : "unknown");
        const sl::Result result = slInitApi(preferences, sl::kSDKVersion);
        Log("SL_INIT_END sdk=2.14.1 result=%lld features_requested=3 runtime_dir=%ls log_dir=%ls",
            SLResultCode(result), slRuntimeDirectory.c_str(), slLogDirectory.c_str());
        if (result != sl::Result::eOk) {
            Log("SL_DISABLED stage=init reason=slInit_failed result=%lld", SLResultCode(result));
            slBootstrapState.store(3);
            return;
        }
        slBootstrapState.store(2);
        Log("SL_CORE_READY core_only=0 native_host_interfaces=1 features_requested=3 features=reflex,pcl,dlssg device_set=%u factory_upgrade=armed swapchain_upgrade=via_factory queue_hook=ready fg_activation_gate=armed", slDeviceConfigured.load());
    } catch (...) {
        Log("SL_DISABLED stage=bootstrap reason=exception");
        slBootstrapState.store(3);
    }
}

static bool UpgradeFactoryForPresentation(void** factory, const char* entrypoint) noexcept {
    if (!factory || !*factory || slBootstrapState.load() != 2 || !slUpgradeInterfaceApi) {
        Log("SL_FACTORY_UPGRADE_SKIP entrypoint=%s factory=%p bootstrap_state=%u api=%p",
            entrypoint ? entrypoint : "unknown", factory ? *factory : nullptr, slBootstrapState.load(), slUpgradeInterfaceApi);
        return false;
    }
    ++slFactoryUpgradeAttempts;
    void* nativeFactory = *factory;
    void* upgradedFactory = nativeFactory;
    const sl::Result result = slUpgradeInterfaceApi(&upgradedFactory);
    const bool upgraded = result == sl::Result::eOk && upgradedFactory && upgradedFactory != nativeFactory;
    if (result == sl::Result::eOk && upgradedFactory) {
        *factory = upgradedFactory;
        if (upgraded) ++slFactoryUpgradeSuccesses;
    }
    Log("SL_FACTORY_UPGRADE entrypoint=%s result=%lld native=%p returned=%p proxy=%u attempts=%u successes=%u purpose=streamline_swapchain_present_common",
        entrypoint ? entrypoint : "unknown", SLResultCode(result), nativeFactory,
        (result == sl::Result::eOk && upgradedFactory) ? upgradedFactory : nativeFactory,
        unsigned(upgraded), slFactoryUpgradeAttempts.load(), slFactoryUpgradeSuccesses.load());
    return result == sl::Result::eOk && upgradedFactory;
}

static void ProbeSLFeatureGate(const LUID& luid) noexcept {
    if (!slIsFeatureSupportedApi || !slIsFeatureLoadedApi) return;
    sl::AdapterInfo adapter{};
    adapter.deviceLUID = reinterpret_cast<uint8_t*>(const_cast<LUID*>(&luid));
    adapter.deviceLUIDSizeInBytes = sizeof(LUID);
    bool reflexLoaded = false, pclLoaded = false, dlssgLoaded = false;
    const sl::Result reflexLoadedResult = slIsFeatureLoadedApi(sl::kFeatureReflex, reflexLoaded);
    const sl::Result pclLoadedResult = slIsFeatureLoadedApi(sl::kFeaturePCL, pclLoaded);
    const sl::Result dlssgLoadedResult = slIsFeatureLoadedApi(sl::kFeatureDLSS_G, dlssgLoaded);
    const sl::Result reflexSupportResult = slIsFeatureSupportedApi(sl::kFeatureReflex, adapter);
    const sl::Result pclSupportResult = slIsFeatureSupportedApi(sl::kFeaturePCL, adapter);
    Log("SL_FEATURE_GATE project_id=%s reflex_loaded_result=%lld reflex_loaded=%u reflex_support_result=%lld pcl_loaded_result=%lld pcl_loaded=%u pcl_support_result=%lld dlssg_loaded_result=%lld dlssg_loaded=%u dlssg_expected_loaded=1 fg_api_enabled=%u",
        kControlProjectId,
        SLResultCode(reflexLoadedResult), unsigned(reflexLoaded), SLResultCode(reflexSupportResult),
        SLResultCode(pclLoadedResult), unsigned(pclLoaded), SLResultCode(pclSupportResult),
        SLResultCode(dlssgLoadedResult), unsigned(dlssgLoaded), slFgEnabledByApi.load());
}

static bool IsHdr10BridgeActive() noexcept;
static unsigned int GetHdr10BridgeWidth() noexcept;
static unsigned int GetHdr10BridgeHeight() noexcept;
static void SetDLSSGModeForPresent(unsigned long long present, bool enable) noexcept;

static void ResolveSLFrameFeatureFunctions() noexcept {
    if (!slGetFeatureFunctionApi || !slDeviceConfigured.load()) return;
    if (!slPCLSetMarkerApi) {
        void* fn = nullptr;
        const sl::Result result = slGetFeatureFunctionApi(sl::kFeaturePCL, "slPCLSetMarker", fn);
        slPCLSetMarkerApi = result == sl::Result::eOk ? reinterpret_cast<PFun_slPCLSetMarker*>(fn) : nullptr;
        Log("SL_FRAME_FUNCTION feature=pcl name=slPCLSetMarker result=%lld function=%p", SLResultCode(result), fn);
    }
    if (!slReflexSetOptionsApi) {
        void* fn = nullptr;
        const sl::Result result = slGetFeatureFunctionApi(sl::kFeatureReflex, "slReflexSetOptions", fn);
        slReflexSetOptionsApi = result == sl::Result::eOk ? reinterpret_cast<PFun_slReflexSetOptions*>(fn) : nullptr;
        Log("SL_FRAME_FUNCTION feature=reflex name=slReflexSetOptions result=%lld function=%p", SLResultCode(result), fn);
    }
    if (!slReflexSleepApi) {
        void* fn = nullptr;
        const sl::Result result = slGetFeatureFunctionApi(sl::kFeatureReflex, "slReflexSleep", fn);
        slReflexSleepApi = result == sl::Result::eOk ? reinterpret_cast<PFun_slReflexSleep*>(fn) : nullptr;
        Log("SL_FRAME_FUNCTION feature=reflex name=slReflexSleep result=%lld function=%p", SLResultCode(result), fn);
    }
}

static unsigned int ResolveSLReflexFrameLimitUs(bool& dynamicSelected, float& targetFps) noexcept {
    dynamicSelected = IsFGDynamicSelection(GetFGUserMultiplier());
    targetFps = dynamicSelected ? GetFGDynamicResolvedTargetFrameRate() : 0.0f;

    // If Streamline has positively reported Dynamic unsupported, do not leave a
    // limiter active merely because the UI selection still says Dynamic. Before
    // the capability query settles, applying the limiter is safe and avoids a
    // one-frame dependency on the state query.
    if (!dynamicSelected || (IsFGDynamicCapabilityKnown() && !IsFGDynamicMFGSupported()) || !(targetFps >= 1.0f))
        return 0;

    // NVIDIA exposes the Reflex limiter as a frame period in microseconds. Keep
    // the user's Dynamic target expressed as output FPS and use the same numeric
    // target for the Reflex cap. This is intentionally a minimal integration: it
    // enables the required Reflex-aware pacing without inventing a multiplier or
    // forcing a lower rendered-FPS target.
    double frameLimitUs = 1000000.0 / static_cast<double>(targetFps);
    if (frameLimitUs < 1.0) frameLimitUs = 1.0;
    if (frameLimitUs > 4294967294.0) frameLimitUs = 4294967294.0;
    return static_cast<unsigned int>(frameLimitUs + 0.5);
}

static void ConfigureSLReflexForCurrentFG(unsigned long long present) noexcept {
    if (!slDeviceConfigured.load()) return;
    ResolveSLFrameFeatureFunctions();
    if (!slReflexSetOptionsApi) return;

    bool dynamicSelected = false;
    float targetFps = 0.0f;
    unsigned int desiredFrameLimitUs = ResolveSLReflexFrameLimitUs(dynamicSelected, targetFps);
    if (slReflexOptionsReady.load(std::memory_order_acquire) &&
        slReflexAppliedFrameLimitUs.load(std::memory_order_acquire) == desiredFrameLimitUs) return;

    AcquireSRWLockExclusive(&slReflexOptionsLock);
    dynamicSelected = false;
    targetFps = 0.0f;
    desiredFrameLimitUs = ResolveSLReflexFrameLimitUs(dynamicSelected, targetFps);
    const unsigned int previousFrameLimitUs = slReflexAppliedFrameLimitUs.load(std::memory_order_acquire);
    if (slReflexOptionsReady.load(std::memory_order_acquire) && previousFrameLimitUs == desiredFrameLimitUs) {
        ReleaseSRWLockExclusive(&slReflexOptionsLock);
        return;
    }

    sl::ReflexOptions options{};
    options.mode = sl::ReflexMode::eLowLatency;
    options.frameLimitUs = desiredFrameLimitUs;
    const sl::Result result = slReflexSetOptionsApi(options);
    const unsigned long long optionsCall = ++slReflexOptionsCalls;
    if (result == sl::Result::eOk) {
        slReflexAppliedFrameLimitUs.store(desiredFrameLimitUs, std::memory_order_release);
        slReflexOptionsReady.store(1, std::memory_order_release);
    }
    const double limiterFps = desiredFrameLimitUs ? 1000000.0 / static_cast<double>(desiredFrameLimitUs) : 0.0;
    Log("SL_REFLEX_OPTIONS present=%llu mode=low_latency frame_limit_us=%u limiter_fps=%.3f dynamic_selected=%u dynamic_capability_known=%u dynamic_supported=%u target_policy=%s target_fps=%.3f previous_frame_limit_us=%u result=%lld ready=%u options_calls=%llu purpose=%s",
        present, desiredFrameLimitUs, limiterFps, unsigned(dynamicSelected), unsigned(IsFGDynamicCapabilityKnown()),
        unsigned(IsFGDynamicMFGSupported()), dynamicSelected ? GetFGDynamicTargetPolicyName() : "n/a",
        static_cast<double>(targetFps), previousFrameLimitUs, SLResultCode(result), slReflexOptionsReady.load(),
        optionsCall, dynamicSelected && desiredFrameLimitUs ? "dynamic_mfg_reflex_limiter" : "dlssg_required_reflex_no_limit");

    if (result == sl::Result::eOk && dynamicSelected && desiredFrameLimitUs) {
        const unsigned long long ordinal = ++slDynamicReflexLimiterChanges;
        Log("FG_DYNAMIC_REFLEX_LIMITER present=%llu target_policy=%s target_fps=%.3f frame_limit_us=%u limiter_fps=%.3f previous_frame_limit_us=%u changes=%llu apply=before_reflex_sleep",
            present, GetFGDynamicTargetPolicyName(), static_cast<double>(targetFps), desiredFrameLimitUs, limiterFps,
            previousFrameLimitUs, ordinal);
    } else if (result == sl::Result::eOk && previousFrameLimitUs != 0xFFFFFFFFu && previousFrameLimitUs != 0 && desiredFrameLimitUs == 0) {
        Log("FG_DYNAMIC_REFLEX_LIMITER_RELEASE present=%llu previous_frame_limit_us=%u frame_limit_us=0 selection=%s apply=before_reflex_sleep",
            present, previousFrameLimitUs, GetFGSelectionName(GetFGUserMultiplier()));
    }
    ReleaseSRWLockExclusive(&slReflexOptionsLock);
}

static SLFrameSlot* SLFrameSlotFor(unsigned long long present) noexcept {
    return &slFrameSlots[present % sl::MAX_FRAMES_IN_FLIGHT];
}

static sl::FrameToken* GetSLFrameToken(unsigned long long present, bool requireConstants) noexcept {
    SLFrameSlot* slot = SLFrameSlotFor(present);
    if (slot->present.load(std::memory_order_acquire) != present) return nullptr;
    if (requireConstants && !slot->constantsReady.load(std::memory_order_acquire)) return nullptr;
    return slot->token.load(std::memory_order_acquire);
}

static void MarkSLFrameConstantsReady(unsigned long long present, bool ready) noexcept {
    SLFrameSlot* slot = SLFrameSlotFor(present);
    if (slot->present.load(std::memory_order_acquire) == present)
        slot->constantsReady.store(ready ? 1u : 0u, std::memory_order_release);
}

static unsigned int GetSLFrameTagMask(unsigned long long present) noexcept {
    SLFrameSlot* slot = SLFrameSlotFor(present);
    if (slot->present.load(std::memory_order_acquire) != present) return 0;
    return slot->tagMask.load(std::memory_order_acquire);
}

static void MarkSLFrameTagBits(unsigned long long present, unsigned int bits) noexcept {
    SLFrameSlot* slot = SLFrameSlotFor(present);
    if (slot->present.load(std::memory_order_acquire) == present)
        slot->tagMask.fetch_or(bits, std::memory_order_acq_rel);
}

static void PrepareSLFrameToken(unsigned long long present) noexcept {
    if (!present || !slDeviceConfigured.load() || !slGetNewFrameTokenApi) return;
    if (GetSLFrameToken(present, false)) return;
    ConfigureSLReflexForCurrentFG(present);
    ResolveSLFrameFeatureFunctions();
    ++slFrameTokenAttempts;
    const uint32_t frameIndex = static_cast<uint32_t>(present);
    sl::FrameToken* token = nullptr;
    const sl::Result result = slGetNewFrameTokenApi(token, &frameIndex);
    if (result != sl::Result::eOk || !token) {
        if (present <= 8 || (present % 240) == 0)
            Log("SL_FRAME_TOKEN present=%llu frame_index=%u result=%lld token=%p success=0", present, frameIndex, SLResultCode(result), token);
        return;
    }
    SLFrameSlot* slot = SLFrameSlotFor(present);
    slot->constantsReady.store(0, std::memory_order_relaxed);
    slot->tagMask.store(0, std::memory_order_relaxed);
    slot->token.store(token, std::memory_order_relaxed);
    slot->present.store(present, std::memory_order_release);
    ++slFrameTokenSuccesses;

    sl::Result sleepResult = sl::Result::eErrorNotInitialized;
    if (slReflexSleepApi) {
        sleepResult = slReflexSleepApi(*token);
        ++slReflexSleepCalls;
    }
    if (present <= 8 || (present % 240) == 0) {
        Log("SL_FRAME_TOKEN present=%llu frame_index=%u result=%lld token=%p success=1 reflex_sleep_result=%lld token_successes=%llu sleep_calls=%llu",
            present, frameIndex, SLResultCode(result), token, SLResultCode(sleepResult), slFrameTokenSuccesses.load(), slReflexSleepCalls.load());
    }
}

static void MarkSLSimulationStart(unsigned long long present) noexcept {
    if (!present) return;
    ResolveSLFrameFeatureFunctions();
    sl::FrameToken* token = GetSLFrameToken(present, false);
    if (!token || !slPCLSetMarkerApi) {
        if (present <= 8 || (present % 240) == 0)
            Log("SL_SIMULATION_MARKER_START_SKIP present=%llu token=%p marker_api=%p reason=frame_token_or_marker_missing",
                present, token, slPCLSetMarkerApi);
        return;
    }
    ++slSimulationMarkerAttempts;
    const sl::Result result = slPCLSetMarkerApi(sl::PCLMarker::eSimulationStart, *token);
    if (result == sl::Result::eOk) ++slSimulationMarkerStarts;
    else ++slSimulationMarkerFailures;
    if (present <= 8 || (present % 240) == 0 || slSimulationMarkerStarts.load() <= 8 || result != sl::Result::eOk)
        Log("SL_SIMULATION_MARKER_START present=%llu token=%p result=%lld attempts=%llu starts=%llu failures=%llu placement=after_reflex_sleep_before_control_beginframe",
            present, token, SLResultCode(result), slSimulationMarkerAttempts.load(), slSimulationMarkerStarts.load(),
            slSimulationMarkerFailures.load());
}

static sl::FrameToken* BeginSLPresentFrame(unsigned long long present) noexcept {
    ResolveSLFrameFeatureFunctions();
    sl::FrameToken* token = GetSLFrameToken(present, false);
    const bool constantsReady = GetSLFrameToken(present, true) != nullptr;
    const unsigned int tagMask = GetSLFrameTagMask(present);
    const bool tagsReady = (tagMask & kSLRequiredTagMask) == kSLRequiredTagMask;
    const bool frameReady = token && constantsReady && tagsReady;
    if (frameReady) ++slFgFrameReadyPresents;
    SetDLSSGModeForPresent(present, frameReady);
    if (frameReady && slFgEnabledByApi.load(std::memory_order_acquire)) ++slFgEnabledPresents;

    // Do not issue Present markers for frames where the FG inputs are incomplete.
    // This prevents stale constants/tags from being paired with menu/loading frames.
    if (!frameReady || !slPCLSetMarkerApi) {
        if (present <= 8 || (present % 240) == 0 || constantsReady || tagMask)
            Log("SL_PRESENT_FRAME_GATE present=%llu token=%p constants_ready=%u tag_mask=0x%X required_mask=0x%X frame_ready=%u marker_api=%p",
                present, token, unsigned(constantsReady), tagMask, kSLRequiredTagMask, unsigned(frameReady), slPCLSetMarkerApi);
        return nullptr;
    }
    const sl::Result result = slPCLSetMarkerApi(sl::PCLMarker::ePresentStart, *token);
    if (result == sl::Result::eOk) ++slPresentMarkerStarts;
    if (present <= 8 || (present % 240) == 0 || slPresentMarkerStarts.load() <= 8)
        Log("SL_PRESENT_MARKER_START present=%llu token=%p constants_ready=1 tag_mask=0x%X result=%lld starts=%llu",
            present, token, tagMask, SLResultCode(result), slPresentMarkerStarts.load());
    return result == sl::Result::eOk ? token : nullptr;
}

static void UpdateFGOverlayCurrentFps() noexcept {
    LARGE_INTEGER now{};
    LARGE_INTEGER freq{};
    if (!QueryPerformanceCounter(&now) || !QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0) return;
    static LONGLONG previousQpc = 0;
    static double smoothedNativeFps = 0.0;
    if (previousQpc > 0 && now.QuadPart > previousQpc) {
        const double dt = static_cast<double>(now.QuadPart - previousQpc) / static_cast<double>(freq.QuadPart);
        if (dt > 0.0001 && dt < 1.0) {
            const double nativeFps = 1.0 / dt;
            smoothedNativeFps = smoothedNativeFps > 0.0 ? (smoothedNativeFps * 0.88 + nativeFps * 0.12) : nativeFps;
            unsigned int multiplier = GetFGEffectiveMultiplierForOverlay();
            if (!multiplier) multiplier = 1;
            double outputFps = smoothedNativeFps * static_cast<double>(multiplier);
            if (outputFps < 0.0) outputFps = 0.0;
            if (outputFps > 5000.0) outputFps = 5000.0;
            slOverlayCurrentFpsMilli.store(static_cast<unsigned int>(outputFps * 1000.0 + 0.5), std::memory_order_release);
        }
    }
    previousQpc = now.QuadPart;
}

static void EndSLPresentFrame(unsigned long long present, sl::FrameToken* token) noexcept {
    if (!token || !slPCLSetMarkerApi) return;
    const sl::Result result = slPCLSetMarkerApi(sl::PCLMarker::ePresentEnd, *token);
    if (result == sl::Result::eOk) ++slPresentMarkerEnds;
    if (present <= 8 || (present % 240) == 0)
        Log("SL_PRESENT_MARKER_END present=%llu token=%p result=%lld ends=%llu", present, token, SLResultCode(result), slPresentMarkerEnds.load());
    UpdateFGOverlayCurrentFps();
}

static void ResolveDLSSGFeatureFunctions() noexcept {
    if (!slGetFeatureFunctionApi || !slDeviceConfigured.load()) return;
    if (!slDLSSGSetOptionsApi) {
        void* fn = nullptr;
        const sl::Result result = slGetFeatureFunctionApi(sl::kFeatureDLSS_G, "slDLSSGSetOptions", fn);
        slDLSSGSetOptionsApi = result == sl::Result::eOk ? reinterpret_cast<PFun_slDLSSGSetOptions*>(fn) : nullptr;
        Log("SL_DLSSG_FUNCTION name=slDLSSGSetOptions result=%lld function=%p", SLResultCode(result), fn);
    }
    if (!slDLSSGGetStateApi) {
        void* fn = nullptr;
        const sl::Result result = slGetFeatureFunctionApi(sl::kFeatureDLSS_G, "slDLSSGGetState", fn);
        slDLSSGGetStateApi = result == sl::Result::eOk ? reinterpret_cast<PFun_slDLSSGGetState*>(fn) : nullptr;
        Log("SL_DLSSG_FUNCTION name=slDLSSGGetState result=%lld function=%p", SLResultCode(result), fn);
    }
}

static void CacheDLSSGCapabilities(const sl::DLSSGState& state) noexcept {
    // A zero max can be transient during plugin warm-up. Do not cache either fixed
    // or Dynamic capability until Streamline is reporting a settled MFG state.
    if (state.numFramesToGenerateMax != 0) {
        slMfgMaxGenerated.store(state.numFramesToGenerateMax, std::memory_order_release);
        slMfgCapabilityKnown.store(1u, std::memory_order_release);
        slDynamicMfgSupported.store(state.bIsDynamicMFGSupported == sl::Boolean::eTrue ? 1u : 0u, std::memory_order_release);
        slDynamicMfgCapabilityKnown.store(1u, std::memory_order_release);
    }
}

static bool EnsureDLSSGMFGCapability(unsigned long long present, unsigned int requestedGenerated) noexcept {
    if (slMfgCapabilityKnown.load(std::memory_order_acquire)) {
        const unsigned int maxGenerated = slMfgMaxGenerated.load(std::memory_order_acquire);
        const bool supported = requestedGenerated > 0 && maxGenerated >= requestedGenerated;
        slMfgCapabilitySupported.store(supported ? 1u : 0u, std::memory_order_release);
        return supported;
    }

    if (!slDLSSGGetStateApi) return false;
    sl::DLSSGState state{};
    ++slMfgCapabilityChecks;
    const sl::Result result = slDLSSGGetStateApi(slFgViewport, state, nullptr);
    if (result != sl::Result::eOk) {
        Log("SL_DLSSG_MFG_CAPABILITY present=%llu result=%lld requested_generated=%u target_multiplier=%ux status=unknown max_generated=0 supported=0 checks=%llu",
            present, SLResultCode(result), requestedGenerated, requestedGenerated + 1, slMfgCapabilityChecks.load());
        return false;
    }

    CacheDLSSGCapabilities(state);
    if (state.numFramesToGenerateMax == 0) {
        Log("SL_DLSSG_MFG_CAPABILITY present=%llu result=%lld requested_generated=%u target_multiplier=%ux status=0x%X max_generated=0 supported=0 transient=1 dynamic_supported=%u checks=%llu",
            present, SLResultCode(result), requestedGenerated, requestedGenerated + 1, unsigned(state.status),
            unsigned(IsFGDynamicMFGSupported()), slMfgCapabilityChecks.load());
        return false;
    }

    const unsigned int maxGenerated = slMfgMaxGenerated.load(std::memory_order_acquire);
    const bool supported = requestedGenerated > 0 && maxGenerated >= requestedGenerated;
    slMfgCapabilitySupported.store(supported ? 1u : 0u, std::memory_order_release);
    Log("SL_DLSSG_MFG_CAPABILITY present=%llu result=0 requested_generated=%u target_multiplier=%ux max_generated=%u max_multiplier=%ux supported=%u dynamic_supported=%u checks=%llu cached=%u",
        present, requestedGenerated, requestedGenerated + 1, maxGenerated, maxGenerated ? maxGenerated + 1 : 0,
        unsigned(supported), unsigned(IsFGDynamicMFGSupported()), slMfgCapabilityChecks.load(), slMfgCapabilityKnown.load());
    return supported;
}

static bool EnsureDLSSGDynamicCapability(unsigned long long present) noexcept {
    if (slDynamicMfgCapabilityKnown.load(std::memory_order_acquire)) return IsFGDynamicMFGSupported();
    if (!slDLSSGGetStateApi) return false;

    sl::DLSSGState state{};
    ++slDynamicMfgCapabilityChecks;
    const sl::Result result = slDLSSGGetStateApi(slFgViewport, state, nullptr);
    if (result != sl::Result::eOk) {
        Log("SL_DLSSG_DYNAMIC_CAPABILITY present=%llu result=%lld supported=0 status=unknown checks=%llu",
            present, SLResultCode(result), slDynamicMfgCapabilityChecks.load());
        return false;
    }
    CacheDLSSGCapabilities(state);
    Log("SL_DLSSG_DYNAMIC_CAPABILITY present=%llu result=0 supported=%u status=0x%X max_generated=%u max_multiplier=%ux target_policy=%s target_fps=%.3f detected_refresh_fps=%.3f checks=%llu",
        present, unsigned(IsFGDynamicMFGSupported()), unsigned(state.status), state.numFramesToGenerateMax,
        state.numFramesToGenerateMax ? state.numFramesToGenerateMax + 1 : 0, GetFGDynamicTargetPolicyName(),
        static_cast<double>(GetFGDynamicResolvedTargetFrameRate()), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
        slDynamicMfgCapabilityChecks.load());
    return IsFGDynamicMFGSupported();
}

static void SetDLSSGModeForPresent(unsigned long long present, bool frameReady) noexcept {
    if (!slDeviceConfigured.load() || !slControlDlssReady.load()) return;
    bool loaded = false;
    if (!slIsFeatureLoadedApi || slIsFeatureLoadedApi(sl::kFeatureDLSS_G, loaded) != sl::Result::eOk || !loaded) return;
    ResolveDLSSGFeatureFunctions();
    if (!slDLSSGSetOptionsApi) return;

    const unsigned int selected = GetFGUserMultiplier();
    const bool dynamicRequested = IsFGDynamicSelection(selected);
    const unsigned int requestedGenerated = selected >= 2 ? selected - 1 : 0;
    bool enable = frameReady && selected != kSLSelectionOff;
    if (enable && !IsFGSelectionAllowedByGpuPolicy(selected)) {
        Log("FG_GPU_POLICY_BLOCK present=%llu selected_selection=%s selected_code=%u rtx40_series=%u allowed=off,2x action=disable",
            present, GetFGSelectionName(selected), selected, unsigned(IsFGRtx40Series()));
        enable = false;
    }
    if (enable) {
        if (dynamicRequested) {
            if (!EnsureDLSSGDynamicCapability(present)) enable = false;
        } else if (!EnsureDLSSGMFGCapability(present, requestedGenerated)) {
            enable = false;
        }
    }

    const unsigned int desiredEnabled = enable ? 1u : 0u;
    const unsigned int desiredMode = enable ? (dynamicRequested ? 2u : 1u) : 0u;
    const unsigned int desiredDynamicTargetMilliFps = enable && dynamicRequested ? GetFGDynamicResolvedTargetMilliFps() : 0u;
    const unsigned int appliedGenerated = slFgAppliedGeneratedFrames.load(std::memory_order_acquire);
    const unsigned int appliedMode = slFgAppliedMode.load(std::memory_order_acquire);
    const unsigned int appliedDynamicTargetMilliFps = slFgAppliedDynamicTargetMilliFps.load(std::memory_order_acquire);
    if (slFgOptionsInitialized.load(std::memory_order_acquire) &&
        slFgEnabledByApi.load(std::memory_order_acquire) == desiredEnabled &&
        appliedMode == desiredMode &&
        (!enable || (dynamicRequested ? appliedDynamicTargetMilliFps == desiredDynamicTargetMilliFps
                                     : appliedGenerated == requestedGenerated))) return;

    sl::DLSSGOptions options{};
    options.mode = !enable ? sl::DLSSGMode::eOff : (dynamicRequested ? sl::DLSSGMode::eDynamic : sl::DLSSGMode::eOn);
    // Ignored by Streamline in eDynamic, but keep a valid fixed value in the struct.
    options.numFramesToGenerate = requestedGenerated ? requestedGenerated : 1;
    // v1.0.0 retains the v0.8.26 explicit target behavior even in Auto mode. The presentation
    // proxy bypasses Control's requested VSync only while native Dynamic is API-enabled,
    // because NVIDIA documents that dynamicTargetFrameRate is ignored under VSync.
    // If monitor detection has not completed, zero retains the SDK's documented fallback.
    options.dynamicTargetFrameRate = dynamicRequested ? GetFGDynamicResolvedTargetFrameRate() : 0.0f;
    options.mvecDepthWidth = slFgMvecDepthWidth.load();
    options.mvecDepthHeight = slFgMvecDepthHeight.load();
    options.depthBufferFormat = slFgDepthFormat.load();
    options.mvecBufferFormat = slFgMvecFormat.load();
    options.colorWidth = slFgColorWidth.load();
    options.colorHeight = slFgColorHeight.load();
    options.colorBufferFormat = slFgColorFormat.load();
    options.hudLessBufferFormat = slFgHudLessFormat.load();
    if (IsHdr10BridgeActive()) {
        options.colorWidth = GetHdr10BridgeWidth();
        options.colorHeight = GetHdr10BridgeHeight();
        options.colorBufferFormat = static_cast<unsigned int>(DXGI_FORMAT_R10G10B10A2_UNORM);
        options.hudLessBufferFormat = 0;
    }

    const unsigned int wasInitialized = slFgOptionsInitialized.load(std::memory_order_acquire);
    const unsigned int wasEnabled = slFgEnabledByApi.load(std::memory_order_acquire);
    const unsigned int previousAppliedGenerated = slFgAppliedGeneratedFrames.load(std::memory_order_acquire);
    const unsigned int previousAppliedMode = slFgAppliedMode.load(std::memory_order_acquire);
    const unsigned int previousAppliedDynamicTargetMilliFps = slFgAppliedDynamicTargetMilliFps.load(std::memory_order_acquire);
    const sl::Result setResult = slDLSSGSetOptionsApi(slFgViewport, options);
    ++slFgOptionsCalls;
    if (setResult == sl::Result::eOk) {
        slFgEnabledByApi.store(desiredEnabled, std::memory_order_release);
        slFgOptionsInitialized.store(1, std::memory_order_release);
        slFgAppliedMode.store(desiredMode, std::memory_order_release);
        slFgAppliedGeneratedFrames.store(enable && !dynamicRequested ? requestedGenerated : 0u, std::memory_order_release);
        slFgAppliedDynamicTargetMilliFps.store(enable && dynamicRequested ? desiredDynamicTargetMilliFps : 0u, std::memory_order_release);

        const bool fixedMultiplierChanged = wasInitialized && wasEnabled && enable && previousAppliedMode == 1u && desiredMode == 1u && previousAppliedGenerated != requestedGenerated;
        const bool modeChanged = wasInitialized && wasEnabled && enable && previousAppliedMode != desiredMode;
        const bool dynamicTargetChanged = wasInitialized && wasEnabled && enable && previousAppliedMode == 2u && desiredMode == 2u &&
            previousAppliedDynamicTargetMilliFps != desiredDynamicTargetMilliFps;
        const bool selectionChanged = fixedMultiplierChanged || modeChanged || dynamicTargetChanged;
        if (fixedMultiplierChanged) {
            Log("FG_MULTIPLIER_CHANGE present=%llu from_multiplier=%ux to_multiplier=%ux from_generated=%u to_generated=%u selected_selection=%s hdr10_bridge=%u",
                present, previousAppliedGenerated + 1, requestedGenerated + 1, previousAppliedGenerated, requestedGenerated,
                GetFGSelectionName(selected), unsigned(IsHdr10BridgeActive()));
        }
        if (modeChanged) {
            Log("FG_MODE_CHANGE present=%llu from_mode=%s to_mode=%s selected_selection=%s dynamic_target_policy=%s dynamic_target_fps=%.3f hdr10_bridge=%u",
                present, previousAppliedMode == 2u ? "dynamic" : "fixed", desiredMode == 2u ? "dynamic" : "fixed",
                GetFGSelectionName(selected), GetFGDynamicTargetPolicyName(), static_cast<double>(options.dynamicTargetFrameRate),
                unsigned(IsHdr10BridgeActive()));
        }
        if (dynamicTargetChanged) {
            Log("FG_DYNAMIC_TARGET_CHANGE present=%llu previous_target_fps=%.3f target_fps=%.3f policy=%s manual_target_fps=%u detected_refresh_fps=%.3f hdr10_bridge=%u",
                present, static_cast<double>(previousAppliedDynamicTargetMilliFps) / 1000.0,
                static_cast<double>(desiredDynamicTargetMilliFps) / 1000.0, GetFGDynamicTargetPolicyName(),
                GetFGDynamicManualTargetFps(), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                unsigned(IsHdr10BridgeActive()));
        }
        if (selectionChanged) slFgGenerationConfirmedCurrentSegment.store(0, std::memory_order_release);

        if (enable && (!wasEnabled || selectionChanged)) {
            const unsigned long long enableOrdinal = ++slFgEnableTransitions;
            const unsigned int previousWidth = slFgLastEnabledMvecWidth.exchange(options.mvecDepthWidth);
            const unsigned int previousHeight = slFgLastEnabledMvecHeight.exchange(options.mvecDepthHeight);
            const bool resolutionChanged = enableOrdinal > 1 &&
                (previousWidth != options.mvecDepthWidth || previousHeight != options.mvecDepthHeight);
            if (resolutionChanged) ++slFgResolutionTransitions;
            slFgGenerationConfirmedCurrentSegment.store(0, std::memory_order_release);
            Log("FG_MODE_SEGMENT present=%llu segment=%llu mode=%s selected_selection=%s requested_generated=%u target_multiplier=%ux dynamic_target_policy=%s dynamic_target_fps=%.3f detected_refresh_fps=%.3f input=%ux%u color=%ux%u resolution_changed=%u previous_input=%ux%u enable_transitions=%llu disable_transitions=%llu",
                present, enableOrdinal, dynamicRequested ? "dynamic" : "fixed", GetFGSelectionName(selected), requestedGenerated,
                dynamicRequested ? 0u : selected, dynamicRequested ? GetFGDynamicTargetPolicyName() : "n/a",
                dynamicRequested ? static_cast<double>(options.dynamicTargetFrameRate) : 0.0,
                static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                options.mvecDepthWidth, options.mvecDepthHeight, options.colorWidth, options.colorHeight,
                unsigned(resolutionChanged), previousWidth, previousHeight, slFgEnableTransitions.load(), slFgDisableTransitions.load());
        } else if (!enable && wasEnabled) {
            const unsigned long long disableOrdinal = ++slFgDisableTransitions;
            Log("FG_MODE_SEGMENT present=%llu segment=%llu mode=off selected_selection=%s last_applied_mode=%s last_requested_generated=%u last_input=%ux%u enable_transitions=%llu disable_transitions=%llu generated_segments=%llu",
                present, disableOrdinal, GetFGSelectionName(selected), previousAppliedMode == 2u ? "dynamic" : "fixed", previousAppliedGenerated,
                options.mvecDepthWidth, options.mvecDepthHeight, slFgEnableTransitions.load(), slFgDisableTransitions.load(), slFgGeneratedSegments.load());
        }
    }

    Log("SL_DLSSG_MODE present=%llu viewport=0 mode=%s selected_selection=%s selected_code=%u generated_frames_requested=%u options_num_frames_to_generate=%u target_multiplier=%ux dynamic_target_policy=%s dynamic_target_fps=%.3f manual_target_fps=%u detected_refresh_fps=%.3f result=%lld api_enabled=%u options_calls=%llu frame_gate=constants_plus_depth_mv input=%ux%u depth_fmt=%u mv_fmt=%u color=%ux%u color_fmt=%u hudless_fmt=%u hdr10_bridge=%u dynamic_supported=%u max_multiplier=%ux",
        present, !enable ? "off" : (dynamicRequested ? "dynamic" : "on"), GetFGSelectionName(selected), selected,
        requestedGenerated, options.numFramesToGenerate, dynamicRequested ? 0u : selected, dynamicRequested ? GetFGDynamicTargetPolicyName() : "n/a",
        dynamicRequested ? static_cast<double>(options.dynamicTargetFrameRate) : 0.0, GetFGDynamicManualTargetFps(),
        static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0, SLResultCode(setResult),
        slFgEnabledByApi.load(), slFgOptionsCalls.load(), options.mvecDepthWidth, options.mvecDepthHeight,
        options.depthBufferFormat, options.mvecBufferFormat, options.colorWidth, options.colorHeight,
        options.colorBufferFormat, options.hudLessBufferFormat, unsigned(IsHdr10BridgeActive()),
        unsigned(IsFGDynamicMFGSupported()), GetFGMaxSupportedMultiplier());

    if (slDLSSGGetStateApi) {
        sl::DLSSGState state{};
        const sl::Result stateResult = slDLSSGGetStateApi(slFgViewport, state, nullptr);
        if (stateResult == sl::Result::eOk) CacheDLSSGCapabilities(state);
        Log("SL_DLSSG_STATE present=%llu result=%lld status=0x%X frames_presented=%u max_generated=%u min_dimension=%u selected_selection=%s dynamic_supported=%u",
            present, SLResultCode(stateResult), unsigned(state.status), state.numFramesActuallyPresented,
            state.numFramesToGenerateMax, state.minWidthOrHeight, GetFGSelectionName(selected), unsigned(IsFGDynamicMFGSupported()));
    }
}

static bool PrepareSLPrivateDeviceProxy(ID3D12Device* baseDevice) noexcept {
    if (!baseDevice || !slUpgradeInterfaceApi || !slGetNativeInterfaceApi || slBootstrapState.load() != 2) {
        Log("SL_DEVICE_PROXY_SKIP reason=core_not_ready native_device=%p bootstrap_state=%u", baseDevice, slBootstrapState.load());
        return false;
    }
    if (slPrivateDeviceProxy.load()) return true;

    void* upgraded = baseDevice;
    Log("SL_DEVICE_PROXY_BEGIN native_device=%p", baseDevice);
    const sl::Result result = slUpgradeInterfaceApi(&upgraded);
    void* unwrapped = nullptr;
    sl::Result unwrapResult = sl::Result::eErrorInvalidParameter;
    if (result == sl::Result::eOk && upgraded && upgraded != baseDevice) {
        unwrapResult = slGetNativeInterfaceApi(upgraded, &unwrapped);
    }
    const bool nativeMatch = unwrapResult == sl::Result::eOk && unwrapped == baseDevice;
    Log("SL_DEVICE_PROXY_END result=%lld native_device=%p proxy_device=%p unwrap_result=%lld unwrapped=%p native_match=%u host_device_return=native",
        SLResultCode(result), baseDevice, upgraded, SLResultCode(unwrapResult), unwrapped, unsigned(nativeMatch));
    if (unwrapped) reinterpret_cast<IUnknown*>(unwrapped)->Release();
    if (result != sl::Result::eOk || !upgraded || upgraded == baseDevice || !nativeMatch) {
        if (upgraded && upgraded != baseDevice) reinterpret_cast<IUnknown*>(upgraded)->Release();
        return false;
    }

    auto proxy = reinterpret_cast<ID3D12Device*>(upgraded);
    ID3D12Device* expected = nullptr;
    if (!slPrivateDeviceProxy.compare_exchange_strong(expected, proxy)) {
        proxy->Release();
        return expected != nullptr;
    }
    slPrivateDeviceNativeIdentity.store(baseDevice);
    Log("SL_DEVICE_PROXY_READY native_device=%p proxy_device=%p host_device_return=native queue_route=exact_constructor_only fg_api_enabled=%u",
        baseDevice, proxy, slFgEnabledByApi.load());
    return true;
}


static void TryConfigureSLNativeDeviceEarly(ID3D12Device* baseDevice, const char* trigger) noexcept {
    if (!baseDevice || slDeviceConfigured.load() || slDeviceBindAttempted.load()) return;
    if (slBootstrapState.load() != 2 || !slSetD3DDeviceApi) {
        Log("SL_DEVICE_BIND_EARLY_SKIP reason=core_not_ready trigger=%s bootstrap_state=%u",
            trigger ? trigger : "unknown", slBootstrapState.load());
        return;
    }

    unsigned int expected = 0;
    if (!slDeviceBindAttempted.compare_exchange_strong(expected, 1)) return;

    const LUID luid = baseDevice->GetAdapterLuid();
    CacheFGGpuClassFromLuid(luid);
    Log("SL_DEVICE_BIND_BEGIN native_device=%p adapter_luid=%08lX:%08lX trigger=%s ordering=before_control_ngx dual_ngx_paths=1",
        baseDevice, static_cast<unsigned long>(luid.HighPart), luid.LowPart, trigger ? trigger : "unknown");
    const sl::Result result = slSetD3DDeviceApi(baseDevice);
    Log("SL_DEVICE_BIND_END native_device=%p result=%lld trigger=%s ordering=before_control_ngx dual_ngx_paths=1",
        baseDevice, SLResultCode(result), trigger ? trigger : "unknown");
    if (result == sl::Result::eOk) {
        slDeviceConfigured.store(1);
        Log("SL_DEVICE_READY native_device=%p features_requested=3 control_dlss_ready=%u ordering=before_control_ngx dual_ngx_paths=1 factory_upgrade=presentation_only swapchain_upgrade=via_factory queue_hook=1 fg_enabled=0",
            baseDevice, slControlDlssReady.load());
        ProbeSLFeatureGate(luid);
        ResolveSLFrameFeatureFunctions();
        ResolveDLSSGFeatureFunctions();
        ConfigureSLReflexForCurrentFG(presentCount.load());
    } else {
        Log("SL_DEVICE_BIND_FAILED result=%lld ordering=before_control_ngx dual_ngx_paths=1 fg_enabled=0", SLResultCode(result));
    }
}

static void CaptureSLNativeDeviceForDeferredBind(void* returnedInterface) noexcept {
    if (!returnedInterface) return;

    ID3D12Device* baseDevice = nullptr;
    HRESULT query = E_NOINTERFACE;
    __try {
        query = reinterpret_cast<IUnknown*>(returnedInterface)->QueryInterface(
            __uuidof(ID3D12Device), reinterpret_cast<void**>(&baseDevice));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("SL_DEVICE_QUERY_EXCEPTION exception=0x%08lX returned_interface=%p", GetExceptionCode(), returnedInterface);
        return;
    }
    if (FAILED(query) || !baseDevice) {
        Log("SL_DEVICE_QUERY_FAILED hr=0x%08lX returned_interface=%p", static_cast<unsigned long>(query), returnedInterface);
        return;
    }

    ID3D12Device* expected = nullptr;
    if (!slPendingNativeDevice.compare_exchange_strong(expected, baseDevice)) {
        Log("SL_DEVICE_CAPTURE_SKIPPED reason=already_captured captured=%p returned_interface=%p", expected, returnedInterface);
        baseDevice->Release();
        return;
    }

    const LUID luid = baseDevice->GetAdapterLuid();
    CacheFGGpuClassFromLuid(luid);
    Log("SL_DEVICE_CAPTURED native_device=%p returned_interface=%p adapter_luid=%08lX:%08lX device_set=%u",
        baseDevice, returnedInterface, static_cast<unsigned long>(luid.HighPart), luid.LowPart, slDeviceConfigured.load());
    PrepareSLPrivateDeviceProxy(baseDevice);
    TryConfigureSLNativeDeviceEarly(baseDevice, "d3d12_create_device_return");
    if (slDeviceBindAttempted.load()) {
        ID3D12Device* held = slPendingNativeDevice.exchange(nullptr);
        if (held) held->Release();
    } else {
        Log("SL_DEVICE_BIND_FALLBACK_PENDING reason=early_bind_not_attempted native_device=%p control_dlss_ready=%u fg_enabled=0",
            baseDevice, slControlDlssReady.load());
    }
}

static void MarkControlDLSSReady(unsigned long long call, bool success) noexcept {
    if (!success) return;
    unsigned int expected = 0;
    if (!slControlDlssReady.compare_exchange_strong(expected, 1)) return;
    Log("CONTROL_DLSS_READY call=%llu present=%llu native_dlss_success=1 pending_device=%p device_set=%u",
        call, presentCount.load(), slPendingNativeDevice.load(), slDeviceConfigured.load());
}

static void TryConfigureSLNativeDeviceDeferred(const char* trigger) noexcept {
    if (!slControlDlssReady.load()) return;
    if (slDeviceConfigured.load() || slDeviceBindAttempted.load()) return;
    if (slBootstrapState.load() != 2 || !slSetD3DDeviceApi) {
        Log("SL_DEVICE_BIND_DEFERRED_SKIP reason=core_not_ready trigger=%s bootstrap_state=%u",
            trigger ? trigger : "unknown", slBootstrapState.load());
        return;
    }

    ID3D12Device* baseDevice = slPendingNativeDevice.load();
    if (!baseDevice) {
        Log("SL_DEVICE_BIND_DEFERRED_SKIP reason=device_not_captured trigger=%s", trigger ? trigger : "unknown");
        return;
    }
    if (!slPrivateDeviceProxy.load()) PrepareSLPrivateDeviceProxy(baseDevice);

    unsigned int expected = 0;
    if (!slDeviceBindAttempted.compare_exchange_strong(expected, 1)) return;

    const LUID luid = baseDevice->GetAdapterLuid();
    CacheFGGpuClassFromLuid(luid);
    Log("SL_DEVICE_BIND_BEGIN native_device=%p adapter_luid=%08lX:%08lX trigger=%s ordering=after_control_dlss",
        baseDevice, static_cast<unsigned long>(luid.HighPart), luid.LowPart, trigger ? trigger : "unknown");
    const sl::Result result = slSetD3DDeviceApi(baseDevice);
    Log("SL_DEVICE_BIND_END native_device=%p result=%lld trigger=%s ordering=after_control_dlss",
        baseDevice, SLResultCode(result), trigger ? trigger : "unknown");
    if (result == sl::Result::eOk) {
        slDeviceConfigured.store(1);
        Log("SL_DEVICE_READY native_device=%p features_requested=3 control_dlss_ready=1 ordering=after_control_dlss factory_upgrade=presentation_only swapchain_upgrade=via_factory queue_hook=1 fg_enabled=0", baseDevice);
        ProbeSLFeatureGate(luid);
        ResolveSLFrameFeatureFunctions();
        ResolveDLSSGFeatureFunctions();
        ConfigureSLReflexForCurrentFG(presentCount.load());
    } else {
        Log("SL_DEVICE_BIND_FAILED result=%lld control_dlss_ready=1 fg_enabled=0", SLResultCode(result));
    }

    ID3D12Device* held = slPendingNativeDevice.exchange(nullptr);
    if (held) held->Release();
}



// Shared x64 rel32 relay helpers. Keep these available even in native-interface
// isolation builds because the build-locked Control NGX callsite hook still
// uses a nearby executable relay.
static bool Rel32Fits(const unsigned char* fromAfterInstruction, const void* to) noexcept {
    const long long from = static_cast<long long>(reinterpret_cast<uintptr_t>(fromAfterInstruction));
    const long long dest = static_cast<long long>(reinterpret_cast<uintptr_t>(to));
    const long long delta = dest - from;
    return delta >= -2147483648LL && delta <= 2147483647LL;
}

static void* AllocateExecutableRelayNear(const void* origin) noexcept {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const uintptr_t minApp = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);
    const uintptr_t maxApp = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    const uintptr_t base = reinterpret_cast<uintptr_t>(origin);
    const uintptr_t span = 0x7FFF0000ull;
    uintptr_t low = base > span ? base - span : minApp;
    if (low < minApp) low = minApp;
    uintptr_t high = base + span;
    if (high < base || high > maxApp) high = maxApp;
    const uintptr_t gran = si.dwAllocationGranularity ? si.dwAllocationGranularity : 0x10000u;

    uintptr_t cursor = low;
    while (cursor < high) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &mbi, sizeof(mbi))) break;
        uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t regionEnd = regionBase + mbi.RegionSize;
        if (regionEnd <= cursor) break;
        if (mbi.State == MEM_FREE) {
            uintptr_t candidate = (regionBase + gran - 1u) & ~(gran - 1u);
            if (candidate < low) candidate = (low + gran - 1u) & ~(gran - 1u);
            if (candidate + 0x1000u <= regionEnd && candidate + 0x1000u <= high) {
                void* mem = VirtualAlloc(reinterpret_cast<void*>(candidate), 0x1000,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                if (mem) return mem;
            }
        }
        cursor = regionEnd;
    }
    return nullptr;
}

static void* HookControlCommandQueueCtor(void* self, unsigned int queueType) {
    if (!controlOriginalCommandQueueCtor) return self;
    ID3D12Device* proxyDevice = slPrivateDeviceProxy.load();
    ID3D12Device* nativeIdentity = slPrivateDeviceNativeIdentity.load();
    if (!verifiedD3d || !proxyDevice || !nativeIdentity || queueType > 2) {
        Log("SL_QUEUE_ROUTE_SKIP reason=proxy_unavailable queue_type=%u native=%p proxy=%p bootstrap_state=%u",
            queueType, nativeIdentity, proxyDevice, slBootstrapState.load());
        return controlOriginalCommandQueueCtor(self, queueType);
    }

    auto slot = reinterpret_cast<ID3D12Device**>(
        reinterpret_cast<unsigned char*>(verifiedD3d) + kControlGlobalD3D12DevicePointerRva);
    AcquireSRWLockExclusive(&slQueueRouteLock);
    ID3D12Device* observed = nullptr;
    __try { observed = *slot; } __except (EXCEPTION_EXECUTE_HANDLER) { observed = nullptr; }
    if (observed != nativeIdentity) {
        ReleaseSRWLockExclusive(&slQueueRouteLock);
        Log("SL_QUEUE_ROUTE_SKIP reason=global_device_mismatch queue_type=%u observed=%p expected_native=%p proxy=%p",
            queueType, observed, nativeIdentity, proxyDevice);
        return controlOriginalCommandQueueCtor(self, queueType);
    }

    *slot = proxyDevice;
    const unsigned int ordinal = ++slQueueRouteCalls;
    const char* typeName = queueType == 0 ? "direct" : queueType == 1 ? "compute" : "copy";
    Log("SL_QUEUE_ROUTE_BEGIN ordinal=%u queue_type=%u queue_name=%s native_device=%p proxy_device=%p global_slot=%p",
        ordinal, queueType, typeName, nativeIdentity, proxyDevice, slot);

    void* result = nullptr;
    __try {
        result = controlOriginalCommandQueueCtor(self, queueType);
    } __finally {
        *slot = nativeIdentity;
        ReleaseSRWLockExclusive(&slQueueRouteLock);
        if (AbnormalTermination()) {
            Log("SL_QUEUE_ROUTE_EXCEPTION ordinal=%u queue_type=%u restored_native=1", ordinal, queueType);
        }
    }

    ID3D12CommandQueue* queue = nullptr;
    __try {
        queue = self ? *reinterpret_cast<ID3D12CommandQueue**>(
            reinterpret_cast<unsigned char*>(self) + 0x10) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) { queue = nullptr; }
    void* nativeQueue = nullptr;
    sl::Result unwrapResult = sl::Result::eErrorInvalidParameter;
    if (queue && slGetNativeInterfaceApi) unwrapResult = slGetNativeInterfaceApi(queue, &nativeQueue);
    const bool proxyQueue = unwrapResult == sl::Result::eOk && nativeQueue && nativeQueue != queue;
    if (queueType == 0 && queue && proxyQueue) {
        ID3D12CommandQueue* expectedQueue = nullptr;
        queue->AddRef();
        if (!slHdr10DirectQueue.compare_exchange_strong(expectedQueue, queue, std::memory_order_acq_rel)) {
            queue->Release();
        } else {
            Log("HDR10_BRIDGE_QUEUE_CAPTURE queue=%p native_queue=%p source=control_direct_streamline_proxy", queue, nativeQueue);
        }
    }
    Log("SL_QUEUE_ROUTE_END ordinal=%u queue_type=%u queue_name=%s object=%p queue=%p unwrap_result=%lld native_queue=%p proxy_queue=%u global_restored_native=1",
        ordinal, queueType, typeName, self, queue, SLResultCode(unwrapResult), nativeQueue, unsigned(proxyQueue));
    if (nativeQueue) reinterpret_cast<IUnknown*>(nativeQueue)->Release();
    return result;
}

static bool ValidateControlCommandQueueCtorCallsite(HMODULE d3d) noexcept {
    if (!d3d) return false;
    auto base = reinterpret_cast<unsigned char*>(d3d);
    auto call = base + kControlCommandQueueCtorCallRva;
    const unsigned char expected[5] = {0xE8, 0xFD, 0x7D, 0x01, 0x00};
    if (memcmp(call, expected, sizeof(expected)) != 0) return false;
    int32_t rel = 0;
    memcpy(&rel, call + 1, sizeof(rel));
    auto target = call + 5 + rel;
    if (target != base + kControlCommandQueueCtorRva) return false;
    controlCommandQueueCtorCallsite = call;
    controlOriginalCommandQueueCtor = reinterpret_cast<ControlCommandQueueCtorFn>(target);
    memcpy(controlCommandQueueCtorOriginalCall, call, sizeof(controlCommandQueueCtorOriginalCall));
    Log("SL_QUEUE_CTOR_TARGET callsite=%p original=%p call_rva=0x%zX target_rva=0x%zX global_device_rva=0x%zX",
        call, target, kControlCommandQueueCtorCallRva, kControlCommandQueueCtorRva,
        kControlGlobalD3D12DevicePointerRva);
    return true;
}

static bool PrepareControlCommandQueueCtorRelay() noexcept {
    if (!controlCommandQueueCtorCallsite || !controlOriginalCommandQueueCtor) return false;
    controlCommandQueueCtorRelay = AllocateExecutableRelayNear(controlCommandQueueCtorCallsite);
    if (!controlCommandQueueCtorRelay) return false;
    auto relay = reinterpret_cast<unsigned char*>(controlCommandQueueCtorRelay);
    relay[0] = 0x48; relay[1] = 0xB8;
    const uintptr_t hook = reinterpret_cast<uintptr_t>(&HookControlCommandQueueCtor);
    memcpy(relay + 2, &hook, sizeof(hook));
    relay[10] = 0xFF; relay[11] = 0xE0;
    FlushInstructionCache(GetCurrentProcess(), relay, 12);
    DWORD old = 0;
    if (!VirtualProtect(relay, 0x1000, PAGE_EXECUTE_READ, &old)) {
        VirtualFree(relay, 0, MEM_RELEASE);
        controlCommandQueueCtorRelay = nullptr;
        return false;
    }
    if (!Rel32Fits(controlCommandQueueCtorCallsite + 5, relay)) {
        VirtualFree(relay, 0, MEM_RELEASE);
        controlCommandQueueCtorRelay = nullptr;
        return false;
    }
    Log("SL_QUEUE_CTOR_RELAY relay=%p hook=%p", relay, reinterpret_cast<void*>(&HookControlCommandQueueCtor));
    return true;
}

static bool InstallControlCommandQueueCtorCallHook() noexcept {
    if (!controlCommandQueueCtorCallsite || !controlCommandQueueCtorRelay) return false;
    const intptr_t relayAddress = reinterpret_cast<intptr_t>(controlCommandQueueCtorRelay);
    const intptr_t returnAddress = reinterpret_cast<intptr_t>(controlCommandQueueCtorCallsite + 5);
    const int64_t delta64 = static_cast<int64_t>(relayAddress) - static_cast<int64_t>(returnAddress);
    if (delta64 < INT32_MIN || delta64 > INT32_MAX) return false;
    const int32_t delta = static_cast<int32_t>(delta64);
    unsigned char patched[5] = {0xE8, 0, 0, 0, 0};
    memcpy(patched + 1, &delta, sizeof(delta));
    DWORD old = 0;
    if (!VirtualProtect(controlCommandQueueCtorCallsite, sizeof(patched), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(controlCommandQueueCtorCallsite, patched, sizeof(patched));
    FlushInstructionCache(GetCurrentProcess(), controlCommandQueueCtorCallsite, sizeof(patched));
    DWORD ignored = 0;
    const BOOL restored = VirtualProtect(controlCommandQueueCtorCallsite, sizeof(patched), old, &ignored);
    if (!restored) Log("PROTECTION_RESTORE_FAILED label=CONTROL_COMMAND_QUEUE_CTOR error=%lu", GetLastError());
    return memcmp(controlCommandQueueCtorCallsite, patched, sizeof(patched)) == 0;
}

static bool RollbackControlCommandQueueCtorCallHook() noexcept {
    bool ok = true;
    if (controlCommandQueueCtorCallsite) {
        DWORD old = 0;
        if (VirtualProtect(controlCommandQueueCtorCallsite, sizeof(controlCommandQueueCtorOriginalCall), PAGE_EXECUTE_READWRITE, &old)) {
            memcpy(controlCommandQueueCtorCallsite, controlCommandQueueCtorOriginalCall, sizeof(controlCommandQueueCtorOriginalCall));
            FlushInstructionCache(GetCurrentProcess(), controlCommandQueueCtorCallsite, sizeof(controlCommandQueueCtorOriginalCall));
            DWORD ignored = 0;
            if (!VirtualProtect(controlCommandQueueCtorCallsite, sizeof(controlCommandQueueCtorOriginalCall), old, &ignored)) ok = false;
        } else ok = false;
    }
    if (controlCommandQueueCtorRelay) {
        if (!VirtualFree(controlCommandQueueCtorRelay, 0, MEM_RELEASE)) ok = false;
        controlCommandQueueCtorRelay = nullptr;
    }
    return ok;
}


static unsigned int HookControlNgxInitProject(
    const char* projectId, int engineType, const char* engineVersion,
    const wchar_t* applicationDataPath, ID3D12Device* device,
    const ControlNgxFeatureCommonInfo* featureInfo, unsigned int sdkVersion) {
    if (!controlOriginalNgxInitProject) return 0xBAD00000u;

    const std::wstring fgDll = slRuntimeDirectory.empty()
        ? std::wstring{} : slRuntimeDirectory + L"\\nvngx_dlssg.dll";
    if (slRuntimeDirectory.empty() || !FileExists(fgDll)) {
        Log("CONTROL_NGX_INIT_AUGMENT_SKIP reason=runtime_unavailable project=%s feature_info=%p",
            projectId ? projectId : "(null)", featureInfo);
        return controlOriginalNgxInitProject(projectId, engineType, engineVersion,
            applicationDataPath, device, featureInfo, sdkVersion);
    }

    // The exact supported Control binary passes FeatureInfo=null at this callsite.
    // If that ever changes despite the file hash lock, fail open to the game's data
    // rather than guessing how to merge an unknown client-owned path list.
    if (featureInfo) {
        Log("CONTROL_NGX_INIT_AUGMENT_SKIP reason=unexpected_nonnull_feature_info feature_info=%p",
            featureInfo);
        return controlOriginalNgxInitProject(projectId, engineType, engineVersion,
            applicationDataPath, device, featureInfo, sdkVersion);
    }

    const wchar_t* extraPaths[1] = { slRuntimeDirectory.c_str() };
    ControlNgxFeatureCommonInfo augmented{};
    augmented.PathListInfo.Path = extraPaths;
    augmented.PathListInfo.Length = 1;

    const unsigned int ordinal = ++controlNgxPathAugmentCalls;
    Log("CONTROL_NGX_INIT_AUGMENT call=%u project=%s engine_type=%d engine_version=%s app_data=%ls device=%p sdk=0x%X extra_path=%ls feature_info_original=null",
        ordinal, projectId ? projectId : "(null)", engineType,
        engineVersion ? engineVersion : "(null)",
        applicationDataPath ? applicationDataPath : L"(null)", device, sdkVersion,
        slRuntimeDirectory.c_str());

    const unsigned int result = controlOriginalNgxInitProject(projectId, engineType,
        engineVersion, applicationDataPath, device, &augmented, sdkVersion);
    Log("CONTROL_NGX_INIT_RETURN call=%u result=0x%08X project=%s device=%p",
        ordinal, result, projectId ? projectId : "(null)", device);
    return result;
}

static bool ValidateControlNgxInitCallsite(HMODULE d3d) noexcept {
    if (!d3d) return false;
    auto base = reinterpret_cast<unsigned char*>(d3d);
    auto call = base + kControlNgxInitCallRva;
    const unsigned char expected[5] = {0xE8, 0x31, 0x44, 0x03, 0x00};
    if (memcmp(call, expected, sizeof(expected)) != 0) return false;
    int32_t rel = 0;
    memcpy(&rel, call + 1, sizeof(rel));
    auto target = call + 5 + rel;
    if (target != base + kControlNgxInitWrapperRva) return false;
    controlNgxInitCallsite = call;
    controlOriginalNgxInitProject = reinterpret_cast<ControlNgxInitProjectFn>(target);
    memcpy(controlNgxInitOriginalCall, call, sizeof(controlNgxInitOriginalCall));
    Log("CONTROL_NGX_INIT_TARGET callsite=%p original=%p call_rva=0x%zX target_rva=0x%zX",
        call, target, kControlNgxInitCallRva, kControlNgxInitWrapperRva);
    return true;
}

static bool PrepareControlNgxInitRelay() noexcept {
    if (!controlNgxInitCallsite || !controlOriginalNgxInitProject) return false;
    controlNgxInitRelay = AllocateExecutableRelayNear(controlNgxInitCallsite);
    if (!controlNgxInitRelay) return false;
    auto relay = reinterpret_cast<unsigned char*>(controlNgxInitRelay);
    relay[0] = 0x48; relay[1] = 0xB8; // mov rax, imm64
    const uintptr_t hook = reinterpret_cast<uintptr_t>(&HookControlNgxInitProject);
    memcpy(relay + 2, &hook, sizeof(hook));
    relay[10] = 0xFF; relay[11] = 0xE0; // jmp rax
    FlushInstructionCache(GetCurrentProcess(), relay, 12);
    DWORD old = 0;
    if (!VirtualProtect(relay, 0x1000, PAGE_EXECUTE_READ, &old)) {
        VirtualFree(relay, 0, MEM_RELEASE);
        controlNgxInitRelay = nullptr;
        return false;
    }
    if (!Rel32Fits(controlNgxInitCallsite + 5, relay)) {
        VirtualFree(relay, 0, MEM_RELEASE);
        controlNgxInitRelay = nullptr;
        return false;
    }
    Log("CONTROL_NGX_INIT_RELAY relay=%p hook=%p", relay, reinterpret_cast<void*>(&HookControlNgxInitProject));
    return true;
}

static bool InstallControlNgxInitCallHook() noexcept {
    if (!controlNgxInitCallsite || !controlNgxInitRelay) return false;
    const intptr_t relayAddress = reinterpret_cast<intptr_t>(controlNgxInitRelay);
    const intptr_t returnAddress = reinterpret_cast<intptr_t>(controlNgxInitCallsite + 5);
    const int64_t delta64 = static_cast<int64_t>(relayAddress) - static_cast<int64_t>(returnAddress);
    if (delta64 < INT32_MIN || delta64 > INT32_MAX) return false;
    const int32_t delta = static_cast<int32_t>(delta64);
    unsigned char patched[5] = {0xE8, 0, 0, 0, 0};
    memcpy(patched + 1, &delta, sizeof(delta));
    DWORD old = 0;
    if (!VirtualProtect(controlNgxInitCallsite, sizeof(patched), PAGE_EXECUTE_READWRITE, &old))
        return false;
    memcpy(controlNgxInitCallsite, patched, sizeof(patched));
    FlushInstructionCache(GetCurrentProcess(), controlNgxInitCallsite, sizeof(patched));
    DWORD ignored = 0;
    const BOOL restored = VirtualProtect(controlNgxInitCallsite, sizeof(patched), old, &ignored);
    if (!restored) Log("PROTECTION_RESTORE_FAILED label=CONTROL_NGX_INIT_PROJECT error=%lu", GetLastError());
    return memcmp(controlNgxInitCallsite, patched, sizeof(patched)) == 0;
}

static bool RollbackControlNgxInitCallHook() noexcept {
    bool ok = true;
    if (controlNgxInitCallsite) {
        DWORD old = 0;
        if (VirtualProtect(controlNgxInitCallsite, sizeof(controlNgxInitOriginalCall), PAGE_EXECUTE_READWRITE, &old)) {
            memcpy(controlNgxInitCallsite, controlNgxInitOriginalCall, sizeof(controlNgxInitOriginalCall));
            FlushInstructionCache(GetCurrentProcess(), controlNgxInitCallsite, sizeof(controlNgxInitOriginalCall));
            DWORD ignored = 0;
            if (!VirtualProtect(controlNgxInitCallsite, sizeof(controlNgxInitOriginalCall), old, &ignored)) ok = false;
        } else ok = false;
    }
    if (controlNgxInitRelay) {
        if (!VirtualFree(controlNgxInitRelay, 0, MEM_RELEASE)) ok = false;
        controlNgxInitRelay = nullptr;
    }
    return ok;
}

static HRESULT WINAPI HookD3D12CreateDevice(IUnknown* adapter, D3D_FEATURE_LEVEL minimumFeatureLevel,
                                             REFIID iid, void** device) {
    if (!slOriginalD3D12CreateDevice) {
        if (device) *device = nullptr;
        return E_NOINTERFACE;
    }
    const HRESULT hr = slOriginalD3D12CreateDevice(adapter, minimumFeatureLevel, iid, device);
    void* returned = (SUCCEEDED(hr) && device) ? *device : nullptr;
    Log("D3D12_CREATE_DEVICE_RETURN hr=0x%08lX adapter=%p feature_level=0x%X iid=%08lX returned=%p bootstrap_state=%u device_set=%u control_dlss_ready=%u",
        static_cast<unsigned long>(hr), adapter, unsigned(minimumFeatureLevel), iid.Data1, returned,
        slBootstrapState.load(), slDeviceConfigured.load(), slControlDlssReady.load());
    if (SUCCEEDED(hr) && returned) CaptureSLNativeDeviceForDeferredBind(returned);
    return hr;
}

static void SampleDLSSGState(unsigned long long present) noexcept {
    if (!slDeviceConfigured.load()) return;
    ResolveDLSSGFeatureFunctions();
    if (!slDLSSGGetStateApi) return;
    sl::DLSSGState state{};
    const sl::Result result = slDLSSGGetStateApi(slFgViewport, state, nullptr);
    const unsigned int selected = GetFGUserMultiplier();
    const bool dynamicSelected = IsFGDynamicSelection(selected);
    const unsigned int requestedGenerated = selected >= 2 ? selected - 1 : 0;
    bool firstGeneratedNow = false;
    bool firstTargetMultiplierNow = false;
    bool firstDynamicNow = false;
    if (result == sl::Result::eOk) {
        slFgLastFramesPresented.store(state.numFramesActuallyPresented, std::memory_order_release);
        slFgLastStatePresent.store(present, std::memory_order_release);
        slFgLastStateSelection.store(selected, std::memory_order_release);
        CacheDLSSGCapabilities(state);
        unsigned int maxObserved = slFgMaxFramesPresentedObserved.load(std::memory_order_acquire);
        while (state.numFramesActuallyPresented > maxObserved &&
               !slFgMaxFramesPresentedObserved.compare_exchange_weak(maxObserved, state.numFramesActuallyPresented)) {}

        if (state.numFramesActuallyPresented > 1) {
            ++slFgGeneratedStateSamples;
            unsigned int expected = 0;
            if (slFirstGeneratedFrameLogged.compare_exchange_strong(expected, 1)) {
                firstGeneratedNow = true;
                Log("FG_FIRST_GENERATED_FRAME present=%llu frames_presented=%u status=0x%X selected_selection=%s requested_generated=%u mode=%s",
                    present, state.numFramesActuallyPresented, unsigned(state.status), GetFGSelectionName(selected), requestedGenerated,
                    dynamicSelected ? "dynamic" : "fixed");
            }
        }

        if (dynamicSelected && state.numFramesActuallyPresented > 1) {
            ++slFgDynamicGeneratedStateSamples;
            unsigned int segmentExpected = 0;
            if (slFgGenerationConfirmedCurrentSegment.compare_exchange_strong(segmentExpected, 1)) {
                const unsigned long long confirmedCount = ++slFgGeneratedSegments;
                Log("FG_DYNAMIC_SEGMENT_CONFIRMED present=%llu segment=%llu confirmed_segments=%llu frames_presented=%u observed_multiplier=%ux status=0x%X dynamic_supported=%u target_policy=%s target_fps=%.3f detected_refresh_fps=%.3f input=%ux%u color=%ux%u hdr10_bridge=%u",
                    present, slFgEnableTransitions.load(), confirmedCount, state.numFramesActuallyPresented, state.numFramesActuallyPresented,
                    unsigned(state.status), unsigned(IsFGDynamicMFGSupported()), GetFGDynamicTargetPolicyName(),
                    static_cast<double>(GetFGDynamicResolvedTargetFrameRate()), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                    slFgMvecDepthWidth.load(), slFgMvecDepthHeight.load(), slFgColorWidth.load(), slFgColorHeight.load(), unsigned(IsHdr10BridgeActive()));
            }
            unsigned int dynamicExpected = 0;
            if (slFirstDynamicFrameLogged.compare_exchange_strong(dynamicExpected, 1)) {
                firstDynamicNow = true;
                Log("FG_FIRST_DYNAMIC_MFG_FRAME present=%llu frames_presented=%u observed_multiplier=%ux status=0x%X max_generated=%u max_multiplier=%ux dynamic_supported=%u target_policy=%s target_fps=%.3f detected_refresh_fps=%.3f hdr10_bridge=%u",
                    present, state.numFramesActuallyPresented, state.numFramesActuallyPresented, unsigned(state.status), state.numFramesToGenerateMax,
                    state.numFramesToGenerateMax ? state.numFramesToGenerateMax + 1 : 0, unsigned(IsFGDynamicMFGSupported()),
                    GetFGDynamicTargetPolicyName(), static_cast<double>(GetFGDynamicResolvedTargetFrameRate()),
                    static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0, unsigned(IsHdr10BridgeActive()));
            }
        }

        if (!dynamicSelected && selected >= 2 && state.numFramesActuallyPresented >= selected) {
            ++slFgTargetMultiplierStateSamples;
            unsigned int segmentExpected = 0;
            if (slFgGenerationConfirmedCurrentSegment.compare_exchange_strong(segmentExpected, 1)) {
                const unsigned long long confirmedCount = ++slFgGeneratedSegments;
                const unsigned long long enableSegment = slFgEnableTransitions.load();
                Log("FG_GENERATION_SEGMENT_CONFIRMED present=%llu segment=%llu confirmed_segments=%llu frames_presented=%u requested_generated=%u target_multiplier=%ux status=0x%X input=%ux%u color=%ux%u enable_transitions=%llu resolution_transitions=%llu",
                    present, enableSegment, confirmedCount, state.numFramesActuallyPresented, requestedGenerated, selected,
                    unsigned(state.status), slFgMvecDepthWidth.load(), slFgMvecDepthHeight.load(), slFgColorWidth.load(), slFgColorHeight.load(),
                    slFgEnableTransitions.load(), slFgResolutionTransitions.load());
            }
            unsigned int expected = 0;
            if (slFirstTargetMultiplierFrameLogged.compare_exchange_strong(expected, 1)) {
                firstTargetMultiplierNow = true;
                Log("FG_FIRST_TARGET_MFG_FRAME present=%llu frames_presented=%u requested_generated=%u target_multiplier=%ux status=0x%X max_generated=%u hdr10_bridge=%u",
                    present, state.numFramesActuallyPresented, requestedGenerated, selected, unsigned(state.status),
                    state.numFramesToGenerateMax, unsigned(IsHdr10BridgeActive()));
            }
            if (selected == 4) {
                unsigned int expected4 = 0;
                if (slFirst4xFrameLogged.compare_exchange_strong(expected4, 1)) {
                    firstTargetMultiplierNow = true;
                    Log("FG_FIRST_4X_MFG_FRAME present=%llu frames_presented=%u requested_generated=3 status=0x%X max_generated=%u hdr10_bridge=%u",
                        present, state.numFramesActuallyPresented, unsigned(state.status), state.numFramesToGenerateMax, unsigned(IsHdr10BridgeActive()));
                }
            } else if (selected == 5) {
                unsigned int expected5 = 0;
                if (slFirst5xFrameLogged.compare_exchange_strong(expected5, 1)) {
                    firstTargetMultiplierNow = true;
                    Log("FG_FIRST_5X_MFG_FRAME present=%llu frames_presented=%u requested_generated=4 status=0x%X max_generated=%u hdr10_bridge=%u",
                        present, state.numFramesActuallyPresented, unsigned(state.status), state.numFramesToGenerateMax, unsigned(IsHdr10BridgeActive()));
                }
            } else if (selected == 6) {
                unsigned int expected6 = 0;
                if (slFirst6xFrameLogged.compare_exchange_strong(expected6, 1)) {
                    firstTargetMultiplierNow = true;
                    Log("FG_FIRST_6X_MFG_FRAME present=%llu frames_presented=%u requested_generated=5 status=0x%X max_generated=%u hdr10_bridge=%u",
                        present, state.numFramesActuallyPresented, unsigned(state.status), state.numFramesToGenerateMax, unsigned(IsHdr10BridgeActive()));
                }
            }
        }
    }
    Log("SL_DLSSG_STATE_SAMPLE present=%llu result=%lld status=0x%X frames_presented=%u selected_selection=%s selected_code=%u requested_generated=%u target_multiplier=%ux dynamic_target_policy=%s dynamic_target_fps=%.3f detected_refresh_fps=%.3f max_generated=%u max_multiplier=%ux dynamic_supported=%u min_dimension=%u constants_successes=%llu tag_successes=%llu tag_failures=%llu frame_tag_mask=0x%X simulation_marker_starts=%llu simulation_marker_failures=%llu marker_starts=%llu marker_ends=%llu fg_api_enabled=%u backbuffer_index_successes=%llu backbuffer_index_failures=%llu last_backbuffer_index=%u generated_segments=%llu target_multiplier_state_samples=%llu dynamic_generated_state_samples=%llu max_frames_presented_observed=%u",
        present, SLResultCode(result), unsigned(state.status), state.numFramesActuallyPresented, GetFGSelectionName(selected), selected,
        requestedGenerated, dynamicSelected ? 0u : selected, dynamicSelected ? GetFGDynamicTargetPolicyName() : "n/a",
        dynamicSelected ? static_cast<double>(GetFGDynamicResolvedTargetFrameRate()) : 0.0,
        static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0, state.numFramesToGenerateMax,
        state.numFramesToGenerateMax ? state.numFramesToGenerateMax + 1 : 0, unsigned(IsFGDynamicMFGSupported()), state.minWidthOrHeight,
        slConstantsSuccesses.load(), slResourceTagSuccesses.load(), slResourceTagFailures.load(), GetSLFrameTagMask(present),
        slSimulationMarkerStarts.load(), slSimulationMarkerFailures.load(), slPresentMarkerStarts.load(), slPresentMarkerEnds.load(), slFgEnabledByApi.load(), slBackBufferIndexSuccesses.load(),
        slBackBufferIndexFailures.load(), slLastBackBufferIndex.load(), slFgGeneratedSegments.load(), slFgTargetMultiplierStateSamples.load(),
        slFgDynamicGeneratedStateSamples.load(), slFgMaxFramesPresentedObserved.load());
    if (firstGeneratedNow || firstTargetMultiplierNow || firstDynamicNow || (present % 240) == 0) {
        Log("FG_RUNTIME_STATS present=%llu selected_selection=%s selected_code=%u requested_generated=%u target_multiplier=%ux dynamic_target_policy=%s dynamic_target_fps=%.3f manual_target_fps=%u detected_refresh_fps=%.3f fg_ready_presents=%llu fg_enabled_presents=%llu generated_state_samples=%llu target_multiplier_state_samples=%llu dynamic_generated_state_samples=%llu last_frames_presented=%u max_frames_presented_observed=%u mfg_capability_known=%u mfg_supported=%u mfg_max_generated=%u mfg_max_multiplier=%ux mfg_capability_checks=%llu dynamic_capability_known=%u dynamic_supported=%u dynamic_capability_checks=%llu dynamic_target_changes=%llu display_refresh_changes=%llu backbuffer_index_attempts=%llu backbuffer_index_successes=%llu backbuffer_index_failures=%llu enable_transitions=%llu disable_transitions=%llu selection_changes=%llu generated_segments=%llu resolution_transitions=%llu simulation_marker_attempts=%llu simulation_marker_starts=%llu simulation_marker_failures=%llu suppressed_info_callbacks=%llu",
            present, GetFGSelectionName(selected), selected, requestedGenerated, dynamicSelected ? 0u : selected,
            dynamicSelected ? GetFGDynamicTargetPolicyName() : "n/a",
            dynamicSelected ? static_cast<double>(GetFGDynamicResolvedTargetFrameRate()) : 0.0, GetFGDynamicManualTargetFps(),
            static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
            slFgFrameReadyPresents.load(), slFgEnabledPresents.load(), slFgGeneratedStateSamples.load(), slFgTargetMultiplierStateSamples.load(),
            slFgDynamicGeneratedStateSamples.load(), slFgLastFramesPresented.load(), slFgMaxFramesPresentedObserved.load(), slMfgCapabilityKnown.load(),
            slMfgCapabilitySupported.load(), slMfgMaxGenerated.load(), GetFGMaxSupportedMultiplier(), slMfgCapabilityChecks.load(),
            slDynamicMfgCapabilityKnown.load(), slDynamicMfgSupported.load(), slDynamicMfgCapabilityChecks.load(),
            slFgDynamicTargetChanges.load(), slFgDisplayRefreshChanges.load(), slBackBufferIndexAttempts.load(),
            slBackBufferIndexSuccesses.load(), slBackBufferIndexFailures.load(), slFgEnableTransitions.load(), slFgDisableTransitions.load(),
            slFgMultiplierChanges.load(), slFgGeneratedSegments.load(), slFgResolutionTransitions.load(),
            slSimulationMarkerAttempts.load(), slSimulationMarkerStarts.load(), slSimulationMarkerFailures.load(), slSuppressedInfoCallbacks.load());
    }
}

static void ProbeSLPresentState(unsigned long long present) noexcept {
    if (!verifiedD3d) return;
    const bool sample = present <= 4 || (present % 240) == 0 || (slFgEnabledByApi.load() && slPresentStateSamples.load() < 64);
    if (!sample) return;
    ++slPresentStateSamples;

    IDXGISwapChain* chain = nullptr;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto engineDevice = *reinterpret_cast<unsigned char**>(base + kNativeDevicePointerRva);
        chain = engineDevice ? *reinterpret_cast<IDXGISwapChain**>(engineDevice + kSwapChainOffset) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("SL_PRESENT_CHAIN_EXCEPTION present=%llu exception=0x%08lX isolation=manual_hook_gate", present, GetExceptionCode());
        return;
    }
    if (!chain) {
        Log("SL_PRESENT_CHAIN_UNAVAILABLE present=%llu isolation=manual_hook_gate bootstrap_state=%u", present, slBootstrapState.load());
        return;
    }
    SampleDLSSGState(present);
    DXGI_SWAP_CHAIN_DESC desc{};
    const HRESULT descHr = chain->GetDesc(&desc);
    void* nativeChain = nullptr;
    sl::Result unwrapResult = sl::Result::eErrorInvalidParameter;
    if (slGetNativeInterfaceApi) unwrapResult = slGetNativeInterfaceApi(chain, &nativeChain);
    const bool proxyChain = unwrapResult == sl::Result::eOk && nativeChain && nativeChain != chain;
    Log("SL_PRESENT_MANUAL_GATE present=%llu chain=%p desc_hr=0x%08lX width=%u height=%u format=%u buffers=%u unwrap_result=%lld native_chain=%p proxy_chain=%u bootstrap_state=%u features_requested=3 device_set=%u control_dlss_ready=%u bind_attempted=%u factory_upgrade_count=%u queue_route_calls=%u fg_api_enabled=%u",
        present, chain, static_cast<unsigned long>(descHr),
        SUCCEEDED(descHr) ? desc.BufferDesc.Width : 0u, SUCCEEDED(descHr) ? desc.BufferDesc.Height : 0u,
        SUCCEEDED(descHr) ? unsigned(desc.BufferDesc.Format) : 0u, SUCCEEDED(descHr) ? desc.BufferCount : 0u,
        SLResultCode(unwrapResult), nativeChain, unsigned(proxyChain), slBootstrapState.load(), slDeviceConfigured.load(),
        slControlDlssReady.load(), slDeviceBindAttempted.load(), slFactoryUpgradeSuccesses.load(), slQueueRouteCalls.load(), slFgEnabledByApi.load());
    if (nativeChain) reinterpret_cast<IUnknown*>(nativeChain)->Release();
}

