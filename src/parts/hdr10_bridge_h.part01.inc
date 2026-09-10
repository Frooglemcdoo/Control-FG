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
        if (active_ && (Flags & DXGI_PRESENT_TEST) == 0) {
            const HRESULT convert = ConvertCurrentBackBuffer();
            if (FAILED(convert)) {
                ++hdr10BridgeConversionFailures;
                Log("HDR10_BRIDGE_CONVERT_FAIL stage=present hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
                Log("HDR10_BRIDGE_PRESENT_FAIL stage=convert hr=0x%08lX failures=%llu", static_cast<unsigned long>(convert), hdr10BridgeConversionFailures.load());
            }
        }
        const UINT appliedSyncInterval = (Flags & DXGI_PRESENT_TEST) != 0 ? SyncInterval : ApplyDynamicMFGPresentSyncInterval(SyncInterval, "Present");
        return inner_->Present(appliedSyncInterval, Flags);
    }
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) override {
        if (!active_) return inner_->GetBuffer(Buffer, riid, ppSurface);
        if (!ppSurface) return E_POINTER;
        *ppSurface = nullptr;
        if (Buffer >= shadowBuffers_.size() || !shadowBuffers_[Buffer]) return DXGI_ERROR_INVALID_CALL;
        return shadowBuffers_[Buffer]->QueryInterface(riid, ppSurface);
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
        const DXGI_FORMAT requested = NewFormat == DXGI_FORMAT_UNKNOWN ? gameFormat_ : NewFormat;
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
        if (SUCCEEDED(hr)) {
            gameFormat_ = requested;
            ClearActiveState("resize_passthrough");
            Log("HDR10_BRIDGE_RESIZE_OK stage=deactivate count=%u width=%u height=%u game_format=%u real_format=%u resizes=%llu",
                BufferCount, Width, Height, unsigned(gameFormat_), unsigned(requested), ++hdr10BridgeResizeCount);
        }
        return hr;
    }
