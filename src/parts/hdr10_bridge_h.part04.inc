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
        HRESULT hr = WaitAllocator(index);
        if (FAILED(hr)) return hr;
        hr = allocators_[index]->Reset();
        if (FAILED(hr)) return hr;
        hr = commandList_->Reset(allocators_[index], pso_);
        if (FAILED(hr)) return hr;

        D3D12_RESOURCE_BARRIER barriers[2]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = shadows_[index];
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
        srvGpu.ptr += UINT64(index) * UINT64(srvIncrement_);
        commandList_->SetGraphicsRootDescriptorTable(0, srvGpu);
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList_->DrawInstanced(3, 1, 0, 0);

        std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
        std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
        commandList_->ResourceBarrier(2, barriers);
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
        Log("HDR10_BRIDGE_METADATA_SET hr=0x%08lX max_nits=%.6g min_nits=%.6g max_full_frame_nits=%.6g",
            static_cast<unsigned long>(hr), maxLuminance, minLuminance, maxFullFrame);
    }

    void ClearActiveState(const char* reason) noexcept {
        const bool wasActive = active_;
        active_ = false;
        if (wasActive) ++hdr10BridgeDeactivationCount;
        hdr10BridgeActive.store(0, std::memory_order_release);
        hdr10BridgeWidth.store(0, std::memory_order_release);
        hdr10BridgeHeight.store(0, std::memory_order_release);
        Log("HDR10_BRIDGE_DEACTIVATED reason=%s deactivations=%llu wrapper=%p inner=%p",
            reason ? reason : "unknown", hdr10BridgeDeactivationCount.load(), this, inner_);
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
