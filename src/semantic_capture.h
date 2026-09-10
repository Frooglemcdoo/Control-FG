#pragma once
// Included after probe globals and Log. No NVIDIA libraries or SDK are loaded.
// Getter signatures match the public NGX C API; pointers resolve from the game.
using NgxGetResource = unsigned int (*)(void*, const char*, ID3D12Resource**);
using NgxGetFloat = unsigned int (*)(void*, const char*, float*);
using NgxGetUInt = unsigned int (*)(void*, const char*, unsigned int*);
using NgxGetInt = unsigned int (*)(void*, const char*, int*);
static NgxGetResource ngxGetResource = nullptr;
static NgxGetFloat ngxGetFloat = nullptr;
static NgxGetUInt ngxGetUInt = nullptr;
static NgxGetInt ngxGetInt = nullptr;

// Presentation-path identities are pointer values only. The probe never retains
// a COM reference to the DLSS output and never dereferences a stored pointer at
// Present time. Current resources are queried synchronously when sampled.
static std::atomic<void*> lastDlssOutput{nullptr};
static std::atomic<unsigned long long> lastDlssOutputCall{0};
static std::atomic<unsigned long long> lastDlssOutputPresent{0};
static std::atomic<unsigned long long> lastDlssOutputEngineFrame{0};
static std::atomic<unsigned long long> pathLastAaCall{0};
static std::atomic<unsigned long long> pathLastAaPresent{0};
static std::atomic<unsigned long long> pathLastAaEngineFrame{0};
static std::atomic<unsigned long long> swapSamples{0};
static std::atomic<unsigned long long> swapFullEnumerations{0};
static std::atomic<void*> lastSwapBackbuffer{nullptr};
static std::atomic<unsigned int> lastSwapBackbufferIndex{0xFFFFFFFFu};
static std::atomic<unsigned long long> lastSwapBackbufferPresent{0};
static std::atomic<void*> lastSwapModeChain{nullptr};
static std::atomic<unsigned int> lastSwapModeFormat{0xFFFFFFFFu};
static std::atomic<unsigned int> lastSwapModeHdr{0xFFFFFFFFu};

static bool ResolveSemanticGetters(HMODULE module) noexcept {
    ngxGetResource = reinterpret_cast<NgxGetResource>(GetProcAddress(module, "NVSDK_NGX_Parameter_GetD3d12Resource"));
    ngxGetFloat = reinterpret_cast<NgxGetFloat>(GetProcAddress(module, "NVSDK_NGX_Parameter_GetF"));
    ngxGetUInt = reinterpret_cast<NgxGetUInt>(GetProcAddress(module, "NVSDK_NGX_Parameter_GetUI"));
    ngxGetInt = reinterpret_cast<NgxGetInt>(GetProcAddress(module, "NVSDK_NGX_Parameter_GetI"));
    return ngxGetResource && ngxGetFloat && ngxGetUInt && ngxGetInt;
}

// SEH leaves below contain POD only. They are deliberately small because these
// are build-locked reads of game-owned state.
static bool ReadEngineFrameSafe(unsigned long long* value, DWORD* fault) noexcept {
    *value = 0;
    *fault = 0;
    __try {
        if (!verifiedD3d) return false;
        *value = *reinterpret_cast<const unsigned long long*>(
            reinterpret_cast<unsigned char*>(verifiedD3d) + kEngineFrameCounterRva);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        return false;
    }
}

static bool ReadNgxReset(int* value, unsigned int* status, DWORD* fault) noexcept {
    const DWORD saved = GetLastError();
    bool queried = false;
    *value = 0;
    *status = 0;
    *fault = 0;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = base ? *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva) : nullptr;
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        if (parameters && ngxGetInt) {
            *status = ngxGetInt(parameters, "Reset", value);
            queried = true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode();
        queried = false;
    }
    SetLastError(saved);
    return queried;
}

static void RememberAaFrame(unsigned long long call, unsigned long long present,
                            unsigned long long engineFrame) noexcept {
    pathLastAaCall.store(call);
    pathLastAaPresent.store(present);
    pathLastAaEngineFrame.store(engineFrame);
}

// Post-evaluation inspection. This confirms stored parameter values after
// evaluation; it does not intercept NGX EvaluateFeature. All pointers are
// borrowed and used synchronously on the game's AA thread; no Set* calls occur.
// Keep this function free of objects with C++ destructors because it uses SEH.
static void CaptureSemantic(unsigned long long call, void* const* slots,
                            const char* sampleReason) noexcept {
    const DWORD saved = GetLastError();
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva);
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        if (!parameters) {
            Log("NGX_POST_EVAL_UNAVAILABLE call=%llu reason=null_context_or_parameters sample_reason=%s", call, sampleReason);
        } else {
            Log("NGX_POST_EVAL call=%llu context=%p parameters=%p sample_reason=%s", call, context, parameters, sampleReason);
            const char* resourceKeys[] = {"Color", "Output", "Depth", "MotionVectors", "ExposureTexture", "TransparencyMask"};
            for (const char* key : resourceKeys) {
                ID3D12Resource* resource = nullptr;
                const unsigned int status = ngxGetResource(parameters, key, &resource);
                unsigned int slot = 0;
                if (status == 1 && resource) {
                    if (slots) {
                        for (unsigned int i = 0; i < 9; ++i) if (slots[i] == resource) slot = i + 1;
                    }
                    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
                    Log("NGX_RESOURCE call=%llu key=%s status=0x%08X slot=%u resource=%p width=%llu height=%u format=%u",
                        call, key, status, slot, resource, desc.Width, desc.Height, unsigned(desc.Format));
                    if (!strcmp(key, "Output")) {
                        unsigned long long engineFrame = 0;
                        DWORD frameFault = 0;
                        ReadEngineFrameSafe(&engineFrame, &frameFault);
                        void* previous = lastDlssOutput.exchange(resource);
                        lastDlssOutputCall.store(call);
                        lastDlssOutputPresent.store(presentCount.load());
                        lastDlssOutputEngineFrame.store(engineFrame);
                        if (previous && previous != resource) {
                            Log("DLSS_OUTPUT_CHANGED call=%llu old=%p new=%p engine_frame=%llu", call, previous, resource, engineFrame);
                        }
                    }
                } else {
                    Log("NGX_RESOURCE_EMPTY call=%llu key=%s status=0x%08X resource=%p", call, key, status, resource);
                }
            }
            const char* floatKeys[] = {"Jitter.Offset.X", "Jitter.Offset.Y", "MV.Scale.X", "MV.Scale.Y", "DLSS.Pre.Exposure", "DLSS.Exposure.Scale"};
            for (const char* key : floatKeys) {
                float value = 0;
                const unsigned int status = ngxGetFloat(parameters, key, &value);
                if (status == 1) Log("NGX_FLOAT call=%llu key=%s value=%.9g", call, key, double(value));
                else Log("NGX_PARAM_UNAVAILABLE call=%llu key=%s status=0x%08X", call, key, status);
            }
            const char* uintKeys[] = {"Width", "Height", "OutWidth", "OutHeight", "DLSS.Render.Subrect.Dimensions.Width", "DLSS.Render.Subrect.Dimensions.Height"};
            for (const char* key : uintKeys) {
                unsigned int value = 0;
                const unsigned int status = ngxGetUInt(parameters, key, &value);
                if (status == 1) Log("NGX_UINT call=%llu key=%s value=%u", call, key, value);
                else Log("NGX_PARAM_UNAVAILABLE call=%llu key=%s status=0x%08X", call, key, status);
            }
            const char* intKeys[] = {"Reset", "DLSS.Feature.Create.Flags", "DLSS.Indicator.Invert.X.Axis", "DLSS.Indicator.Invert.Y.Axis"};
            for (const char* key : intKeys) {
                int value = 0;
                const unsigned int status = ngxGetInt(parameters, key, &value);
                if (status == 1) {
                    Log("NGX_INT call=%llu key=%s value=%d", call, key, value);
                    if (!strcmp(key, "DLSS.Feature.Create.Flags")) {
                        Log("NGX_FEATURE_FLAGS call=%llu hdr_input=%u low_res_mv=%u jittered_mv=%u inverted_depth=%u",
                            call, unsigned((value & 1) != 0), unsigned((value & 2) != 0), unsigned((value & 4) != 0), unsigned((value & 8) != 0));
                    }
                } else Log("NGX_PARAM_UNAVAILABLE call=%llu key=%s status=0x%08X", call, key, status);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("NGX_QUERY_EXCEPTION call=%llu exception=0x%08lX sample_reason=%s", call, GetExceptionCode(), sampleReason);
    }
    SetLastError(saved);
}

// +0x178 is the object whose vtable slot 8 is called by DeviceUtil::present.
// v0.7 preserves v0.6 pre/post-Present sampling and re-queries buffer/color-space
// capabilities whenever the chain format or engine HDR state changes. DXGI exposes support
// and SetColorSpace1 here, but no getter for the currently selected color space.
// Only COM read/query operations occur; the probe never calls Present itself.
static void CaptureSwapChain(unsigned long long present, const char* phase,
                             bool enumerateAll) noexcept {
    const DWORD saved = GetLastError();
    IDXGISwapChain3* chain3 = nullptr;
    ID3D12Resource* currentBuffer = nullptr;
    ID3D12Resource* allBuffers[8]{};
    ID3D12Device* device = nullptr;
    unsigned long long engineFrame = 0;
    UINT currentIndex = 0xFFFFFFFFu;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto nativeDevice = *reinterpret_cast<unsigned char**>(base + kNativeDevicePointerRva);
        auto chain = nativeDevice ? *reinterpret_cast<IDXGISwapChain**>(nativeDevice + kSwapChainOffset) : nullptr;
        if (!chain) {
            Log("SWAPCHAIN_UNAVAILABLE present=%llu phase=%s reason=null_pointer", present, phase);
        } else {
            engineFrame = *reinterpret_cast<const unsigned long long*>(base + kEngineFrameCounterRva);
            Log("SWAPCHAIN_FRAME present=%llu phase=%s engine_frame=%llu", present, phase, engineFrame);
            DXGI_SWAP_CHAIN_DESC desc{};
            const HRESULT hr = chain->GetDesc(&desc);
            unsigned int hdrValue = 0xFFFFFFFFu;
            if (originalIsHDREnabled) {
                __try {
                    hdrValue = unsigned(originalIsHDREnabled());
                    Log("ENGINE_HDR_STATE present=%llu phase=%s enabled=%u source=DeviceUtil_isHDREnabled", present, phase, hdrValue);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    Log("ENGINE_HDR_STATE_UNAVAILABLE present=%llu phase=%s exception=0x%08lX", present, phase, GetExceptionCode());
                }
            }
            bool modeChanged = false;
            unsigned int oldFormat = lastSwapModeFormat.load();
            unsigned int oldHdr = lastSwapModeHdr.load();
            void* oldChain = lastSwapModeChain.load();
            if (SUCCEEDED(hr)) {
                Log("SWAPCHAIN present=%llu phase=%s aa=%llu chain=%p width=%u height=%u format=%u buffers=%u effect=%u flags=%u windowed=%u",
                    present, phase, aaCount.load(), chain, desc.BufferDesc.Width, desc.BufferDesc.Height,
                    unsigned(desc.BufferDesc.Format), desc.BufferCount, unsigned(desc.SwapEffect), desc.Flags, unsigned(desc.Windowed));
                const unsigned int newFormat = unsigned(desc.BufferDesc.Format);
                modeChanged = oldChain != chain || oldFormat != newFormat || oldHdr != hdrValue;
                if (modeChanged) {
                    Log("SWAPCHAIN_MODE_CHANGE present=%llu phase=%s old_chain=%p new_chain=%p old_format=%u new_format=%u old_hdr=%u new_hdr=%u",
                        present, phase, oldChain, chain, oldFormat, newFormat, oldHdr, hdrValue);
                    lastSwapModeChain.store(chain);
                    lastSwapModeFormat.store(newFormat);
                    lastSwapModeHdr.store(hdrValue);
                }
            } else Log("SWAPCHAIN_DESC_FAILED present=%llu phase=%s hr=0x%08lX", present, phase, static_cast<unsigned long>(hr));

            HRESULT query = chain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&chain3));
            if (SUCCEEDED(query) && chain3) {
                currentIndex = chain3->GetCurrentBackBufferIndex();
                query = chain3->GetBuffer(currentIndex, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&currentBuffer));
                if (SUCCEEDED(query) && currentBuffer) {
                    const D3D12_RESOURCE_DESC resource = currentBuffer->GetDesc();
                    lastSwapBackbuffer.store(currentBuffer);
                    lastSwapBackbufferIndex.store(currentIndex);
                    lastSwapBackbufferPresent.store(present);
                    Log("SWAPCHAIN_BUFFER present=%llu phase=%s index=%u resource=%p width=%llu height=%u format=%u flags=%u",
                        present, phase, currentIndex, currentBuffer, resource.Width, resource.Height, unsigned(resource.Format), unsigned(resource.Flags));
                } else Log("SWAPCHAIN_BUFFER_FAILED present=%llu phase=%s index=%u hr=0x%08lX", present, phase, currentIndex, static_cast<unsigned long>(query));

                const bool modeEnumeration = enumerateAll || modeChanged;
                if (modeEnumeration) {
                    UINT sdr = 0, scrgb = 0, hdr10 = 0;
                    const HRESULT sdrHr = chain3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, &sdr);
                    const HRESULT scrgbHr = chain3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, &scrgb);
                    const HRESULT hdr10Hr = chain3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &hdr10);
                    Log("SWAPCHAIN_COLOR_SUPPORT present=%llu reason=%s format=%u engine_hdr=%u sdr_hr=0x%08lX sdr=0x%X scrgb_hr=0x%08lX scrgb=0x%X hdr10_hr=0x%08lX hdr10=0x%X current_color_space=not_exposed_by_idxgiswapchain3",
                        present, modeChanged ? "mode_change" : "enumerate_all", SUCCEEDED(hr) ? unsigned(desc.BufferDesc.Format) : 0xFFFFFFFFu, hdrValue,
                        static_cast<unsigned long>(sdrHr), sdr, static_cast<unsigned long>(scrgbHr), scrgb,
                        static_cast<unsigned long>(hdr10Hr), hdr10);
                }

                if ((enumerateAll || modeChanged) && SUCCEEDED(hr)) {
                    const UINT count = desc.BufferCount < _countof(allBuffers) ? desc.BufferCount : static_cast<UINT>(_countof(allBuffers));
                    for (UINT i = 0; i < count; ++i) {
                        HRESULT bufferHr = chain3->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&allBuffers[i]));
                        if (SUCCEEDED(bufferHr) && allBuffers[i]) {
                            const D3D12_RESOURCE_DESC resource = allBuffers[i]->GetDesc();
                            Log("SWAPCHAIN_BUFFER_ALL present=%llu phase=%s reason=%s index=%u resource=%p width=%llu height=%u format=%u flags=%u",
                                present, phase, modeChanged ? "mode_change" : "enumerate_all", i, allBuffers[i], resource.Width, resource.Height, unsigned(resource.Format), unsigned(resource.Flags));
                        } else {
                            Log("SWAPCHAIN_BUFFER_ALL_FAILED present=%llu phase=%s index=%u hr=0x%08lX", present, phase, i, static_cast<unsigned long>(bufferHr));
                        }
                    }
                }
            } else Log("SWAPCHAIN3_UNAVAILABLE present=%llu phase=%s hr=0x%08lX", present, phase, static_cast<unsigned long>(query));

            if (enumerateAll || modeChanged) {
                query = chain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&device));
                if (SUCCEEDED(query) && device) {
                    const LUID adapter = device->GetAdapterLuid();
                    Log("SWAPCHAIN_DEVICE present=%llu phase=%s reason=%s device=%p adapter_luid=%08lX:%08lX", present, phase,
                        modeChanged ? "mode_change" : "enumerate_all", device, static_cast<unsigned long>(adapter.HighPart), adapter.LowPart);
                } else Log("SWAPCHAIN_DEVICE_FAILED present=%llu phase=%s hr=0x%08lX", present, phase, static_cast<unsigned long>(query));
            }

            void* output = lastDlssOutput.load();
            Log("PRESENT_PATH present=%llu phase=%s engine_frame=%llu current_index=%u backbuffer=%p last_aa_call=%llu last_aa_present=%llu last_aa_engine_frame=%llu dlss_output=%p output_call=%llu output_present=%llu output_engine_frame=%llu output_is_backbuffer=%u",
                present, phase, engineFrame, currentIndex, currentBuffer,
                pathLastAaCall.load(), pathLastAaPresent.load(), pathLastAaEngineFrame.load(), output,
                lastDlssOutputCall.load(), lastDlssOutputPresent.load(), lastDlssOutputEngineFrame.load(),
                unsigned(output && currentBuffer && output == currentBuffer));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("SWAPCHAIN_QUERY_EXCEPTION present=%llu phase=%s exception=0x%08lX", present, phase, GetExceptionCode());
    }
    __try {
        if (device) device->Release();
        for (unsigned int i = 0; i < _countof(allBuffers); ++i) if (allBuffers[i]) allBuffers[i]->Release();
        if (currentBuffer) currentBuffer->Release();
        if (chain3) chain3->Release();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("SWAPCHAIN_RELEASE_EXCEPTION present=%llu phase=%s exception=0x%08lX", present, phase, GetExceptionCode());
    }
    SetLastError(saved);
}
