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



// RR Native P4: observe Control's own NGX parameter-population boundary.
// Static analysis of the hash-locked Steam build maps the DLSS parameter setup
// helper to d3d RVA 0x1CDF0 and its single call inside doAntiAliasing to RVA
// 0x1FDBB. We replace only that CALL with a near JMP to a tiny relay. The relay
// invokes Remedy's original helper with the untouched ABI/stack, snapshots the
// Control-owned NGX parameter object immediately AFTER setup, preserves the
// helper return value, and jumps back to the original continuation. No NGX Set
// calls are added and NVIDIA evaluation remains entirely game-owned.
inline constexpr size_t kNativeNgxSetupCallRva = 0x1FDBB;
inline constexpr size_t kNativeNgxSetupHelperRva = 0x1CDF0;
static unsigned char* rrNativeSetupCallsite = nullptr;
static void* rrNativeSetupHelper = nullptr;
static void* rrNativeSetupRelay = nullptr;
static std::atomic<unsigned long long> rrNativeSetupCount{0};
static std::atomic<unsigned int> rrNativePostSetupSnapshotCount{0};

static void CaptureNativeRRNgxPostSetup(unsigned long long aaCall,
                                        unsigned long long setupOrdinal) noexcept {
    if (!ngxGetResource || !ngxGetInt || !ngxGetUInt) return;
    unsigned int current = rrNativePostSetupSnapshotCount.load();
    if (current >= 8 || aaCall < 120 || (aaCall % 120) != 0) return;
    current = rrNativePostSetupSnapshotCount.fetch_add(1) + 1;
    if (current > 8) return;

    const DWORD saved = GetLastError();
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = base ? *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva) : nullptr;
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        if (!parameters) {
            Log("RR_NATIVE_POST_SETUP_UNAVAILABLE aa_call=%llu setup=%llu snapshot=%u context=%p parameters=%p",
                aaCall, setupOrdinal, current, context, parameters);
            SetLastError(saved);
            return;
        }

        Log("RR_NATIVE_POST_SETUP_BEGIN aa_call=%llu setup=%llu snapshot=%u context=%p parameters=%p mode=read_only hot_hooks=0 set_calls=0",
            aaCall, setupOrdinal, current, context, parameters);

        const char* resourceKeys[] = {
            "Color", "Output", "Depth", "MotionVectors", "ExposureTexture",
            "DLSS.Input.DiffuseAlbedo", "DLSS.Input.SpecularAlbedo",
            "GBuffer.Normals", "GBuffer.Roughness", "GBuffer.Albedo",
            "GBuffer.DiffuseAlbedo", "GBuffer.SpecularAlbedo", "GBuffer.IndirectAlbedo",
            "GBuffer.SpecularMvec", "GBuffer.DisocclusionMask", "GBuffer.Emissive",
            "DLSSD.ReflectedAlbedo", "DLSSD.DiffuseHitDistance", "DLSSD.SpecularHitDistance",
            "DLSSD.DiffuseRayDirection", "DLSSD.SpecularRayDirection",
            "DLSSD.DiffuseRayDirectionHitDistance", "DLSSD.SpecularRayDirectionHitDistance",
            "DLSSD.ColorBeforeParticles", "DLSSD.ColorAfterParticles",
            "DLSSD.ColorBeforeTransparency", "DLSSD.ColorAfterTransparency",
            "DLSSD.ColorBeforeFog", "DLSSD.ColorAfterFog",
            "DLSSD.ScreenSpaceSubsurfaceScatteringGuide", "DLSSD.ScreenSpaceRefractionGuide",
            "DLSSD.DepthOfFieldGuide", "DLSSD.ResponsivityMask", "DLSSD.Alpha", "DLSSD.OutputAlpha"
        };

        unsigned int resourceHits = 0;
        unsigned int rrHits = 0;
        for (const char* key : resourceKeys) {
            ID3D12Resource* resource = nullptr;
            const unsigned int status = ngxGetResource(parameters, key, &resource);
            if (status == 1 && resource) {
                ++resourceHits;
                if (strcmp(key, "Color") && strcmp(key, "Output") && strcmp(key, "Depth") &&
                    strcmp(key, "MotionVectors") && strcmp(key, "ExposureTexture")) ++rrHits;
                const D3D12_RESOURCE_DESC desc = resource->GetDesc();
                Log("RR_NATIVE_POST_SETUP_RESOURCE aa_call=%llu setup=%llu snapshot=%u key=%s status=0x%08X resource=%p dimension=%u width=%llu height=%u array=%u mips=%u format=%u samples=%u flags=%u",
                    aaCall, setupOrdinal, current, key, status, resource, unsigned(desc.Dimension),
                    desc.Width, desc.Height, unsigned(desc.DepthOrArraySize), unsigned(desc.MipLevels),
                    unsigned(desc.Format), desc.SampleDesc.Count, unsigned(desc.Flags));
            } else {
                Log("RR_NATIVE_POST_SETUP_RESOURCE_EMPTY aa_call=%llu setup=%llu snapshot=%u key=%s status=0x%08X resource=%p",
                    aaCall, setupOrdinal, current, key, status, resource);
            }
        }

        const char* intKeys[] = {
            "Reset", "DLSS.Feature.Create.Flags", "DLSS.Denoise.Mode", "DLSS.Roughness.Mode",
            "DLSS.Use.HW.Depth", "RayReconstruction.Hint.Render.Preset.DLAA",
            "RayReconstruction.Hint.Render.Preset.Quality", "RayReconstruction.Hint.Render.Preset.Balanced",
            "RayReconstruction.Hint.Render.Preset.Performance", "RayReconstruction.Hint.Render.Preset.UltraPerformance",
            "RayReconstruction.Hint.Render.Preset.UltraQuality"
        };
        unsigned int intHits = 0;
        for (const char* key : intKeys) {
            int value = 0;
            const unsigned int status = ngxGetInt(parameters, key, &value);
            if (status == 1) {
                ++intHits;
                Log("RR_NATIVE_POST_SETUP_INT aa_call=%llu setup=%llu snapshot=%u key=%s status=0x%08X value=%d",
                    aaCall, setupOrdinal, current, key, status, value);
            } else {
                unsigned int uvalue = 0;
                const unsigned int ustatus = ngxGetUInt(parameters, key, &uvalue);
                if (ustatus == 1) {
                    ++intHits;
                    Log("RR_NATIVE_POST_SETUP_UINT aa_call=%llu setup=%llu snapshot=%u key=%s status=0x%08X value=%u int_status=0x%08X",
                        aaCall, setupOrdinal, current, key, ustatus, uvalue, status);
                } else {
                    Log("RR_NATIVE_POST_SETUP_INT_EMPTY aa_call=%llu setup=%llu snapshot=%u key=%s int_status=0x%08X uint_status=0x%08X",
                        aaCall, setupOrdinal, current, key, status, ustatus);
                }
            }
        }

        Log("RR_NATIVE_POST_SETUP_SUMMARY aa_call=%llu setup=%llu snapshot=%u resource_hits=%u rr_resource_hits=%u int_hits=%u rr_eval=0 ngx_writes=0 denoiser_bypass=0",
            aaCall, setupOrdinal, current, resourceHits, rrHits, intHits);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("RR_NATIVE_POST_SETUP_EXCEPTION aa_call=%llu setup=%llu snapshot=%u exception=0x%08lX",
            aaCall, setupOrdinal, current, GetExceptionCode());
    }
    SetLastError(saved);
}

static void NativeRRNgxPostSetupProbe() noexcept {
    const unsigned long long setupOrdinal = rrNativeSetupCount.fetch_add(1) + 1;
    const unsigned long long aaCall = aaCount.load();
    CaptureNativeRRNgxPostSetup(aaCall, setupOrdinal);
}

static void EmitRelayByte(unsigned char*& p, unsigned char v) noexcept { *p++ = v; }
static void EmitRelayU64(unsigned char*& p, uint64_t v) noexcept { memcpy(p, &v, sizeof(v)); p += sizeof(v); }

static bool PrepareNativeNgxPostSetupHook(HMODULE d3d) noexcept {
    if (!d3d || !originalAA) return false;
    auto base = reinterpret_cast<unsigned char*>(d3d);
    auto call = base + kNativeNgxSetupCallRva;
    auto expectedTarget = base + kNativeNgxSetupHelperRva;

    if (call[0] != 0xE8) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=opcode_mismatch call_rva=0x%zX opcode=0x%02X",
            kNativeNgxSetupCallRva, unsigned(call[0]));
        return false;
    }
    int32_t originalRel = 0;
    memcpy(&originalRel, call + 1, sizeof(originalRel));
    auto resolvedTarget = call + 5 + originalRel;
    if (resolvedTarget != expectedTarget) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=target_mismatch call_rva=0x%zX resolved=%p expected=%p",
            kNativeNgxSetupCallRva, resolvedTarget, expectedTarget);
        return false;
    }

    rrNativeSetupCallsite = call;
    rrNativeSetupHelper = resolvedTarget;
    rrNativeSetupRelay = AllocateExecutableRelayNear(call);
    if (!rrNativeSetupRelay) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=relay_allocation_failed callsite=%p", call);
        return false;
    }
    if (!Rel32Fits(call + 5, rrNativeSetupRelay)) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=relay_out_of_range callsite=%p relay=%p", call, rrNativeSetupRelay);
        VirtualFree(rrNativeSetupRelay, 0, MEM_RELEASE);
        rrNativeSetupRelay = nullptr;
        return false;
    }

    // The callsite is changed from CALL helper to JMP relay. Entering via JMP
    // preserves the exact stack layout the original helper expects. The relay:
    //   call original helper
    //   preserve RAX/XMM0 return values
    //   call the no-argument read-only probe with proper x64 shadow/alignment
    //   restore return values
    //   jump back to callsite+5
    auto out = reinterpret_cast<unsigned char*>(rrNativeSetupRelay);
    unsigned char* w = out;
    // mov r11, originalHelper
    EmitRelayByte(w, 0x49); EmitRelayByte(w, 0xBB); EmitRelayU64(w, reinterpret_cast<uint64_t>(rrNativeSetupHelper));
    // call r11
    EmitRelayByte(w, 0x41); EmitRelayByte(w, 0xFF); EmitRelayByte(w, 0xD3);
    // sub rsp, 40h (32-byte shadow + return preservation, keeps pre-call alignment)
    EmitRelayByte(w, 0x48); EmitRelayByte(w, 0x83); EmitRelayByte(w, 0xEC); EmitRelayByte(w, 0x40);
    // mov [rsp+20h], rax
    const unsigned char saveRax[] = {0x48,0x89,0x44,0x24,0x20}; memcpy(w, saveRax, sizeof(saveRax)); w += sizeof(saveRax);
    // movdqu [rsp+30h], xmm0
    const unsigned char saveXmm0[] = {0xF3,0x0F,0x7F,0x44,0x24,0x30}; memcpy(w, saveXmm0, sizeof(saveXmm0)); w += sizeof(saveXmm0);
    // mov r11, NativeRRNgxPostSetupProbe ; call r11
    EmitRelayByte(w, 0x49); EmitRelayByte(w, 0xBB); EmitRelayU64(w, reinterpret_cast<uint64_t>(&NativeRRNgxPostSetupProbe));
    EmitRelayByte(w, 0x41); EmitRelayByte(w, 0xFF); EmitRelayByte(w, 0xD3);
    // movdqu xmm0, [rsp+30h]
    const unsigned char restoreXmm0[] = {0xF3,0x0F,0x6F,0x44,0x24,0x30}; memcpy(w, restoreXmm0, sizeof(restoreXmm0)); w += sizeof(restoreXmm0);
    // mov rax, [rsp+20h]
    const unsigned char restoreRax[] = {0x48,0x8B,0x44,0x24,0x20}; memcpy(w, restoreRax, sizeof(restoreRax)); w += sizeof(restoreRax);
    // add rsp, 40h
    EmitRelayByte(w, 0x48); EmitRelayByte(w, 0x83); EmitRelayByte(w, 0xC4); EmitRelayByte(w, 0x40);
    // mov r11, continuation ; jmp r11
    EmitRelayByte(w, 0x49); EmitRelayByte(w, 0xBB); EmitRelayU64(w, reinterpret_cast<uint64_t>(call + 5));
    EmitRelayByte(w, 0x41); EmitRelayByte(w, 0xFF); EmitRelayByte(w, 0xE3);

    const size_t relaySize = size_t(w - out);
    FlushInstructionCache(GetCurrentProcess(), out, relaySize);
    DWORD relayOld = 0;
    if (!VirtualProtect(out, 0x1000, PAGE_EXECUTE_READ, &relayOld)) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=relay_protect_failed error=%lu", GetLastError());
        VirtualFree(rrNativeSetupRelay, 0, MEM_RELEASE);
        rrNativeSetupRelay = nullptr;
        return false;
    }

    const intptr_t delta64 = reinterpret_cast<unsigned char*>(rrNativeSetupRelay) - (call + 5);
    if (delta64 < INT32_MIN || delta64 > INT32_MAX) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=patch_delta_out_of_range");
        return false;
    }
    const int32_t rel32 = static_cast<int32_t>(delta64);
    unsigned char patch[5] = {0xE9,0,0,0,0};
    memcpy(patch + 1, &rel32, sizeof(rel32));
    DWORD old = 0;
    if (!VirtualProtect(call, sizeof(patch), PAGE_EXECUTE_READWRITE, &old)) {
        Log("RR_NATIVE_SETUP_HOOK_UNAVAILABLE reason=callsite_protect_failed error=%lu", GetLastError());
        return false;
    }
    memcpy(call, patch, sizeof(patch));
    FlushInstructionCache(GetCurrentProcess(), call, sizeof(patch));
    DWORD ignored = 0;
    VirtualProtect(call, sizeof(patch), old, &ignored);

    Log("RR_NATIVE_SETUP_HOOK_INSTALLED callsite=%p call_rva=0x%zX helper=%p helper_rva=0x%zX relay=%p relay_bytes=%zu mode=post_setup_read_only",
        call, kNativeNgxSetupCallRva, resolvedTarget, kNativeNgxSetupHelperRva,
        rrNativeSetupRelay, relaySize);
    return true;
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
