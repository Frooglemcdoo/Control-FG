#pragma once
// v0.8.20 makes the confirmed HDR10 presentation bridge transition-aware.
// Control can create its presentation swap chain as RGB10 SDR and later switch the SAME
// swap chain to FP16/scRGB through ResizeBuffers. The wrapper therefore exists from SDR
// startup in dormant passthrough mode and activates only when FP16/scRGB is requested.
// While active, the real Streamline-facing swap chain stays RGB10/PQ/BT.2020 and Control
// renders into private FP16 shadow backbuffers returned by this wrapper's GetBuffer().

#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <new>
#include <vector>

static std::atomic<unsigned int> hdr10BridgeActive{0};
static std::atomic<unsigned int> hdr10BridgeWidth{0};
static std::atomic<unsigned int> hdr10BridgeHeight{0};
static std::atomic<unsigned long long> hdr10BridgeFactoryWraps{0};
static std::atomic<unsigned long long> hdr10BridgeSwapchains{0};
static std::atomic<unsigned long long> hdr10BridgeConversions{0};
static std::atomic<unsigned long long> hdr10BridgeConversionFailures{0};
static std::atomic<unsigned long long> hdr10BridgeColorSpaceForces{0};
static std::atomic<unsigned long long> hdr10BridgeResizeCount{0};
static std::atomic<unsigned long long> hdr10BridgeDormantSwapchains{0};
static std::atomic<unsigned long long> hdr10BridgeActivationCount{0};
static std::atomic<unsigned long long> hdr10BridgeDeactivationCount{0};
static std::atomic<unsigned long long> hdr10BridgeActivationFailures{0};
static std::atomic<unsigned long long> hdr10BridgeTransitionGeneration{0};
static std::atomic<unsigned long long> fgDynamicVsyncBypassPresents{0};
static std::atomic<unsigned int> fgDynamicVsyncLastRequested{0xFFFFFFFFu};

static bool ShouldBypassVSyncForDynamicMFG() noexcept {
    return IsFGDynamicSelection(GetFGUserMultiplier()) &&
        slFgEnabledByApi.load(std::memory_order_acquire) != 0 &&
        slFgAppliedMode.load(std::memory_order_acquire) == 2u;
}

static UINT ApplyDynamicMFGPresentSyncInterval(UINT requested, const char* api) noexcept {
    if (!ShouldBypassVSyncForDynamicMFG() || requested == 0) return requested;
    const unsigned long long ordinal = ++fgDynamicVsyncBypassPresents;
    const unsigned int previous = fgDynamicVsyncLastRequested.exchange(requested, std::memory_order_acq_rel);
    if (ordinal <= 8 || previous != requested || (ordinal % 240ull) == 0) {
        Log("FG_DYNAMIC_VSYNC_BYPASS api=%s requested_sync=%u applied_sync=0 bypass_presents=%llu target_policy=%s target_fps=%.3f",
            api ? api : "unknown", requested, ordinal, GetFGDynamicTargetPolicyName(),
            static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
    }
    return 0;
}

static bool IsHdr10BridgeActive() noexcept {
    return hdr10BridgeActive.load(std::memory_order_acquire) != 0;
}
static unsigned int GetHdr10BridgeWidth() noexcept {
    return hdr10BridgeWidth.load(std::memory_order_acquire);
}
static unsigned int GetHdr10BridgeHeight() noexcept {
    return hdr10BridgeHeight.load(std::memory_order_acquire);
}
static unsigned long long GetHdr10BridgeTransitionGeneration() noexcept {
    return hdr10BridgeTransitionGeneration.load(std::memory_order_acquire);
}

static bool Hdr10BridgeDisabledByEnvironment() noexcept {
    wchar_t value[16]{};
    const DWORD n = GetEnvironmentVariableW(L"CONTROL_FG_DISABLE_HDR10_BRIDGE", value, _countof(value));
    if (!n || n >= _countof(value)) return false;
    return _wcsicmp(value, L"1") == 0 || _wcsicmp(value, L"true") == 0 || _wcsicmp(value, L"yes") == 0;
}

static bool ShouldAttemptHdr10Bridge(DXGI_FORMAT format) noexcept {
    return format == DXGI_FORMAT_R16G16B16A16_FLOAT && !Hdr10BridgeDisabledByEnvironment();
}

static bool ShouldWatchHdr10BridgeTransition(DXGI_FORMAT format) noexcept {
    if (Hdr10BridgeDisabledByEnvironment()) return false;
    // Control's observed SDR presentation format is RGB10 (24). Watching only the known
    // SDR/HDR pair keeps every unrelated swap-chain format completely untouched.
    return format == DXGI_FORMAT_R10G10B10A2_UNORM ||
           format == DXGI_FORMAT_R16G16B16A16_FLOAT;
}

static ID3D12CommandQueue* GetHdr10BridgeDirectQueue() noexcept {
    return slHdr10DirectQueue.load(std::memory_order_acquire);
}

using D3DCompileDynamicFn = HRESULT (WINAPI*)(LPCVOID, SIZE_T, LPCSTR,
    const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT,
    ID3DBlob**, ID3DBlob**);
using D3D12SerializeRootSignatureDynamicFn = HRESULT (WINAPI*)(
    const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION,
    ID3DBlob**, ID3DBlob**);

static constexpr char kHdr10BridgeShader[] = R"HLSL(
Texture2D<float4> Src : register(t0);

struct VSOut { float4 pos : SV_Position; };

VSOut VSMain(uint id : SV_VertexID)
{
    VSOut o;
    float2 p = (id == 0) ? float2(-1.0, -1.0) :
               (id == 1) ? float2(-1.0,  3.0) : float2(3.0, -1.0);
    o.pos = float4(p, 0.0, 1.0);
    return o;
}

float3 Linear709ToLinear2020(float3 c)
{
    return float3(
        0.6274040 * c.r + 0.3292820 * c.g + 0.0433136 * c.b,
        0.0690970 * c.r + 0.9195400 * c.g + 0.0113612 * c.b,
        0.0163916 * c.r + 0.0880132 * c.g + 0.8955950 * c.b);
}

float3 LinearNitsToPQ(float3 nits)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    float3 L = saturate(max(nits, 0.0) / 10000.0);
    float3 Lm = pow(L, m1);
    return pow((c1 + c2 * Lm) / (1.0 + c3 * Lm), m2);
}

float4 PSMain(VSOut i) : SV_Target
{
    int2 p = int2(i.pos.xy);
    float4 src = Src.Load(int3(p, 0));
    float3 linear2020 = max(Linear709ToLinear2020(src.rgb), 0.0);
    // Windows scRGB defines 1.0 as 80 cd/m^2 (nits).
    float3 pq = LinearNitsToPQ(linear2020 * 80.0);
    return float4(pq, 1.0);
}
)HLSL";

#include "fg_present_capture.h"
#include "fg_sdr_correction.h"

class Hdr10SwapChainProxy final : public IDXGISwapChain4 {
public:
    Hdr10SwapChainProxy(IDXGISwapChain4* inner, DXGI_FORMAT gameFormat, DXGI_USAGE gameUsage) noexcept
        : inner_(inner), gameFormat_(gameFormat), gameUsage_(gameUsage) {}

    bool Initialize(bool activateNow) noexcept {
        if (!inner_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=swapchain_missing inner=%p", inner_);
            return false;
        }
        if (!AcquireDevice()) return false;
        if (!activateNow) {
            const auto dormant = ++hdr10BridgeDormantSwapchains;
            Log("HDR10_BRIDGE_DORMANT wrapper=%p inner=%p game_format=%u mode=passthrough transition_watch=ResizeBuffers dormant_swapchains=%llu",
                this, inner_, unsigned(gameFormat_), dormant);
            return true;
        }
        if (!ActivateAfterInnerResize("create")) return false;
        return true;
    }

    virtual ~Hdr10SwapChainProxy() {
        sdrCorrection_.Shutdown();
        slFgOffPresentProof.Invalidate();
        if (active_) ClearActiveState("destroy");
        WaitForConversions();
        ReleaseResources();
        if (commandList_) commandList_->Release();
        if (fence_) fence_->Release();
        if (pso_) pso_->Release();
        if (rootSignature_) rootSignature_->Release();
        if (device_) device_->Release();
        if (fenceEvent_) CloseHandle(fenceEvent_);
        if (inner_) inner_->Release();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIObject) ||
            riid == __uuidof(IDXGIDeviceSubObject) || riid == __uuidof(IDXGISwapChain) ||
            riid == __uuidof(IDXGISwapChain1) || riid == __uuidof(IDXGISwapChain2) ||
            riid == __uuidof(IDXGISwapChain3) || riid == __uuidof(IDXGISwapChain4)) {
            *ppvObject = static_cast<IDXGISwapChain4*>(this);
            AddRef();
            return S_OK;
        }
        const HRESULT hr = inner_ ? inner_->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
        if (SUCCEEDED(hr)) Log("HDR10_BRIDGE_QI_DELEGATED wrapper=%p iid_unknown=1 object=%p", this, *ppvObject);
        return hr;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = --refs_;
        if (!value) delete this;
        return value;
    }

    // IDXGIObject
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override {
        return inner_->SetPrivateData(Name, DataSize, pData);
    }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override {
        return inner_->SetPrivateDataInterface(Name, pUnknown);
    }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override {
        return inner_->GetPrivateData(Name, pDataSize, pData);
    }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override {
        return inner_->GetParent(riid, ppParent);
    }

    // IDXGIDeviceSubObject
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) override {
        return inner_->GetDevice(riid, ppDevice);
    }

    // IDXGISwapChain
    HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override {
        const bool realPresent = (Flags & DXGI_PRESENT_TEST) == 0;
        const unsigned long long present = presentCount.load(std::memory_order_acquire);
        if (realPresent) {
            const HRESULT uiWork=SubmitFGUIRecompositionBeforePresent(present);
            if(FAILED(uiWork))return uiWork;
            ObserveSLDisplayHdrDomainBeforePresent(inner_, present);
        }
        if (active_ && realPresent) {
            const HRESULT convert = ConvertCurrentBackBuffer();
            if (FAILED(convert)) {
                ++hdr10BridgeConversionFailures;
                Log("HDR10_BRIDGE_CONVERT_FAIL stage=present hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
                Log("HDR10_BRIDGE_PRESENT_FAIL stage=convert hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
            }
        }
        if (!active_ && realPresent) {
            const HRESULT correction=sdrCorrection_.Apply(this,inner_,present);
            if(FAILED(correction)){Log("FG_SDR_PRESENT_FAIL frame=%llu hr=0x%08lX action=stop_failed_copy",present,static_cast<unsigned long>(correction));return correction;}
            FGPixelSDRPresent(inner_, present);
        }
        const UINT appliedSyncInterval = !realPresent
            ? SyncInterval
            : ApplyDynamicMFGPresentSyncInterval(SyncInterval, "Present");
        const auto alignSerial=realPresent?FGAlignPresentEnter(present,inner_,"Present",Flags):0;
        const HRESULT hr = inner_->Present(appliedSyncInterval, Flags);
        if(realPresent) FGAlignPresentExit(alignSerial,present,inner_,hr);
        if (realPresent) RecordSLOffPresentBoundary(inner_, present, hr);
        if (realPresent) CompleteSLDLSSGHardResetAfterPresent(present, hr, "present");
        return hr;
    }
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) override {
        if (!ppSurface) return E_POINTER;
        *ppSurface = nullptr;
        if (!active_) return inner_->GetBuffer(Buffer, riid, ppSurface);
        if (Buffer >= shadows_.size() || !shadows_[Buffer]) return DXGI_ERROR_INVALID_CALL;
        return shadows_[Buffer]->QueryInterface(riid, ppSurface);
    }
    HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) override {
        const HRESULT hr = inner_->SetFullscreenState(Fullscreen, pTarget);
        if (SUCCEEDED(hr) && active_) {
            ForceHdr10ColorSpace("fullscreen");
            ApplyHdr10Metadata();
        }
        return hr;
    }
    HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) override {
        return inner_->GetFullscreenState(pFullscreen, ppTarget);
    }
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) override {
        const HRESULT hr = inner_->GetDesc(pDesc);
        if (SUCCEEDED(hr) && pDesc) {
            pDesc->BufferDesc.Format = gameFormat_;
            pDesc->BufferUsage = gameUsage_;
        }
        return hr;
    }
    HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height,
        DXGI_FORMAT NewFormat, UINT SwapChainFlags) override {
        const HRESULT sdrDrain=sdrCorrection_.PrepareResize(presentCount.load(),"ResizeBuffers");
        if(FAILED(sdrDrain))return sdrDrain;
        const DXGI_FORMAT requested = NewFormat == DXGI_FORMAT_UNKNOWN ? gameFormat_ : NewFormat;
        const bool domainTransition = (!active_ && requested == DXGI_FORMAT_R16G16B16A16_FLOAT) ||
                                      (active_ && requested != DXGI_FORMAT_R16G16B16A16_FLOAT);
        if (domainTransition) {
            if (!QuiesceDLSSGForHdrSwapchainTransition(inner_, active_ ? "hdr_to_sdr_resize" : "sdr_to_hdr_resize")) {
                Log("HDR10_BRIDGE_TRANSITION_WARN stage=ResizeBuffers reason=fg_hard_reset_failed action=continue_resize_fail_closed_fg_off");
            }
        }
        if (!active_ && requested != DXGI_FORMAT_R16G16B16A16_FLOAT) {
            const HRESULT hr = inner_->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
            if (SUCCEEDED(hr)) {
                gameFormat_ = requested;
                Log("HDR10_BRIDGE_DORMANT_RESIZE mode=passthrough count=%u width=%u height=%u format=%u hr=0x%08lX",
                    BufferCount, Width, Height, unsigned(requested), static_cast<unsigned long>(hr));
            }
            return hr;
        }

        if (requested == DXGI_FORMAT_R16G16B16A16_FLOAT) {
            const bool wasActive = active_;
            WaitForConversions();
            ReleaseResources();
            const HRESULT hr = inner_->ResizeBuffers(BufferCount, Width, Height,
                DXGI_FORMAT_R10G10B10A2_UNORM, SwapChainFlags);
            if (FAILED(hr)) {
                Log("HDR10_BRIDGE_RESIZE_FAIL stage=%s hr=0x%08lX count=%u width=%u height=%u",
                    wasActive ? "active_resize" : "activate_resize", static_cast<unsigned long>(hr), BufferCount, Width, Height);
                return hr;
            }
            gameFormat_ = requested;
            if (!ActivateAfterInnerResize(wasActive ? "resize" : "resize_activate")) {
                ++hdr10BridgeActivationFailures;
                Log("HDR10_BRIDGE_ACTIVATE_FAIL stage=ResizeBuffers rollback=attempt_passthrough_fp16 failures=%llu",
                    hdr10BridgeActivationFailures.load());
                ReleaseResources();
                const HRESULT rollback = inner_->ResizeBuffers(BufferCount, Width, Height, requested, SwapChainFlags);
                if (SUCCEEDED(rollback)) {
                    active_ = false;
                    ClearActiveState("activate_rollback");
                    Log("HDR10_BRIDGE_ACTIVATE_ROLLBACK_OK stage=ResizeBuffers format=%u", unsigned(requested));
                    return S_OK;
                }
                Log("HDR10_BRIDGE_ACTIVATE_ROLLBACK_FAIL stage=ResizeBuffers hr=0x%08lX", static_cast<unsigned long>(rollback));
                return E_FAIL;
            }
            ++hdr10BridgeResizeCount;
            Log("HDR10_BRIDGE_RESIZE_OK stage=%s count=%u width=%u height=%u game_format=%u real_format=%u resizes=%llu",
                wasActive ? "active" : "activate", bufferCount_, width_, height_, unsigned(gameFormat_),
                unsigned(DXGI_FORMAT_R10G10B10A2_UNORM), hdr10BridgeResizeCount.load());
            return S_OK;
        }

        // Active HDR -> non-FP16 transition. Tear down the shadow path and let the original
        // swap chain format flow through unchanged again.
        WaitForConversions();
        ReleaseResources();
        const HRESULT hr = inner_->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
        if (FAILED(hr)) {
            Log("HDR10_BRIDGE_DEACTIVATE_FAIL stage=ResizeBuffers hr=0x%08lX requested=%u",
                static_cast<unsigned long>(hr), unsigned(requested));
            return hr;
        }
        active_ = false;
        gameFormat_ = requested;
        ClearActiveState("resize_deactivate");
        RestorePassthroughColorSpace("resize_deactivate");
        const auto deactivations = ++hdr10BridgeDeactivationCount;
        Log("HDR10_BRIDGE_DEACTIVATED stage=ResizeBuffers wrapper=%p game_format=%u mode=passthrough deactivations=%llu",
            this, unsigned(gameFormat_), deactivations);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) override {
        return inner_->ResizeTarget(pNewTargetParameters);
    }
    HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput) override {
        return inner_->GetContainingOutput(ppOutput);
    }
    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override {
        return inner_->GetFrameStatistics(pStats);
    }
    HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount) override {
        return inner_->GetLastPresentCount(pLastPresentCount);
    }

    // IDXGISwapChain1
    HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc) override {
        const HRESULT hr = inner_->GetDesc1(pDesc);
        if (SUCCEEDED(hr) && pDesc) {
            pDesc->Format = gameFormat_;
            pDesc->BufferUsage = gameUsage_;
        }
        return hr;
    }
    HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) override {
        return inner_->GetFullscreenDesc(pDesc);
    }
    HRESULT STDMETHODCALLTYPE GetHwnd(HWND* pHwnd) override { return inner_->GetHwnd(pHwnd); }
    HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void** ppUnk) override {
        return inner_->GetCoreWindow(refiid, ppUnk);
    }
    HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags,
        const DXGI_PRESENT_PARAMETERS* pPresentParameters) override {
        const bool realPresent = (PresentFlags & DXGI_PRESENT_TEST) == 0;
        const unsigned long long present = presentCount.load(std::memory_order_acquire);
        if (realPresent) {
            const HRESULT uiWork=SubmitFGUIRecompositionBeforePresent(present);
            if(FAILED(uiWork))return uiWork;
            ObserveSLDisplayHdrDomainBeforePresent(inner_, present);
        }
        if (active_ && realPresent) {
            const HRESULT convert = ConvertCurrentBackBuffer();
            if (FAILED(convert)) {
                ++hdr10BridgeConversionFailures;
                Log("HDR10_BRIDGE_CONVERT_FAIL stage=present1 hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
                Log("HDR10_BRIDGE_PRESENT_FAIL stage=convert1 hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
            }
        }
        if (!active_ && realPresent) {
            const HRESULT correction=sdrCorrection_.Apply(this,inner_,present);
            if(FAILED(correction)){Log("FG_SDR_PRESENT_FAIL frame=%llu hr=0x%08lX action=stop_failed_copy",present,static_cast<unsigned long>(correction));return correction;}
            FGPixelSDRPresent(inner_, present);
        }
        const UINT appliedSyncInterval = !realPresent
            ? SyncInterval
            : ApplyDynamicMFGPresentSyncInterval(SyncInterval, "Present1");
        const auto alignSerial=realPresent?FGAlignPresentEnter(present,inner_,"Present1",PresentFlags):0;
        const HRESULT hr = inner_->Present1(appliedSyncInterval, PresentFlags, pPresentParameters);
        if(realPresent) FGAlignPresentExit(alignSerial,present,inner_,hr);
        if (realPresent) RecordSLOffPresentBoundary(inner_, present, hr);
        if (realPresent) CompleteSLDLSSGHardResetAfterPresent(present, hr, "present");
        return hr;
    }
    BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override { return inner_->IsTemporaryMonoSupported(); }
    HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput) override {
        return inner_->GetRestrictToOutput(ppRestrictToOutput);
    }
    HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA* pColor) override {
        return inner_->SetBackgroundColor(pColor);
    }
    HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA* pColor) override {
        return inner_->GetBackgroundColor(pColor);
    }
    HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override { return inner_->SetRotation(Rotation); }
    HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION* pRotation) override { return inner_->GetRotation(pRotation); }

    // IDXGISwapChain2
    HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override { return inner_->SetSourceSize(Width, Height); }
    HRESULT STDMETHODCALLTYPE GetSourceSize(UINT* pWidth, UINT* pHeight) override { return inner_->GetSourceSize(pWidth, pHeight); }
    HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override { return inner_->SetMaximumFrameLatency(MaxLatency); }
    HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT* pMaxLatency) override { return inner_->GetMaximumFrameLatency(pMaxLatency); }
    HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override { return inner_->GetFrameLatencyWaitableObject(); }
    HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix) override { return inner_->SetMatrixTransform(pMatrix); }
    HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F* pMatrix) override { return inner_->GetMatrixTransform(pMatrix); }

    // IDXGISwapChain3
    UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override { return inner_->GetCurrentBackBufferIndex(); }
    HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT* pColorSpaceSupport) override {
        if (!active_) return inner_->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
        if (!pColorSpaceSupport) return E_POINTER;
        if (ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709) {
            *pColorSpaceSupport = DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT;
            return S_OK;
        }
        if (ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 ||
            ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
            *pColorSpaceSupport = 0;
            return S_OK;
        }
        return inner_->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
    }
    HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override {
        if (!active_) {
            const HRESULT hr = inner_->SetColorSpace1(ColorSpace);
            if (SUCCEEDED(hr)) {
                lastPassthroughColorSpace_ = ColorSpace;
                hasPassthroughColorSpace_ = true;
            }
            return hr;
        }
        if (ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709) {
            const bool ok = ForceHdr10ColorSpace("game_scrgb_request");
            Log("HDR10_BRIDGE_COLORSPACE_VIRTUALIZED requested=%u underlying=%u success=%u",
                unsigned(ColorSpace), unsigned(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020), unsigned(ok));
            return ok ? S_OK : DXGI_ERROR_INVALID_CALL;
        }
        return inner_->SetColorSpace1(ColorSpace);
    }
    HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height,
        DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask,
        IUnknown* const* ppPresentQueue) override {
        const HRESULT sdrDrain=sdrCorrection_.PrepareResize(presentCount.load(),"ResizeBuffers1");
        if(FAILED(sdrDrain))return sdrDrain;
        const DXGI_FORMAT requested = Format == DXGI_FORMAT_UNKNOWN ? gameFormat_ : Format;
        const bool domainTransition = (!active_ && requested == DXGI_FORMAT_R16G16B16A16_FLOAT) ||
                                      (active_ && requested != DXGI_FORMAT_R16G16B16A16_FLOAT);
        if (domainTransition) {
            if (!QuiesceDLSSGForHdrSwapchainTransition(inner_, active_ ? "hdr_to_sdr_resize1" : "sdr_to_hdr_resize1")) {
                Log("HDR10_BRIDGE_TRANSITION_WARN stage=ResizeBuffers1 reason=fg_hard_reset_failed action=continue_resize_fail_closed_fg_off");
            }
        }
        if (!active_ && requested != DXGI_FORMAT_R16G16B16A16_FLOAT) {
            const HRESULT hr = inner_->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
            if (SUCCEEDED(hr)) {
                gameFormat_ = requested;
                Log("HDR10_BRIDGE_DORMANT_RESIZE stage=ResizeBuffers1 mode=passthrough count=%u width=%u height=%u format=%u hr=0x%08lX",
                    BufferCount, Width, Height, unsigned(requested), static_cast<unsigned long>(hr));
            }
            return hr;
        }

        if (requested == DXGI_FORMAT_R16G16B16A16_FLOAT) {
            const bool wasActive = active_;
            WaitForConversions();
            ReleaseResources();
            const HRESULT hr = inner_->ResizeBuffers1(BufferCount, Width, Height,
                DXGI_FORMAT_R10G10B10A2_UNORM, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
            if (FAILED(hr)) {
                Log("HDR10_BRIDGE_RESIZE_FAIL stage=%s hr=0x%08lX count=%u width=%u height=%u",
                    wasActive ? "ResizeBuffers1_active" : "ResizeBuffers1_activate", static_cast<unsigned long>(hr), BufferCount, Width, Height);
                return hr;
            }
            gameFormat_ = requested;
            if (!ActivateAfterInnerResize(wasActive ? "resize1" : "resize1_activate")) {
                ++hdr10BridgeActivationFailures;
                Log("HDR10_BRIDGE_ACTIVATE_FAIL stage=ResizeBuffers1 rollback=attempt_passthrough_fp16 failures=%llu",
                    hdr10BridgeActivationFailures.load());
                ReleaseResources();
                const HRESULT rollback = inner_->ResizeBuffers1(BufferCount, Width, Height, requested, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
                if (SUCCEEDED(rollback)) {
                    active_ = false;
                    ClearActiveState("activate1_rollback");
                    Log("HDR10_BRIDGE_ACTIVATE_ROLLBACK_OK stage=ResizeBuffers1 format=%u", unsigned(requested));
                    return S_OK;
                }
                Log("HDR10_BRIDGE_ACTIVATE_ROLLBACK_FAIL stage=ResizeBuffers1 hr=0x%08lX", static_cast<unsigned long>(rollback));
                return E_FAIL;
            }
            ++hdr10BridgeResizeCount;
            Log("HDR10_BRIDGE_RESIZE_OK stage=%s count=%u width=%u height=%u game_format=%u real_format=%u resizes=%llu",
                wasActive ? "ResizeBuffers1_active" : "ResizeBuffers1_activate", bufferCount_, width_, height_, unsigned(gameFormat_),
                unsigned(DXGI_FORMAT_R10G10B10A2_UNORM), hdr10BridgeResizeCount.load());
            return S_OK;
        }

        WaitForConversions();
        ReleaseResources();
        const HRESULT hr = inner_->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
        if (FAILED(hr)) {
            Log("HDR10_BRIDGE_DEACTIVATE_FAIL stage=ResizeBuffers1 hr=0x%08lX requested=%u", static_cast<unsigned long>(hr), unsigned(requested));
            return hr;
        }
        active_ = false;
        gameFormat_ = requested;
        ClearActiveState("resize1_deactivate");
        RestorePassthroughColorSpace("resize1_deactivate");
        const auto deactivations = ++hdr10BridgeDeactivationCount;
        Log("HDR10_BRIDGE_DEACTIVATED stage=ResizeBuffers1 wrapper=%p game_format=%u mode=passthrough deactivations=%llu",
            this, unsigned(gameFormat_), deactivations);
        return S_OK;
    }

    // IDXGISwapChain4
    HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void* pMetaData) override {
        if (!active_) return inner_->SetHDRMetaData(Type, Size, pMetaData);
        Log("HDR10_BRIDGE_METADATA_VIRTUALIZED game_type=%u game_size=%u metadata=%p underlying_type=%u",
            unsigned(Type), Size, pMetaData, unsigned(DXGI_HDR_METADATA_TYPE_HDR10));
        ApplyHdr10Metadata();
        return S_OK;
    }

private:
    bool AcquireDevice() noexcept {
        if (device_) return true;
        ID3D12CommandQueue* queue = GetHdr10BridgeDirectQueue();
        if (!queue) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=queue_missing inner=%p", inner_);
            return false;
        }
        HRESULT hr = queue->GetDevice(IID_PPV_ARGS(&device_));
        if (FAILED(hr) || !device_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=get_device hr=0x%08lX queue=%p", static_cast<unsigned long>(hr), queue);
            return false;
        }
        return true;
    }

    void PublishActiveState(const char* reason) noexcept {
        hdr10BridgeWidth.store(width_, std::memory_order_release);
        hdr10BridgeHeight.store(height_, std::memory_order_release);
        const unsigned int previous = hdr10BridgeActive.exchange(1, std::memory_order_acq_rel);
        unsigned long long transitionGeneration = hdr10BridgeTransitionGeneration.load(std::memory_order_acquire);
        if (!previous) transitionGeneration = hdr10BridgeTransitionGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
        slFgColorWidth.store(width_, std::memory_order_release);
        slFgColorHeight.store(height_, std::memory_order_release);
        slFgColorFormat.store(static_cast<unsigned int>(DXGI_FORMAT_R10G10B10A2_UNORM), std::memory_order_release);
        slFgHudLessFormat.store(0, std::memory_order_release);
        const auto activations = ++hdr10BridgeActivationCount;
        Log("HDR10_BRIDGE_ACTIVE wrapper=%p inner=%p reason=%s width=%u height=%u buffers=%u game_format=%u real_format=%u source_space=scrgb_rec709_linear source_white_nits=80 target_space=hdr10_bt2100 target_transfer=st2084 target_primaries=bt2020 activations=%llu transition_generation=%llu",
            this, inner_, reason ? reason : "unknown", width_, height_, bufferCount_, unsigned(gameFormat_),
            unsigned(DXGI_FORMAT_R10G10B10A2_UNORM), activations, transitionGeneration);
    }

    void ClearActiveState(const char* reason) noexcept {
        const unsigned int previous = hdr10BridgeActive.exchange(0, std::memory_order_acq_rel);
        unsigned long long transitionGeneration = hdr10BridgeTransitionGeneration.load(std::memory_order_acquire);
        if (previous) transitionGeneration = hdr10BridgeTransitionGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
        hdr10BridgeWidth.store(0, std::memory_order_release);
        hdr10BridgeHeight.store(0, std::memory_order_release);
        Log("HDR10_BRIDGE_INACTIVE wrapper=%p reason=%s transition_generation=%llu", this, reason ? reason : "unknown", transitionGeneration);
    }

    void RestorePassthroughColorSpace(const char* reason) noexcept {
        if (!inner_ || !hasPassthroughColorSpace_) return;
        const HRESULT hr = inner_->SetColorSpace1(lastPassthroughColorSpace_);
        Log("HDR10_BRIDGE_COLORSPACE_RESTORE reason=%s requested=%u hr=0x%08lX",
            reason ? reason : "unknown", unsigned(lastPassthroughColorSpace_), static_cast<unsigned long>(hr));
    }

    bool ActivateAfterInnerResize(const char* reason) noexcept {
        if (!AcquireDevice()) return false;
        if (!BuildResources()) return false;
        if (!pso_ && !BuildPipeline()) { ReleaseResources(); return false; }
        if (!ForceHdr10ColorSpace(reason)) { ReleaseResources(); return false; }
        ApplyHdr10Metadata();
        active_ = true;
        ++hdr10BridgeSwapchains;
        PublishActiveState(reason);
        return true;
    }

    bool BuildResources() noexcept {
        if (!inner_ || !device_) return false;
        DXGI_SWAP_CHAIN_DESC1 realDesc{};
        HRESULT hr = inner_->GetDesc1(&realDesc);
        if (FAILED(hr) || realDesc.Format != DXGI_FORMAT_R10G10B10A2_UNORM || !realDesc.Width || !realDesc.Height || !realDesc.BufferCount) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=real_desc hr=0x%08lX width=%u height=%u count=%u format=%u",
                static_cast<unsigned long>(hr), realDesc.Width, realDesc.Height, realDesc.BufferCount, unsigned(realDesc.Format));
            return false;
        }
        width_ = realDesc.Width;
        height_ = realDesc.Height;
        bufferCount_ = realDesc.BufferCount;
        shadows_.assign(bufferCount_, nullptr);
        realBuffers_.assign(bufferCount_, nullptr);
        allocators_.assign(bufferCount_, nullptr);
        allocatorFenceValues_.assign(bufferCount_, 0);

        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap.CreationNodeMask = 1;
        heap.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC shadowDesc{};
        shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        shadowDesc.Alignment = 0;
        shadowDesc.Width = width_;
        shadowDesc.Height = height_;
        shadowDesc.DepthOrArraySize = 1;
        shadowDesc.MipLevels = 1;
        shadowDesc.Format = gameFormat_;
        shadowDesc.SampleDesc.Count = 1;
        shadowDesc.SampleDesc.Quality = 0;
        shadowDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = bufferCount_;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap_));
        if (FAILED(hr) || !srvHeap_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=srv_heap hr=0x%08lX count=%u", static_cast<unsigned long>(hr), bufferCount_);
            ReleaseResources();
            return false;
        }
        srvIncrement_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = bufferCount_;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
        if (FAILED(hr) || !rtvHeap_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=rtv_heap hr=0x%08lX count=%u", static_cast<unsigned long>(hr), bufferCount_);
            ReleaseResources();
            return false;
        }
        rtvIncrement_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (UINT i = 0; i < bufferCount_; ++i) {
            hr = device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &shadowDesc,
                D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&shadows_[i]));
            if (FAILED(hr) || !shadows_[i]) {
                Log("HDR10_BRIDGE_INIT_FAIL stage=shadow_buffer index=%u hr=0x%08lX", i, static_cast<unsigned long>(hr));
                ReleaseResources();
                return false;
            }
            wchar_t name[64]{};
            swprintf_s(name, L"ControlFG HDR FP16 Shadow %u", i);
            shadows_[i]->SetName(name);

            hr = inner_->GetBuffer(i, IID_PPV_ARGS(&realBuffers_[i]));
            if (FAILED(hr) || !realBuffers_[i]) {
                Log("HDR10_BRIDGE_INIT_FAIL stage=real_buffer index=%u hr=0x%08lX", i, static_cast<unsigned long>(hr));
                ReleaseResources();
                return false;
            }
            const D3D12_RESOURCE_DESC realResourceDesc = realBuffers_[i]->GetDesc();
            if (realResourceDesc.Format != DXGI_FORMAT_R10G10B10A2_UNORM) {
                Log("HDR10_BRIDGE_INIT_FAIL stage=real_buffer_format index=%u format=%u", i, unsigned(realResourceDesc.Format));
                ReleaseResources();
                return false;
            }
            if ((realResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0) {
                Log("HDR10_BRIDGE_INIT_FAIL stage=real_buffer_flags index=%u flags=0x%X required=allow_render_target", i, unsigned(realResourceDesc.Flags));
                ReleaseResources();
                return false;
            }

            hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators_[i]));
            if (FAILED(hr) || !allocators_[i]) {
                Log("HDR10_BRIDGE_INIT_FAIL stage=command_allocator index=%u hr=0x%08lX", i, static_cast<unsigned long>(hr));
                ReleaseResources();
                return false;
            }

            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
            srv.Format = gameFormat_;
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv.Texture2D.MostDetailedMip = 0;
            srv.Texture2D.MipLevels = 1;
            srv.Texture2D.PlaneSlice = 0;
            srv.Texture2D.ResourceMinLODClamp = 0.0f;
            D3D12_CPU_DESCRIPTOR_HANDLE srvCpu = srvHeap_->GetCPUDescriptorHandleForHeapStart();
            srvCpu.ptr += SIZE_T(i) * SIZE_T(srvIncrement_);
            device_->CreateShaderResourceView(shadows_[i], &srv, srvCpu);

            D3D12_RENDER_TARGET_VIEW_DESC rtv{};
            rtv.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv.Texture2D.MipSlice = 0;
            rtv.Texture2D.PlaneSlice = 0;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvCpu = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            rtvCpu.ptr += SIZE_T(i) * SIZE_T(rtvIncrement_);
            device_->CreateRenderTargetView(realBuffers_[i], &rtv, rtvCpu);
        }
        Log("HDR10_BRIDGE_RESOURCES width=%u height=%u buffers=%u shadow_format=%u real_format=%u per_buffer_srv=1 per_buffer_allocator=1",
            width_, height_, bufferCount_, unsigned(gameFormat_), unsigned(DXGI_FORMAT_R10G10B10A2_UNORM));
        return true;
    }

    bool BuildPipeline() noexcept {
        if (!device_ || !bufferCount_ || allocators_.empty() || !allocators_[0]) return false;
        HMODULE compiler = LoadLibraryW(L"d3dcompiler_47.dll");
        HMODULE d3d12 = GetModuleHandleW(L"d3d12.dll");
        bool releaseD3D12 = false;
        if (!d3d12) { d3d12 = LoadLibraryW(L"d3d12.dll"); releaseD3D12 = d3d12 != nullptr; }
        auto compile = compiler ? reinterpret_cast<D3DCompileDynamicFn>(GetProcAddress(compiler, "D3DCompile")) : nullptr;
        auto serialize = d3d12 ? reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(d3d12, "D3D12SerializeRootSignature")) : nullptr;
        if (!compile || !serialize) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=dynamic_shader_functions compiler=%p d3d12=%p compile=%p serialize=%p", compiler, d3d12, compile, serialize);
            if (compiler) FreeLibrary(compiler);
            if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
            return false;
        }

        ID3DBlob* vs = nullptr;
        ID3DBlob* ps = nullptr;
        ID3DBlob* errors = nullptr;
        HRESULT hr = compile(kHdr10BridgeShader, sizeof(kHdr10BridgeShader) - 1, "ControlFG_HDR10_Bridge",
            nullptr, nullptr, "VSMain", "vs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs, &errors);
        if (FAILED(hr) || !vs) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=compile_vs hr=0x%08lX error=%s", static_cast<unsigned long>(hr), errors ? static_cast<const char*>(errors->GetBufferPointer()) : "none");
            if (errors) errors->Release();
            if (compiler) FreeLibrary(compiler);
            if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
            return false;
        }
        if (errors) { errors->Release(); errors = nullptr; }
        hr = compile(kHdr10BridgeShader, sizeof(kHdr10BridgeShader) - 1, "ControlFG_HDR10_Bridge",
            nullptr, nullptr, "PSMain", "ps_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps, &errors);
        if (FAILED(hr) || !ps) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=compile_ps hr=0x%08lX error=%s", static_cast<unsigned long>(hr), errors ? static_cast<const char*>(errors->GetBufferPointer()) : "none");
            if (errors) errors->Release();
            vs->Release();
            if (compiler) FreeLibrary(compiler);
            if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
            return false;
        }
        if (errors) { errors->Release(); errors = nullptr; }

        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        D3D12_ROOT_PARAMETER parameter{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable.NumDescriptorRanges = 1;
        parameter.DescriptorTable.pDescriptorRanges = &range;
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC rootDesc{};
        rootDesc.NumParameters = 1;
        rootDesc.pParameters = &parameter;
        rootDesc.NumStaticSamplers = 0;
        rootDesc.pStaticSamplers = nullptr;
        rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ID3DBlob* rootBlob = nullptr;
        hr = serialize(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errors);
        if (FAILED(hr) || !rootBlob) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=serialize_root hr=0x%08lX error=%s", static_cast<unsigned long>(hr), errors ? static_cast<const char*>(errors->GetBufferPointer()) : "none");
            if (errors) errors->Release();
            vs->Release(); ps->Release();
            if (compiler) FreeLibrary(compiler);
            if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
            return false;
        }
        if (errors) { errors->Release(); errors = nullptr; }
        hr = device_->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        rootBlob->Release();
        if (FAILED(hr) || !rootSignature_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=create_root hr=0x%08lX", static_cast<unsigned long>(hr));
            vs->Release(); ps->Release();
            if (compiler) FreeLibrary(compiler);
            if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC state{};
        state.pRootSignature = rootSignature_;
        state.VS.pShaderBytecode = vs->GetBufferPointer();
        state.VS.BytecodeLength = vs->GetBufferSize();
        state.PS.pShaderBytecode = ps->GetBufferPointer();
        state.PS.BytecodeLength = ps->GetBufferSize();
        state.BlendState.AlphaToCoverageEnable = FALSE;
        state.BlendState.IndependentBlendEnable = FALSE;
        auto& blend = state.BlendState.RenderTarget[0];
        blend.BlendEnable = FALSE;
        blend.LogicOpEnable = FALSE;
        blend.SrcBlend = D3D12_BLEND_ONE;
        blend.DestBlend = D3D12_BLEND_ZERO;
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.DestBlendAlpha = D3D12_BLEND_ZERO;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.LogicOp = D3D12_LOGIC_OP_NOOP;
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        state.SampleMask = UINT_MAX;
        state.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        state.RasterizerState.FrontCounterClockwise = FALSE;
        state.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        state.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        state.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        state.RasterizerState.DepthClipEnable = TRUE;
        state.RasterizerState.MultisampleEnable = FALSE;
        state.RasterizerState.AntialiasedLineEnable = FALSE;
        state.RasterizerState.ForcedSampleCount = 0;
        state.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        state.DepthStencilState.DepthEnable = FALSE;
        state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        state.DepthStencilState.StencilEnable = FALSE;
        state.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        state.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        state.InputLayout = {nullptr, 0};
        state.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        state.NumRenderTargets = 1;
        state.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
        state.DSVFormat = DXGI_FORMAT_UNKNOWN;
        state.SampleDesc.Count = 1;
        state.SampleDesc.Quality = 0;
        state.NodeMask = 0;
        state.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        hr = device_->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&pso_));
        vs->Release(); ps->Release();
        if (compiler) FreeLibrary(compiler);
        if (releaseD3D12 && d3d12) FreeLibrary(d3d12);
        if (FAILED(hr) || !pso_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=create_pso hr=0x%08lX", static_cast<unsigned long>(hr));
            return false;
        }

        hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators_[0], pso_, IID_PPV_ARGS(&commandList_));
        if (FAILED(hr) || !commandList_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=create_command_list hr=0x%08lX", static_cast<unsigned long>(hr));
            return false;
        }
        hr = commandList_->Close();
        if (FAILED(hr)) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=close_initial_list hr=0x%08lX", static_cast<unsigned long>(hr));
            return false;
        }
        hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
        if (FAILED(hr) || !fence_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=create_fence hr=0x%08lX", static_cast<unsigned long>(hr));
            return false;
        }
        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_) {
            Log("HDR10_BRIDGE_INIT_FAIL stage=create_event error=%lu", GetLastError());
            return false;
        }
        Log("HDR10_BRIDGE_PIPELINE_READY root=%p pso=%p list=%p fence=%p shader_source=embedded_dynamic_compile no_static_d3dcompiler_import=1",
            rootSignature_, pso_, commandList_, fence_);
        return true;
    }

    HRESULT WaitAllocator(UINT index) noexcept {
        if (index >= allocatorFenceValues_.size() || !fence_ || !fenceEvent_) return E_INVALIDARG;
        const UINT64 value = allocatorFenceValues_[index];
        if (!value || fence_->GetCompletedValue() >= value) return S_OK;
        HRESULT hr = fence_->SetEventOnCompletion(value, fenceEvent_);
        if (FAILED(hr)) return hr;
        const DWORD wait = WaitForSingleObject(fenceEvent_, 5000);
        return wait == WAIT_OBJECT_0 ? S_OK : HRESULT_FROM_WIN32(wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
    }

    void WaitForConversions() noexcept {
        if (!fence_ || !fenceEvent_) return;
        UINT64 value = 0;
        for (UINT64 v : allocatorFenceValues_) value = std::max(value, v);
        if (!value || fence_->GetCompletedValue() >= value) return;
        if (SUCCEEDED(fence_->SetEventOnCompletion(value, fenceEvent_))) WaitForSingleObject(fenceEvent_, 5000);
    }

    HRESULT ConvertCurrentBackBuffer() noexcept {
        ID3D12CommandQueue* queue = GetHdr10BridgeDirectQueue();
        if (!queue || !commandList_ || !pso_ || !rootSignature_ || !srvHeap_ || !rtvHeap_ || !fence_) return E_FAIL;
        const UINT index = inner_->GetCurrentBackBufferIndex();
        if (index >= bufferCount_ || index >= shadows_.size() || index >= realBuffers_.size() ||
            index >= allocators_.size() || !shadows_[index] || !realBuffers_[index] || !allocators_[index]) return DXGI_ERROR_INVALID_CALL;
        const auto selection=FGNativeSourceSelect(this,shadows_.data(),bufferCount_,index);
        const UINT sourceIndex=selection.source;
        const bool sourceVerified=selection.status==control_fg_native_source::Status::Matched;
        if(sourceIndex>=shadows_.size()||!shadows_[sourceIndex])return DXGI_ERROR_INVALID_CALL;
        if(FGAlignActive(presentCount.load())||FGPixelNeedsPresent(presentCount.load())||(presentCount.load()%120)==0)
            Log("FG_NATIVE_SOURCE frame=%llu native_index=%u source=%u destination=%u native_resource=%p verified=%u corrected=%u reason=%s",
                presentCount.load(),selection.nativeIndex,sourceIndex,index,reinterpret_cast<void*>(selection.resource),unsigned(sourceVerified),unsigned(sourceVerified&&sourceIndex!=index),control_fg_native_source::Name(selection.status));
        FGAlignBridge(presentCount.load(),sourceIndex,shadows_[sourceIndex],realBuffers_[index],queue);
        HRESULT hr = WaitAllocator(index);
        if (FAILED(hr)) return hr;
        hr = allocators_[index]->Reset();
        if (FAILED(hr)) return hr;
        hr = commandList_->Reset(allocators_[index], pso_);
        if (FAILED(hr)) return hr;
        FGPixelBridge(commandList_,shadows_[sourceIndex],presentCount.load(),sourceIndex);
        if(FGPixelNeedsPresent(presentCount.load())) {
            for(UINT candidate=0;candidate<shadows_.size()&&candidate<4;++candidate)
                if(shadows_[candidate])FGPixelBridge(commandList_,shadows_[candidate],presentCount.load(),candidate,6+candidate);
            Log("PIXEL_HDR_SELECTION frame=%llu selected=%u count=%u captured=%u",presentCount.load(),sourceIndex,bufferCount_,bufferCount_<4?bufferCount_:4);
        }

        D3D12_RESOURCE_BARRIER barriers[2]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = shadows_[sourceIndex];
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = realBuffers_[index];
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList_->ResourceBarrier(2, barriers);

        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(width_);
        viewport.Height = static_cast<float>(height_);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        D3D12_RECT scissor{0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissor);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += SIZE_T(index) * SIZE_T(rtvIncrement_);
        commandList_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        commandList_->SetGraphicsRootSignature(rootSignature_);
        ID3D12DescriptorHeap* heaps[] = {srvHeap_};
        commandList_->SetDescriptorHeaps(1, heaps);
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpu = srvHeap_->GetGPUDescriptorHandleForHeapStart();
        srvGpu.ptr += UINT64(sourceIndex) * UINT64(srvIncrement_);
        commandList_->SetGraphicsRootDescriptorTable(0, srvGpu);
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList_->DrawInstanced(3, 1, 0, 0);

        std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
        std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
        commandList_->ResourceBarrier(2, barriers);
        FGPixelBridge(commandList_,realBuffers_[index],presentCount.load(),index,5);
        hr = commandList_->Close();
        if (FAILED(hr)) return hr;
        ID3D12CommandList* lists[] = {commandList_};
        queue->ExecuteCommandLists(1, lists);
        const UINT64 value = ++nextFenceValue_;
        hr = queue->Signal(fence_, value);
        if (FAILED(hr)) return hr;
        allocatorFenceValues_[index] = value;
        const auto conversions = ++hdr10BridgeConversions;
        if (conversions <= 16 || (conversions % 240) == 0) {
            Log("HDR10_BRIDGE_CONVERT conversion=%llu index=%u width=%u height=%u source_format=%u target_format=%u source_space=scrgb_linear target_space=bt2020_pq queue=%p fence=%llu per_buffer_srv=1",
                conversions, index, width_, height_, unsigned(gameFormat_), unsigned(DXGI_FORMAT_R10G10B10A2_UNORM), queue, value);
        }
        return S_OK;
    }

    bool ForceHdr10ColorSpace(const char* reason) noexcept {
        if (!inner_) return false;
        UINT support = 0;
        HRESULT hr = inner_->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &support);
        if (FAILED(hr) || (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == 0) {
            Log("HDR10_BRIDGE_COLORSPACE_FAIL stage=check reason=%s hr=0x%08lX support=0x%X color_space=%u",
                reason ? reason : "unknown", static_cast<unsigned long>(hr), support, unsigned(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020));
            return false;
        }
        hr = inner_->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        const bool ok = SUCCEEDED(hr);
        if (ok) ++hdr10BridgeColorSpaceForces;
        Log("HDR10_BRIDGE_COLORSPACE_%s reason=%s hr=0x%08lX support=0x%X color_space=%u forces=%llu",
            ok ? "OK" : "FAIL", reason ? reason : "unknown", static_cast<unsigned long>(hr), support,
            unsigned(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020), hdr10BridgeColorSpaceForces.load());
        return ok;
    }

    void ApplyHdr10Metadata() noexcept {
        if (!inner_) return;
        float maxLuminance = 1000.0f;
        float minLuminance = 0.001f;
        float maxFullFrame = 400.0f;
        IDXGIOutput* output = nullptr;
        if (SUCCEEDED(inner_->GetContainingOutput(&output)) && output) {
            IDXGIOutput6* output6 = nullptr;
            if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))) && output6) {
                DXGI_OUTPUT_DESC1 desc{};
                if (SUCCEEDED(output6->GetDesc1(&desc))) {
                    if (std::isfinite(desc.MaxLuminance) && desc.MaxLuminance >= 80.0f) maxLuminance = desc.MaxLuminance;
                    if (std::isfinite(desc.MinLuminance) && desc.MinLuminance >= 0.0f) minLuminance = desc.MinLuminance;
                    if (std::isfinite(desc.MaxFullFrameLuminance) && desc.MaxFullFrameLuminance > 0.0f)
                        maxFullFrame = desc.MaxFullFrameLuminance;
                    Log("HDR10_BRIDGE_OUTPUT color_space=%u bits_per_color=%u min_nits=%.6g max_nits=%.6g max_full_frame_nits=%.6g",
                        unsigned(desc.ColorSpace), desc.BitsPerColor, desc.MinLuminance, desc.MaxLuminance, desc.MaxFullFrameLuminance);
                }
                output6->Release();
            }
            output->Release();
        }
        if (maxLuminance < 80.0f) maxLuminance = 1000.0f;
        maxFullFrame = std::max(1.0f, std::min(maxFullFrame, maxLuminance));
        minLuminance = std::max(0.0f, std::min(minLuminance, maxLuminance));

        DXGI_HDR_METADATA_HDR10 metadata{};
        metadata.RedPrimary[0] = 35400; metadata.RedPrimary[1] = 14600;
        metadata.GreenPrimary[0] = 8500; metadata.GreenPrimary[1] = 39850;
        metadata.BluePrimary[0] = 6550; metadata.BluePrimary[1] = 2300;
        metadata.WhitePoint[0] = 15635; metadata.WhitePoint[1] = 16450;
        metadata.MaxMasteringLuminance = static_cast<UINT>(std::min(429496.7295f, maxLuminance) * 10000.0f + 0.5f);
        metadata.MinMasteringLuminance = static_cast<UINT>(std::min(429496.7295f, minLuminance) * 10000.0f + 0.5f);
        metadata.MaxContentLightLevel = static_cast<UINT16>(std::min(65535.0f, maxLuminance) + 0.5f);
        metadata.MaxFrameAverageLightLevel = static_cast<UINT16>(std::min(65535.0f, maxFullFrame) + 0.5f);
        const HRESULT hr = inner_->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(metadata), &metadata);
        Log("HDR10_BRIDGE_METADATA_SET hr=0x%08lX max_mastering_nits=%.6g min_mastering_nits=%.6g max_cll=%u max_fall=%u primaries=bt2020 white=d65",
            static_cast<unsigned long>(hr), maxLuminance, minLuminance,
            metadata.MaxContentLightLevel, metadata.MaxFrameAverageLightLevel);
    }

    void ReleaseResources() noexcept {
        for (auto*& p : shadows_) { if (p) { p->Release(); p = nullptr; } }
        for (auto*& p : realBuffers_) { if (p) { p->Release(); p = nullptr; } }
        for (auto*& p : allocators_) { if (p) { p->Release(); p = nullptr; } }
        shadows_.clear(); realBuffers_.clear(); allocators_.clear(); allocatorFenceValues_.clear();
        if (srvHeap_) { srvHeap_->Release(); srvHeap_ = nullptr; }
        if (rtvHeap_) { rtvHeap_->Release(); rtvHeap_ = nullptr; }
        srvIncrement_ = 0; rtvIncrement_ = 0; width_ = 0; height_ = 0; bufferCount_ = 0;
    }

    FGSDRCorrection sdrCorrection_{};
    std::atomic<ULONG> refs_{1};
    IDXGISwapChain4* inner_{};
    ID3D12Device* device_{};
    DXGI_FORMAT gameFormat_{DXGI_FORMAT_UNKNOWN};
    DXGI_USAGE gameUsage_{};
    bool active_{};
    bool hasPassthroughColorSpace_{};
    DXGI_COLOR_SPACE_TYPE lastPassthroughColorSpace_{DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709};
    UINT width_{};
    UINT height_{};
    UINT bufferCount_{};
    std::vector<ID3D12Resource*> shadows_{};
    std::vector<ID3D12Resource*> realBuffers_{};
    std::vector<ID3D12CommandAllocator*> allocators_{};
    std::vector<UINT64> allocatorFenceValues_{};
    ID3D12RootSignature* rootSignature_{};
    ID3D12PipelineState* pso_{};
    ID3D12DescriptorHeap* srvHeap_{};
    ID3D12DescriptorHeap* rtvHeap_{};
    ID3D12GraphicsCommandList* commandList_{};
    ID3D12Fence* fence_{};
    HANDLE fenceEvent_{};
    UINT srvIncrement_{};
    UINT rtvIncrement_{};
    UINT64 nextFenceValue_{};
};

static Hdr10SwapChainProxy* CreateHdr10SwapChainWrapper(IUnknown* created,
    DXGI_FORMAT gameFormat, DXGI_USAGE gameUsage, bool activateNow, const char* entrypoint) noexcept {
    if (!created) return nullptr;
    IDXGISwapChain4* inner4 = nullptr;
    const HRESULT qi = created->QueryInterface(IID_PPV_ARGS(&inner4));
    if (FAILED(qi) || !inner4) {
        Log("HDR10_BRIDGE_WRAP_FAIL stage=swapchain4_qi entrypoint=%s hr=0x%08lX object=%p",
            entrypoint ? entrypoint : "unknown", static_cast<unsigned long>(qi), created);
        return nullptr;
    }
    auto* wrapper = new (std::nothrow) Hdr10SwapChainProxy(inner4, gameFormat, gameUsage);
    if (!wrapper) {
        inner4->Release();
        Log("HDR10_BRIDGE_WRAP_FAIL stage=allocation entrypoint=%s", entrypoint ? entrypoint : "unknown");
        return nullptr;
    }
    if (!wrapper->Initialize(activateNow)) {
        wrapper->Release();
        Log("HDR10_BRIDGE_WRAP_FAIL stage=initialize entrypoint=%s retry_original=1", entrypoint ? entrypoint : "unknown");
        return nullptr;
    }
    return wrapper;
}

class Hdr10FactoryProxy final : public IDXGIFactory7 {
public:
    explicit Hdr10FactoryProxy(IDXGIFactory7* inner) noexcept : inner_(inner) {}
    virtual ~Hdr10FactoryProxy() { if (inner_) inner_->Release(); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIObject) ||
            riid == __uuidof(IDXGIFactory) || riid == __uuidof(IDXGIFactory1) ||
            riid == __uuidof(IDXGIFactory2) || riid == __uuidof(IDXGIFactory3) ||
            riid == __uuidof(IDXGIFactory4) || riid == __uuidof(IDXGIFactory5) ||
            riid == __uuidof(IDXGIFactory6) || riid == __uuidof(IDXGIFactory7)) {
            *ppvObject = static_cast<IDXGIFactory7*>(this);
            AddRef();
            return S_OK;
        }
        return inner_ ? inner_->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = --refs_;
        if (!value) delete this;
        return value;
    }

    // IDXGIObject
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override { return inner_->SetPrivateData(Name, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override { return inner_->SetPrivateDataInterface(Name, pUnknown); }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override { return inner_->GetPrivateData(Name, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override { return inner_->GetParent(riid, ppParent); }

    // IDXGIFactory
    HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) override { return inner_->EnumAdapters(Adapter, ppAdapter); }
    HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) override { return inner_->MakeWindowAssociation(WindowHandle, Flags); }
    HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle) override { return inner_->GetWindowAssociation(pWindowHandle); }
    HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override {
        if (!ppSwapChain) return E_POINTER;
        *ppSwapChain = nullptr;
        if (!pDesc || !ShouldWatchHdr10BridgeTransition(pDesc->BufferDesc.Format) || !GetHdr10BridgeDirectQueue())
            return inner_->CreateSwapChain(pDevice, pDesc, ppSwapChain);

        const DXGI_FORMAT gameFormat = pDesc->BufferDesc.Format;
        const DXGI_USAGE gameUsage = pDesc->BufferUsage;
        const bool activateNow = ShouldAttemptHdr10Bridge(gameFormat);
        DXGI_SWAP_CHAIN_DESC real = *pDesc;
        if (activateNow) {
            real.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            real.BufferUsage = (real.BufferUsage | DXGI_USAGE_RENDER_TARGET_OUTPUT) & ~DXGI_USAGE_UNORDERED_ACCESS;
        }
        IDXGISwapChain* created = nullptr;
        HRESULT hr = inner_->CreateSwapChain(pDevice, &real, &created);
        Log("HDR10_BRIDGE_CREATE entrypoint=CreateSwapChain game_format=%u real_format=%u initial_mode=%s usage_game=0x%X usage_real=0x%X hr=0x%08lX object=%p",
            unsigned(gameFormat), unsigned(real.BufferDesc.Format), activateNow ? "active" : "dormant", unsigned(gameUsage),
            unsigned(real.BufferUsage), static_cast<unsigned long>(hr), created);
        if (SUCCEEDED(hr) && created) {
            auto* wrapper = CreateHdr10SwapChainWrapper(created, gameFormat, gameUsage, activateNow, "CreateSwapChain");
            if (wrapper) {
                created->Release();
                *ppSwapChain = static_cast<IDXGISwapChain*>(wrapper);
                return S_OK;
            }
            if (!activateNow) {
                // Dormant wrapping is optional; preserve the already-created original SDR chain.
                *ppSwapChain = created;
                Log("HDR10_BRIDGE_DORMANT_FALLBACK entrypoint=CreateSwapChain reason=wrap_failed original_chain_preserved=1");
                return hr;
            }
            created->Release();
        }
        Log("HDR10_BRIDGE_FALLBACK entrypoint=CreateSwapChain reason=rgb10_create_or_wrap_failed retry_original=1");
        return inner_->CreateSwapChain(pDevice, pDesc, ppSwapChain);
    }
    HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) override { return inner_->CreateSoftwareAdapter(Module, ppAdapter); }

    // IDXGIFactory1
    HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter) override { return inner_->EnumAdapters1(Adapter, ppAdapter); }
    BOOL STDMETHODCALLTYPE IsCurrent() override { return inner_->IsCurrent(); }

    // IDXGIFactory2
    BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled() override { return inner_->IsWindowedStereoEnabled(); }
    HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override {
        if (!ppSwapChain) return E_POINTER;
        *ppSwapChain = nullptr;
        if (!pDesc || !ShouldWatchHdr10BridgeTransition(pDesc->Format) || !GetHdr10BridgeDirectQueue())
            return inner_->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

        const DXGI_FORMAT gameFormat = pDesc->Format;
        const DXGI_USAGE gameUsage = pDesc->BufferUsage;
        const bool activateNow = ShouldAttemptHdr10Bridge(gameFormat);
        DXGI_SWAP_CHAIN_DESC1 real = *pDesc;
        if (activateNow) {
            real.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            real.BufferUsage = (real.BufferUsage | DXGI_USAGE_RENDER_TARGET_OUTPUT) & ~DXGI_USAGE_UNORDERED_ACCESS;
        }
        IDXGISwapChain1* created = nullptr;
        HRESULT hr = inner_->CreateSwapChainForHwnd(pDevice, hWnd, &real, pFullscreenDesc, pRestrictToOutput, &created);
        Log("HDR10_BRIDGE_CREATE entrypoint=CreateSwapChainForHwnd game_format=%u real_format=%u initial_mode=%s usage_game=0x%X usage_real=0x%X hr=0x%08lX object=%p",
            unsigned(gameFormat), unsigned(real.Format), activateNow ? "active" : "dormant", unsigned(gameUsage),
            unsigned(real.BufferUsage), static_cast<unsigned long>(hr), created);
        if (SUCCEEDED(hr) && created) {
            auto* wrapper = CreateHdr10SwapChainWrapper(created, gameFormat, gameUsage, activateNow, "CreateSwapChainForHwnd");
            if (wrapper) {
                created->Release();
                *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapper);
                return S_OK;
            }
            if (!activateNow) {
                *ppSwapChain = created;
                Log("HDR10_BRIDGE_DORMANT_FALLBACK entrypoint=CreateSwapChainForHwnd reason=wrap_failed original_chain_preserved=1");
                return hr;
            }
            created->Release();
        }
        Log("HDR10_BRIDGE_FALLBACK entrypoint=CreateSwapChainForHwnd reason=rgb10_create_or_wrap_failed retry_original=1");
        return inner_->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
    }
    HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override {
        if (!ppSwapChain) return E_POINTER;
        *ppSwapChain = nullptr;
        if (!pDesc || !ShouldWatchHdr10BridgeTransition(pDesc->Format) || !GetHdr10BridgeDirectQueue())
            return inner_->CreateSwapChainForCoreWindow(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

        const DXGI_FORMAT gameFormat = pDesc->Format;
        const DXGI_USAGE gameUsage = pDesc->BufferUsage;
        const bool activateNow = ShouldAttemptHdr10Bridge(gameFormat);
        DXGI_SWAP_CHAIN_DESC1 real = *pDesc;
        if (activateNow) {
            real.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            real.BufferUsage = (real.BufferUsage | DXGI_USAGE_RENDER_TARGET_OUTPUT) & ~DXGI_USAGE_UNORDERED_ACCESS;
        }
        IDXGISwapChain1* created = nullptr;
        HRESULT hr = inner_->CreateSwapChainForCoreWindow(pDevice, pWindow, &real, pRestrictToOutput, &created);
        Log("HDR10_BRIDGE_CREATE entrypoint=CreateSwapChainForCoreWindow game_format=%u real_format=%u initial_mode=%s hr=0x%08lX object=%p",
            unsigned(gameFormat), unsigned(real.Format), activateNow ? "active" : "dormant", static_cast<unsigned long>(hr), created);
        if (SUCCEEDED(hr) && created) {
            auto* wrapper = CreateHdr10SwapChainWrapper(created, gameFormat, gameUsage, activateNow, "CreateSwapChainForCoreWindow");
            if (wrapper) {
                created->Release();
                *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapper);
                return S_OK;
            }
            if (!activateNow) {
                *ppSwapChain = created;
                Log("HDR10_BRIDGE_DORMANT_FALLBACK entrypoint=CreateSwapChainForCoreWindow reason=wrap_failed original_chain_preserved=1");
                return hr;
            }
            created->Release();
        }
        Log("HDR10_BRIDGE_FALLBACK entrypoint=CreateSwapChainForCoreWindow reason=rgb10_create_or_wrap_failed retry_original=1");
        return inner_->CreateSwapChainForCoreWindow(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
    }
    HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid) override { return inner_->GetSharedResourceAdapterLuid(hResource, pLuid); }
    HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override { return inner_->RegisterStereoStatusWindow(WindowHandle, wMsg, pdwCookie); }
    HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override { return inner_->RegisterStereoStatusEvent(hEvent, pdwCookie); }
    void STDMETHODCALLTYPE UnregisterStereoStatus(DWORD dwCookie) override { inner_->UnregisterStereoStatus(dwCookie); }
    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override { return inner_->RegisterOcclusionStatusWindow(WindowHandle, wMsg, pdwCookie); }
    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override { return inner_->RegisterOcclusionStatusEvent(hEvent, pdwCookie); }
    void STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD dwCookie) override { inner_->UnregisterOcclusionStatus(dwCookie); }
    HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override {
        if (!ppSwapChain) return E_POINTER;
        *ppSwapChain = nullptr;
        if (!pDesc || !ShouldWatchHdr10BridgeTransition(pDesc->Format) || !GetHdr10BridgeDirectQueue())
            return inner_->CreateSwapChainForComposition(pDevice, pDesc, pRestrictToOutput, ppSwapChain);

        const DXGI_FORMAT gameFormat = pDesc->Format;
        const DXGI_USAGE gameUsage = pDesc->BufferUsage;
        const bool activateNow = ShouldAttemptHdr10Bridge(gameFormat);
        DXGI_SWAP_CHAIN_DESC1 real = *pDesc;
        if (activateNow) {
            real.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            real.BufferUsage = (real.BufferUsage | DXGI_USAGE_RENDER_TARGET_OUTPUT) & ~DXGI_USAGE_UNORDERED_ACCESS;
        }
        IDXGISwapChain1* created = nullptr;
        HRESULT hr = inner_->CreateSwapChainForComposition(pDevice, &real, pRestrictToOutput, &created);
        Log("HDR10_BRIDGE_CREATE entrypoint=CreateSwapChainForComposition game_format=%u real_format=%u initial_mode=%s hr=0x%08lX object=%p",
            unsigned(gameFormat), unsigned(real.Format), activateNow ? "active" : "dormant", static_cast<unsigned long>(hr), created);
        if (SUCCEEDED(hr) && created) {
            auto* wrapper = CreateHdr10SwapChainWrapper(created, gameFormat, gameUsage, activateNow, "CreateSwapChainForComposition");
            if (wrapper) {
                created->Release();
                *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapper);
                return S_OK;
            }
            if (!activateNow) {
                *ppSwapChain = created;
                Log("HDR10_BRIDGE_DORMANT_FALLBACK entrypoint=CreateSwapChainForComposition reason=wrap_failed original_chain_preserved=1");
                return hr;
            }
            created->Release();
        }
        Log("HDR10_BRIDGE_FALLBACK entrypoint=CreateSwapChainForComposition reason=rgb10_create_or_wrap_failed retry_original=1");
        return inner_->CreateSwapChainForComposition(pDevice, pDesc, pRestrictToOutput, ppSwapChain);
    }

    // IDXGIFactory3
    UINT STDMETHODCALLTYPE GetCreationFlags() override { return inner_->GetCreationFlags(); }

    // IDXGIFactory4
    HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID AdapterLuid, REFIID riid, void** ppvAdapter) override { return inner_->EnumAdapterByLuid(AdapterLuid, riid, ppvAdapter); }
    HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID riid, void** ppvAdapter) override { return inner_->EnumWarpAdapter(riid, ppvAdapter); }

    // IDXGIFactory5
    HRESULT STDMETHODCALLTYPE CheckFeatureSupport(DXGI_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize) override {
        return inner_->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
    }

    // IDXGIFactory6
    HRESULT STDMETHODCALLTYPE EnumAdapterByGpuPreference(UINT Adapter, DXGI_GPU_PREFERENCE GpuPreference,
        REFIID riid, void** ppvAdapter) override {
        return inner_->EnumAdapterByGpuPreference(Adapter, GpuPreference, riid, ppvAdapter);
    }

    // IDXGIFactory7
    HRESULT STDMETHODCALLTYPE RegisterAdaptersChangedEvent(HANDLE hEvent, DWORD* pdwCookie) override {
        return inner_->RegisterAdaptersChangedEvent(hEvent, pdwCookie);
    }
    HRESULT STDMETHODCALLTYPE UnregisterAdaptersChangedEvent(DWORD dwCookie) override {
        return inner_->UnregisterAdaptersChangedEvent(dwCookie);
    }

private:
    std::atomic<ULONG> refs_{1};
    IDXGIFactory7* inner_{};
};

static bool WrapFactoryForHdr10Bridge(void** factory, REFIID requestedIid, const char* entrypoint) noexcept {
    if (!factory || !*factory) return false;
    if (Hdr10BridgeDisabledByEnvironment()) {
        Log("HDR10_FACTORY_WRAP_SKIP entrypoint=%s reason=disabled_by_environment", entrypoint ? entrypoint : "unknown");
        return false;
    }
    IUnknown* raw = reinterpret_cast<IUnknown*>(*factory);
    IDXGIFactory7* factory7 = nullptr;
    const HRESULT qi = raw->QueryInterface(IID_PPV_ARGS(&factory7));
    if (FAILED(qi) || !factory7) {
        Log("HDR10_FACTORY_WRAP_SKIP entrypoint=%s reason=factory7_unavailable hr=0x%08lX factory=%p", entrypoint ? entrypoint : "unknown", static_cast<unsigned long>(qi), raw);
        return false;
    }
    auto* wrapper = new (std::nothrow) Hdr10FactoryProxy(factory7);
    if (!wrapper) {
        factory7->Release();
        Log("HDR10_FACTORY_WRAP_SKIP entrypoint=%s reason=allocation_failed", entrypoint ? entrypoint : "unknown");
        return false;
    }
    void* requested = nullptr;
    const HRESULT wrappedQi = wrapper->QueryInterface(requestedIid, &requested);
    wrapper->Release(); // release construction reference; requested interface owns the returned reference.
    if (FAILED(wrappedQi) || !requested) {
        Log("HDR10_FACTORY_WRAP_SKIP entrypoint=%s reason=requested_iid_unsupported hr=0x%08lX", entrypoint ? entrypoint : "unknown", static_cast<unsigned long>(wrappedQi));
        return false;
    }
    raw->Release();
    *factory = requested;
    const auto wraps = ++hdr10BridgeFactoryWraps;
    Log("HDR10_FACTORY_WRAP entrypoint=%s factory=%p requested_iid_preserved=1 wraps=%llu", entrypoint ? entrypoint : "unknown", requested, wraps);
    return true;
}
