#pragma once
// Included inside native_lighting, after the capture hooks. The renderer scope
// holds one selection across ray production, history filtering and composition.
namespace rt_spot_runtime {
#include "../build/rt_spot_open.h"
#include "../build/rt_spot_history_guard.h"
#include "../build/rt_spot_shadow_isolation.h"
#include "../build/rt_spot_authored_clip.h"
#include "../build/rt_spot_projection.h"
#include "../build/rt_spot_blocker.h"
#include "../build/rt_spot_hit_position.h"
#include "../build/rt_spot_band_colors.h"
using CreateComputeFn = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
using PassFn = void (*)(void*, void*, void*, void*);
inline constexpr char kSelectorSha[] = "a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f";
inline constexpr char kRaySha[] = "3c6bee7eea5b5758dcaf96b400bb7589b4af2031c17bf6ba5ce2343623ddb618";
inline constexpr char kMultiRaySha[] = "d5efe2c2219257e2a7b51bbd65a7fa3b80cdb56fb7e6444223f8b4d7ee45a1a9";
inline constexpr char kLightSha[] = "6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621";
inline constexpr char kTemporalSha[] = "feb37637481fed22a3cd4d256da613d1f61aec70fc11ff5e42aa2a585101fedb";
inline constexpr const char* kSpatialSha[] = {
    "584d4085443970868fe903d10bcdacb18c4c82490d08b742b5219648da437ab3",
    "e1a78e01b31559354cd8d474c22d117c444dd008a0067881d44836a23b0a459b"
};
inline const GUID kVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x01}};
inline const GUID kKind = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x02}};
inline const GUID kRecord = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x03}};
inline const GUID kState = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x04}};
inline const GUID kRawVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x06}};
inline const GUID kHistoryVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x05}};
inline const GUID kVisibilityVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x07}};
inline const GUID kLightIdsVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x08}};
inline const GUID kBandColorsVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0b}};
inline const GUID kOpenVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0a}};
inline const GUID kSelectedOffVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x09}};
inline const GUID kContactOffVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0c}};
inline const GUID kScreenOcclusionVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0d}};
inline const GUID kBentNormalVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0e}};
inline const GUID kLocalMapsVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x0f}};
inline const GUID kCorrectedNoAOVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x10}};
inline const GUID kGuardedHistoryVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x11}};
inline const GUID kCorrectedOpenVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x12}};
inline const GUID kMapPriorityVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x13}};
inline const GUID kMapProbeVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x14}};
inline const GUID kNativeBlendVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x15}};
inline const GUID& IsolationVariant(Mode mode) noexcept {
    if(mode==Mode::RoomMapProbe)return kMapProbeVariant;
    if(mode==Mode::RoomCorrectedMapPriority)return kMapPriorityVariant;
    if(mode==Mode::RoomCorrectedNoAO)return kCorrectedNoAOVariant;
    if(UsesDirectSurfaceNormals(mode))return kBentNormalVariant;
    switch(mode) {
    case Mode::RoomContactOff:return kContactOffVariant;
    case Mode::RoomScreenOcclusionOff:return kScreenOcclusionVariant;
    case Mode::RoomLocalMapsOff:return kLocalMapsVariant;
    default:return kVariant;
    }
}
// Optional diagnostics share ownership with the native PSO and never disable Default.
// Composition signal views override shading, while the earlier producer still
// uses IsolationVariant to retain corrected light selection.
inline const GUID& CompositionVariant(Mode mode) noexcept {
    if(mode==Mode::RoomNativeBlend)return kNativeBlendVariant;
    if(mode==Mode::RoomMapProbe || mode==Mode::RoomCorrectedMapPriority)return kBentNormalVariant;
    if(mode==Mode::RoomCorrectedOpen)return kCorrectedOpenVariant;
    if(IsCorrectedSignal(mode))return ShowsSpotLightIds(mode)?kLightIdsVariant:kVisibilityVariant;
    if(NeedsIsolationProducer(mode)||mode==Mode::RoomContactOff)return IsolationVariant(mode);
    if(mode==Mode::RoomBlocker||IsHitPositionMode(mode))return kBandColorsVariant;
    if(mode==Mode::RoomOpen)return kOpenVariant;
    if(mode==Mode::RtSpotMapGuard||mode==Mode::RoomSelectedOff)return kSelectedOffVariant;
    if(ShowsSpotVisibility(mode))return kVisibilityVariant;
    if(ShowsSpotLightIds(mode))return kLightIdsVariant;
    return kVariant;
}

inline void PrepareIsolation(ID3D12Device* device, D3D12_COMPUTE_PIPELINE_STATE_DESC desc,
    ID3D12PipelineState* owner, CreateComputeFn create, const unsigned char* bytes, size_t length,
    const GUID& key, std::atomic<bool>& ready, const char* label) noexcept {
    desc.CS={bytes,length}; desc.CachedPSO={};
    ID3D12PipelineState* alternate=nullptr;
    HRESULT result=create(device,&desc,IID_PPV_ARGS(&alternate));
    if(SUCCEEDED(result))result=owner->SetPrivateDataInterface(key,alternate);
    if(alternate)alternate->Release();
    ready.store(SUCCEEDED(result),std::memory_order_release);
    Trace("shadow_isolation_prepare input=%s ready=%u hr=0x%08lX",label,unsigned(SUCCEEDED(result)),static_cast<unsigned long>(result));
}
inline std::vector<unsigned char> visibilityBytes, lightIdsBytes, selectedOffBytes, selectionBytes;
inline std::vector<unsigned char> rayBytes, baselineRayBytes, lightBytes, resetBytes, historyBytes;
inline std::vector<unsigned char> spatialBytes[2], rawSpatialBytes[2], rawHistoryBytes;
inline SRWLOCK installLock = SRWLOCK_INIT;
inline std::atomic<unsigned> components{0}, reports{0};
inline std::atomic<bool> disabled{false};
inline RRAlbedoCallPatch passPatch{};
inline PassFn originalPass = nullptr;
inline bool filesAttempted = false;
inline std::atomic<bool> filesReady{false}, scopeReady{false};
inline constexpr unsigned kRayComponent = 1, kLightComponent = 2, kResetComponent = 4;
inline constexpr unsigned kSpatialComponents[] = {8,16};
inline constexpr unsigned kSelectorComponent = 32;
inline constexpr unsigned kRequiredComponents = 1|2|4|8|16|32;
struct Pass {
    uint64_t selectionState = 0;
    Mode mode = Mode::Native;
    bool requested = false, raySeen = false, fullRay = false, reset = false, composed = false;
};
inline constexpr UINT kRayRecordStride = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
inline constexpr unsigned kOptionalRayCount = 5; // X/Y/Z, authored clip, optional projection origin.
inline constexpr UINT kRayRecordBufferSize = (3 + kOptionalRayCount) * kRayRecordStride;
inline constexpr UINT RayRecordOffset(Mode mode) noexcept {
    if (mode==Mode::RoomProjectionOrigin) return 7*kRayRecordStride;
    if (UsesAuthoredClip(mode)) return 6 * kRayRecordStride;
    if (mode == Mode::RoomBlocker) return 2 * kRayRecordStride;
    if (IsHitPositionMode(mode)) return (3 + static_cast<UINT>(mode)-static_cast<UINT>(Mode::RoomHitX))*kRayRecordStride;
    return mode == Mode::RtSpotBaseline || mode == Mode::RtSpotMapGuard ? kRayRecordStride : 0;
}
inline constexpr bool RayRecordFits(Mode mode, UINT64 bytes) noexcept {
    return RayRecordOffset(mode)+D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES<=bytes;
}
inline thread_local Pass* active = nullptr;
inline thread_local bool previousFull = false;
inline thread_local Mode previousMode = Mode::Native;
// Requested mode alone is insufficient: fallback passes must retain RR's policy.
inline bool NeedsNativeShadowFiltering() noexcept {
    return active && active->fullRay;
}
inline void ReportShadowFiltering(bool fullRR, bool contact, bool temporal, int size, int step) {
    if (!NeedsNativeShadowFiltering()) return;
    struct FilterState {
        bool valid = false, fullRR = false, contact = false, temporal = false;
        int size = 0, step = 0;
    };
    static thread_local FilterState last;
    if (!last.valid || active->reset || last.fullRR != fullRR || last.contact != contact ||
        last.temporal != temporal || last.size != size || last.step != step) {
        Trace("rt_spot_filter full_rr=%u contact=%u native_preserved=1 temporal=%u spatial_size=%d spatial_step=%d",
            unsigned(fullRR),unsigned(contact),unsigned(temporal),size,step);
        Trace("rt_spot_history filter_policy=mode_specific point_history=native spotlight_only=1 transition_reset=1 spot_samples=4 sample_pattern=r92_radial_strata closest_hit_per_ray=1 mean_blocker=1 center_probe=%u bounded_penumbra=1 per_sample_endpoint=%u near_distance_cutoff=%u point_samples=1",
            unsigned(RayRecordOffset(active->mode)!=kRayRecordStride), unsigned(RayRecordOffset(active->mode)!=kRayRecordStride), unsigned(RayRecordOffset(active->mode)==kRayRecordStride));
        last = {true, fullRR, contact, temporal, size, step};
    }
}
inline void RefreshReady() noexcept {
    if (scopeReady && components.load() == kRequiredComponents && !disabled.load()) readyMask.fetch_or(kRtSpotReadyBit);
}
inline bool Matches(const D3D12_SHADER_BYTECODE& shader, const char* expected) noexcept {
    if (!shader.pShaderBytecode || !shader.BytecodeLength) return false;
    char sha[65]{};
    return PTReflectionSha256(static_cast<const unsigned char*>(shader.pShaderBytecode), shader.BytecodeLength, sha) && !strcmp(sha, expected);
}
inline void HookPass(void* a, void* b, void* c, void* d) {
    Pass pass;
    Pass* previous = active;
    pass.selectionState = selection.Read();
    pass.mode = LiveSelection::GetMode(pass.selectionState);
    pass.requested = !previous && !disabled.load() && components.load() == kRequiredComponents &&
        IsRtSpotMode(pass.mode);
    active = &pass;
    if(!previous && SupportsLightContextCapture(pass.mode))rt_spot_capture_runtime::BeginScope();
    __try { originalPass(a,b,c,d); }
    __finally {
        if(!previous)rt_spot_capture_runtime::EndScope();
        active = previous;
        if (!previous) {
            if (pass.fullRay && !pass.composed) {
                disabled.store(true); readyMask.fetch_and(~kRtSpotReadyBit);
                Trace("rt_spot_disabled reason=composition_not_observed");
            }
            if (pass.reset || reports.fetch_add(1) < 8)
                Trace("rt_spot_pass requested=%u rays=%u full=%u composed=%u reset=%u gpu_confirmed=0",
                    unsigned(pass.requested),unsigned(pass.raySeen),unsigned(pass.fullRay),unsigned(pass.composed),unsigned(pass.reset));
            if (pass.reset && pass.fullRay && verifiedRenderer) {
                const auto* base = reinterpret_cast<const unsigned char*>(verifiedRenderer);
                int nativeSamples = 0;
                memcpy(&nativeSamples,base+0x913c90,sizeof(nativeSamples));
                Trace("rt_spot_sample_policy native_shadow_samples=%d replacement=R97_center_probe_with_R94_baseline spot_samples=4 point_samples=1 engine_option_unchanged=1",nativeSamples);
            }
            if (pass.reset && pass.fullRay)
                Trace("rt_spot_test seq=%llu mode=%s raw=%u selected_off=%u expanded_casters=%u",
                    LiveSelection::Sequence(pass.selectionState),ModeName(pass.mode),
                    unsigned(UsesRawSpotVisibility(pass.mode)),unsigned(pass.mode==Mode::RtSpotMapGuard),
                    unsigned(ExpandsSpotCasters(pass.mode)));
            if (pass.reset && pass.fullRay && (ShowsSpotVisibility(pass.mode) || ShowsSpotLightIds(pass.mode)))
                Trace("rt_spot_debug mode=%s composition=signal_only invalid_light=magenta downstream_postprocessing=1",ModeName(pass.mode));
            if (pass.fullRay && pass.composed) selection.Observe(pass.selectionState,kDirectBit);
            previousFull = pass.fullRay;
            previousMode = pass.mode;
        }
    }
}
inline void Install() noexcept {
    AcquireSRWLockExclusive(&installLock);
    if (!filesAttempted) {
        filesAttempted = true;
        char sha[65]{}; bool loaded = false;
        filesReady =
            PTReflectionLoadSidecar(L"rt_spot_selected_off.dxbc","RT_SPOT_SELECTED_OFF",selectedOffBytes,sha,loaded) &&
            !strcmp(sha,"b5a2edadb8a7d7f5def335c3e8037f7f21df2cefa650eda0ec3622bd590ee819") &&
            PTReflectionLoadSidecar(L"rt_spot_selection.dxbc","RT_SPOT_SELECTION",selectionBytes,sha,loaded) &&
            !strcmp(sha,"4319b0bd2d0ad74b670c084d92b84bd7ceda2be433ccd3e60588d49bb66ddb9c") &&
            PTReflectionLoadSidecar(L"rt_spot_raw_history.dxbc","RT_SPOT_RAW",rawHistoryBytes,sha,loaded) &&
            !strcmp(sha,"e5075b9dbbaacbb814b292d7bef94d477d1102a85c195c9515f76ca1af3651d0") &&
            PTReflectionLoadSidecar(L"rt_spot_raw_spatial0.dxbc","RT_SPOT_RAW",rawSpatialBytes[0],sha,loaded) &&
            !strcmp(sha,"42418aedec88af1380d6e57b0152e5186a8ac93ca8efc1d5a3837892fa8c5008") &&
            PTReflectionLoadSidecar(L"rt_spot_raw_spatial1.dxbc","RT_SPOT_RAW",rawSpatialBytes[1],sha,loaded) &&
            !strcmp(sha,"f84b55c052931f960ef28c78b270faa69884c69626a30f6b8666728c64511628") &&
            PTReflectionLoadSidecar(L"rt_spot_visibility.dxbc","RT_SPOT_VISIBILITY",visibilityBytes,sha,loaded) &&
            !strcmp(sha,"d12f7d085b26b0a23a1271c8eb83f412540f5370c15704f9b596e1194998de6c") &&
            PTReflectionLoadSidecar(L"rt_spot_light_ids.dxbc","RT_SPOT_LIGHT_IDS",lightIdsBytes,sha,loaded) &&
            !strcmp(sha,"8c813b5bd51894f9d429439a49cc1715524103de499d306609e4a7c9475771f2") &&
            PTReflectionLoadSidecar(L"rt_spot_r94.dxil","RT_SPOT_BASELINE",baselineRayBytes,sha,loaded) &&
            !strcmp(sha,"c1f3f1b7b8845dd85224c6c41e7c8d10c6cd0fc793328e7168766563352707a5") &&
            PTReflectionLoadSidecar(L"rt_spot_full.dxil","RT_SPOT_RAY",rayBytes,sha,loaded) &&
            !strcmp(sha,"26688736d38b5ea056ae6bb37dc1a738334d94c784003a67957f3677f789a246") &&
            PTReflectionLoadSidecar(L"rt_spot_light.dxbc","RT_SPOT_LIGHT",lightBytes,sha,loaded) &&
            !strcmp(sha,"a844888cdd5478605574157b323cb3376369ae9517c4954bf38bee21b664ecd7") &&
            PTReflectionLoadSidecar(L"rt_spot_reset.dxbc","RT_SPOT_RESET",resetBytes,sha,loaded) &&
            !strcmp(sha,"3081daebf178680b5f7e117a836e4494520c0e760de2fc1ee49d3cfb3a8d089f") &&
            PTReflectionLoadSidecar(L"rt_spot_history.dxbc","RT_SPOT_HISTORY",historyBytes,sha,loaded) &&
            !strcmp(sha,"0c30a3cb345a5ccb7f5a10d639b611118f683d007ec4c4944453187c62928e91") &&
            PTReflectionLoadSidecar(L"rt_spot_spatial0.dxbc","RT_SPOT_SPATIAL0",spatialBytes[0],sha,loaded) &&
            !strcmp(sha,"fc1dc49d260b40bdbc11a0a0e843ad34b9cb164afad946b3808fa20fd988c69d") &&
            PTReflectionLoadSidecar(L"rt_spot_spatial1.dxbc","RT_SPOT_SPATIAL1",spatialBytes[1],sha,loaded) &&
            !strcmp(sha,"ef654dc4b4cd49f2811c9002f894a7cb240a348cbde2fa625fabfaebcafd7b7c");
    }
    if (filesReady && !scopeReady && verifiedRenderer) {
        auto* base = reinterpret_cast<unsigned char*>(verifiedRenderer);
        originalPass = reinterpret_cast<PassFn>(base + 0x23ed10);
        scopeReady = RRAlbedoPrepareCallPatch(base+0x12daf7, reinterpret_cast<void*>(originalPass),
            reinterpret_cast<void*>(&HookPass), &passPatch) && RRAlbedoExchangeCall(&passPatch,true);
        Trace("rt_spot_scope ready=%u renderer_verified=1",unsigned(scopeReady));
    }
    RefreshReady();
    ReleaseSRWLockExclusive(&installLock);
}
inline void ObserveCompute(ID3D12Device* device, const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
    IUnknown* native, CreateComputeFn create) noexcept {
    if (!desc || !native || (desc->CS.BytecodeLength != 71144 && desc->CS.BytecodeLength != 50752 && desc->CS.BytecodeLength != 4868 && desc->CS.BytecodeLength != 12360)) return;
    unsigned kind = Matches(desc->CS,kSelectorSha) ? kSelectorComponent : Matches(desc->CS,kLightSha) ? kLightComponent : Matches(desc->CS,kTemporalSha) ? kResetComponent : 0;
    int spatialIndex = -1;
    for (int i=0;i<2;++i) if (Matches(desc->CS,kSpatialSha[i])) {
        kind = kSpatialComponents[i]; spatialIndex = i; break;
    }
    if (!kind) return;
    Install();
    if (!filesReady) return;
    auto custom = *desc;
    const auto& bytes = kind == kSelectorComponent ? selectionBytes : spatialIndex >= 0 ? spatialBytes[spatialIndex] : kind == kLightComponent ? lightBytes : resetBytes;
    custom.CS = {bytes.data(),bytes.size()}; custom.CachedPSO = {};
    ID3D12PipelineState* variant = nullptr;
    HRESULT hr = create(device,&custom,IID_PPV_ARGS(&variant));
    ID3D12PipelineState* owner = nullptr;
    if (SUCCEEDED(hr)) hr = native->QueryInterface(IID_PPV_ARGS(&owner));
    if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(kVariant,variant);
    if (SUCCEEDED(hr)) hr = owner->SetPrivateData(kKind,sizeof(kind),&kind);
    if (SUCCEEDED(hr) && kind == kResetComponent) {
        // Both variants must exist before advertising this component as ready.
        custom.CS = {historyBytes.data(),historyBytes.size()};
        ID3D12PipelineState* history = nullptr;
        hr = create(device,&custom,IID_PPV_ARGS(&history));
        if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(kHistoryVariant,history);
        if (history) history->Release();
    }
    if(SUCCEEDED(hr) && kind==kResetComponent) {
        PrepareIsolation(device,*desc,owner,create,kGuardedHistoryShader,sizeof(kGuardedHistoryShader),kGuardedHistoryVariant,guardedHistoryReady,"guarded_spot_history");
    }
    if (SUCCEEDED(hr) && (kind == kResetComponent || spatialIndex >= 0)) {
        const auto& raw = spatialIndex >= 0 ? rawSpatialBytes[spatialIndex] : rawHistoryBytes;
        custom.CS = {raw.data(),raw.size()};
        ID3D12PipelineState* variantRaw = nullptr;
        hr = create(device,&custom,IID_PPV_ARGS(&variantRaw));
        if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(kRawVariant,variantRaw);
        if (variantRaw) variantRaw->Release();
    }
    if (SUCCEEDED(hr) && kind == kLightComponent) {
        const std::vector<unsigned char>* debugBytes[] = {&visibilityBytes, &lightIdsBytes, &selectedOffBytes};
        const GUID* debugKeys[] = {&kVisibilityVariant, &kLightIdsVariant, &kSelectedOffVariant};
        for (unsigned i=0; i<3 && SUCCEEDED(hr); ++i) {
            custom.CS = {debugBytes[i]->data(),debugBytes[i]->size()};
            ID3D12PipelineState* debug = nullptr;
            hr = create(device,&custom,IID_PPV_ARGS(&debug));
            if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(*debugKeys[i],debug);
            if (debug) debug->Release();
        }
    }
    if (SUCCEEDED(hr) && kind == kLightComponent) {
        custom.CS = {kSpotBandColorsShader,sizeof(kSpotBandColorsShader)};
        ID3D12PipelineState* bands = nullptr;
        HRESULT bandResult = create(device,&custom,IID_PPV_ARGS(&bands));
        if (SUCCEEDED(bandResult)) bandResult = owner->SetPrivateDataInterface(kBandColorsVariant,bands);
        if (bands) bands->Release();
        spotBandColorsReady.store(SUCCEEDED(bandResult),std::memory_order_release);
        Trace("rt_spot_band_colors ready=%u hr=0x%08lX",unsigned(SUCCEEDED(bandResult)),static_cast<unsigned long>(bandResult));
    }
    if constexpr (rt_spot_capture::GpuCaptureEnabled) {
        if(SUCCEEDED(hr) && kind==kLightComponent) {
            custom.CS={rt_spot_capture_gpu::kSpotCaptureSignalShader,sizeof(rt_spot_capture_gpu::kSpotCaptureSignalShader)};
            ID3D12PipelineState* signal=nullptr;
            HRESULT signalHr=create(device,&custom,IID_PPV_ARGS(&signal));
            if(SUCCEEDED(signalHr))signalHr=owner->SetPrivateDataInterface(rt_spot_capture_gpu::kSignalPipeline,signal);
            if(signal)signal->Release();
            if(SUCCEEDED(signalHr)) {
                custom.CS={rt_spot_capture_gpu::kSpotCaptureSignalBeforeShader,sizeof(rt_spot_capture_gpu::kSpotCaptureSignalBeforeShader)};
                signal=nullptr;
                signalHr=create(device,&custom,IID_PPV_ARGS(&signal));
                if(SUCCEEDED(signalHr))signalHr=owner->SetPrivateDataInterface(rt_spot_capture_gpu::kSignalBeforePipeline,signal);
                if(signal)signal->Release();
            }
            rt_spot_capture_runtime::signalReady.store(SUCCEEDED(signalHr));
            Trace("composition_capture_pipeline ready=%u hr=0x%08lX",unsigned(SUCCEEDED(signalHr)),static_cast<unsigned long>(signalHr));
        }
    }
    // Optional diagnostic failure must not disable the working spotlight path.
    if (SUCCEEDED(hr) && kind == kLightComponent) {
        custom.CS = {kSpotOpenShader,sizeof(kSpotOpenShader)};
        ID3D12PipelineState* open = nullptr;
        HRESULT openResult = create(device,&custom,IID_PPV_ARGS(&open));
        if (SUCCEEDED(openResult)) openResult = owner->SetPrivateDataInterface(kOpenVariant,open);
        if (open) open->Release();
        spotOpenReady.store(SUCCEEDED(openResult),std::memory_order_release);
        Trace("rt_spot_open ready=%u hr=0x%08lX",unsigned(SUCCEEDED(openResult)),static_cast<unsigned long>(openResult));
    }
    if(SUCCEEDED(hr) && kind==kLightComponent) {
        PrepareIsolation(device,*desc,owner,create,kNativeBlendShader,sizeof(kNativeBlendShader),kNativeBlendVariant,nativeBlendReady,"native_pcf_blend_composition");
        PrepareIsolation(device,*desc,owner,create,kCorrectedOpenShader,sizeof(kCorrectedOpenShader),kCorrectedOpenVariant,correctedOpenLightReady,"surface_normals_rt_spot_open");
        PrepareIsolation(device,*desc,owner,create,kCorrectedNoAOShader,sizeof(kCorrectedNoAOShader),kCorrectedNoAOVariant,correctedNoAOLightReady,"surface_normals_no_ao_composition");
        PrepareIsolation(device,*desc,owner,create,kBentNormalOffShader,sizeof(kBentNormalOffShader),kBentNormalVariant,bentNormalLightReady,"bent_normal_composition");
        PrepareIsolation(device,*desc,owner,create,kLocalMapsOffShader,sizeof(kLocalMapsOffShader),kLocalMapsVariant,localMapsLightReady,"local_maps_composition");
        PrepareIsolation(device,*desc,owner,create,kContactOffShader,sizeof(kContactOffShader),kContactOffVariant,contactOffReady,"contact_visibility");
        PrepareIsolation(device,*desc,owner,create,kScreenOcclusionOffShader,sizeof(kScreenOcclusionOffShader),kScreenOcclusionVariant,screenOcclusionLightReady,"screen_occlusion_composition");
    }
    if(SUCCEEDED(hr) && kind==kSelectorComponent) {
        PrepareIsolation(device,*desc,owner,create,kSelectorMapProbeShader,sizeof(kSelectorMapProbeShader),kMapProbeVariant,mapProbeReady,"selected_map_probe_producer");
        PrepareIsolation(device,*desc,owner,create,kSelectorMapPriorityShader,sizeof(kSelectorMapPriorityShader),kMapPriorityVariant,mapPrioritySelectorReady,"surface_normals_map_priority_producer");
        PrepareIsolation(device,*desc,owner,create,kSelectorCorrectedNoAOShader,sizeof(kSelectorCorrectedNoAOShader),kCorrectedNoAOVariant,correctedNoAOSelectorReady,"surface_normals_no_ao_producer");
        PrepareIsolation(device,*desc,owner,create,kSelectorBentNormalOffShader,sizeof(kSelectorBentNormalOffShader),kBentNormalVariant,bentNormalSelectorReady,"bent_normal_producer");
        PrepareIsolation(device,*desc,owner,create,kSelectorLocalMapsOffShader,sizeof(kSelectorLocalMapsOffShader),kLocalMapsVariant,localMapsSelectorReady,"local_maps_producer");
        PrepareIsolation(device,*desc,owner,create,kSelectorOcclusionOffShader,sizeof(kSelectorOcclusionOffShader),kScreenOcclusionVariant,screenOcclusionSelectorReady,"screen_occlusion_producer");
    }
    if (owner) owner->Release();
    if (variant) variant->Release();
    if (SUCCEEDED(hr)) components.fetch_or(kind);
    else { disabled.store(true); readyMask.fetch_and(~kRtSpotReadyBit); }
    RefreshReady();
    Trace("rt_spot_compute kind=%u ready=%u hr=0x%08lX",kind,unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr));
}
// Returns an AddRef'd alternate only within the verified renderer pass.
inline ID3D12PipelineState* SelectCompute(ID3D12PipelineState* native) noexcept {
    if (!native) return nullptr;
    unsigned kind = 0; UINT size = sizeof(kind);
    if (FAILED(native->GetPrivateData(kKind,&size,&kind))) return nullptr;
    // The producer runs before the scoped ray/filter/composition pass. Its
    // selected-ID exclusion and mapped accumulation remain self-consistent
    // even if a UI transition occurs before the later pass uses those IDs.
    if (kind == kSelectorComponent) {
        const auto state = selection.Read();
        const bool use = scopeReady && !disabled.load() && components.load()==kRequiredComponents &&
            UsesUnshadowedSpotSelection(LiveSelection::GetMode(state));
        static thread_local uint64_t lastSequence = ~uint64_t(0);
        if (lastSequence != LiveSelection::Sequence(state)) {
            Trace("rt_spot_selection seq=%llu policy=%s cpu_binding_only=1",LiveSelection::Sequence(state),
                use ? (LiveSelection::GetMode(state)==Mode::RoomCorrectedMapPriority?"map_visible_contribution":"unshadowed_contribution") : "native_r93");
            lastSequence = LiveSelection::Sequence(state);
        }
        if (!use) return nullptr;
        ID3D12PipelineState* alternate = nullptr;size=sizeof(alternate);
        const auto mode=LiveSelection::GetMode(state);
        const bool isolated=NeedsIsolationProducer(mode);
        const GUID& key=isolated?IsolationVariant(mode):kVariant;
        if (FAILED(native->GetPrivateData(key,&size,&alternate)) || !alternate) return nullptr;
        if(isolated) {
            selection.Observe(state,kGiBit); // producer evidence; composition supplies kDirectBit
            static thread_local uint64_t observed=~uint64_t(0);
            if(observed!=LiveSelection::Sequence(state)) {
                Trace("shadow_isolation_applied seq=%llu mode=%s producer=1 cpu_binding_only=1",LiveSelection::Sequence(state),ModeName(mode));
                observed=LiveSelection::Sequence(state);
            }
        }
        return alternate;
    }
    if (!active) return nullptr;
    const bool spatial = kind == kSpatialComponents[0] || kind == kSpatialComponents[1];
    const bool replace = (kind == kLightComponent || spatial) ? active->fullRay : kind == kResetComponent && active->raySeen && (active->reset || active->fullRay);
    if (!replace) return nullptr;
    ID3D12PipelineState* alternate = nullptr; size = sizeof(alternate);
    const bool raw = active->fullRay &&
        ((UsesRawSpotVisibility(active->mode) && (spatial || kind == kResetComponent)) ||
         (BypassesSpotHistory(active->mode) && kind == kResetComponent));
    const GUID& variant = kind == kLightComponent ? CompositionVariant(active->mode) :
        raw ? kRawVariant : kind == kResetComponent && !active->reset ?
        (UsesGuardedSpotHistory(active->mode)?kGuardedHistoryVariant:kHistoryVariant) : kVariant;
    if (FAILED(native->GetPrivateData(variant,&size,&alternate)) || !alternate) return nullptr;
    if(kind==kResetComponent) {
        static thread_local uint64_t reported=~uint64_t(0);
        const auto sequence=LiveSelection::Sequence(active->selectionState);
        if(reported!=sequence && !active->reset) {
            Trace("spot_history_applied seq=%llu mode=%s policy=%s cpu_binding_only=1",sequence,ModeName(active->mode),
                raw?"current_frame":UsesGuardedSpotHistory(active->mode)?"current_pixel_agreement_0.125":"neighborhood_clamp");
            reported=sequence;
        }
    }
    if (kind == kLightComponent) {
        active->composed = true;
        static thread_local uint64_t reported = ~uint64_t(0);
        const auto sequence = LiveSelection::Sequence(active->selectionState);
        if (reported != sequence) {
            Trace("spot_light_isolation seq=%llu mode=%s selector=%s composition=%s gpu_confirmed=0",
                sequence,ModeName(active->mode),active->mode==Mode::RoomCorrectedMapPriority?"map_priority":UsesUnshadowedSpotSelection(active->mode)?"unshadowed":"native",
                IsCorrectedSignal(active->mode)?(ShowsSpotLightIds(active->mode)?"corrected_selected_spot_ids":"corrected_selected_spot_visibility"):
                active->mode==Mode::RoomCorrectedOpen?"surface_normals_rt_spot_visibility_one":
                active->mode==Mode::RoomCorrectedNoAO?"surface_normals_screen_occlusion_yw_one":
                UsesDirectSurfaceNormals(active->mode)?"surface_normals":
                active->mode==Mode::RoomLocalMapsOff?"local_map_visibility_one":
                active->mode==Mode::RoomContactOff?"spot_point_contact_visibility_one":
                active->mode==Mode::RoomScreenOcclusionOff?"screen_occlusion_yw_one":
                active->mode==Mode::RoomOpen?"selected_visibility_one":
                active->mode==Mode::RoomSelectedOff?"selected_omitted":
                IsHitPositionMode(active->mode)?"hit_position_bands":
                active->mode==Mode::RoomBlocker?"blocker_bands":
                ShowsSpotVisibility(active->mode)?"visibility_signal":
                ShowsSpotLightIds(active->mode)?"light_ids":
                UsesRawSpotVisibility(active->mode)?"raw_rt":"filtered_rt");
            reported = sequence;
        }
    }
    return alternate;
}
inline HRESULT CreateAugmentedInternal(ID3D12Device5* device, const D3D12_STATE_OBJECT_DESC* desc, REFIID iid, void** out, bool blocker, bool position=true, bool projection=true) {
    projection=projection&&position;
    const unsigned optionalCount=position?(projection?kOptionalRayCount:kOptionalRayCount-1):0;
    using Microsoft::WRL::ComPtr;
    auto original = rt_shadow_capture::originalState;
    if (!desc || desc->Type != D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE || !filesReady)
        return original(device,desc,iid,out);
    const std::vector<unsigned char>* replacementBytes = nullptr;
    const char* variantName = nullptr;
    for (UINT i=0;i<desc->NumSubobjects;++i) {
        const auto& object = desc->pSubobjects[i];
        if (object.Type != D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY || !object.pDesc) continue;
        const auto& shader = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc)->DXILLibrary;
        if (Matches(shader,kRaySha)) { replacementBytes=&rayBytes; variantName="single"; }
        // The single-library raygen uses a subset of the configurable library's
        // bindings. Both native variants therefore run the identical R71 binary.
        // Keep the native hit/miss tables and use their original handlers.
        else if (Matches(shader,kMultiRaySha)) { replacementBytes=&rayBytes; variantName="configurable"; }
    }
    if (!replacementBytes) return original(device,desc,iid,out);
    rt_spot_capture_audit::DescribeState(desc);
    try {
        std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        objects.reserve(desc->NumSubobjects+3+kOptionalRayCount);
        constexpr LPCWSTR optionalRayNames[]={L"controlHitXRayGeneration",L"controlHitYRayGeneration",L"controlHitZRayGeneration",L"controlAuthoredClipRayGeneration",L"controlProjectionRayGeneration"};
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        std::vector<std::vector<LPCWSTR>> names(desc->NumSubobjects);
        for (UINT i=0;i<desc->NumSubobjects;++i) if (objects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
            associations[i] = *static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
            UINT targetIndex = desc->NumSubobjects;
            for (UINT j=0;j<desc->NumSubobjects;++j)
                if (associations[i].pSubobjectToAssociate == desc->pSubobjects+j) targetIndex=j;
            if (targetIndex == desc->NumSubobjects) return original(device,desc,iid,out);
            const auto type = objects[targetIndex].Type;
            bool rayAssociation = false;
            for (UINT j=0;j<associations[i].NumExports;++j)
                rayAssociation |= wcscmp(associations[i].pExports[j],L"spotRayGeneration") == 0;
            if (rayAssociation && (type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE || type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG)) {
                names[i].assign(associations[i].pExports,associations[i].pExports+associations[i].NumExports);
                names[i].push_back(L"controlFullSpotRayGeneration");
                names[i].push_back(L"controlR94SpotRayGeneration");
                if (blocker) names[i].push_back(L"controlBlockerSpotRayGeneration");
                if (position) for(unsigned j=0;j<optionalCount;++j) names[i].push_back(optionalRayNames[j]);
                associations[i].NumExports=static_cast<UINT>(names[i].size()); associations[i].pExports=names[i].data();
            }
            associations[i].pSubobjectToAssociate=&objects[targetIndex]; objects[i].pDesc=&associations[i];
        }
        D3D12_EXPORT_DESC exportDesc{L"controlFullSpotRayGeneration",L"spotRayGeneration",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC extra{{replacementBytes->data(),replacementBytes->size()},1,&exportDesc};
        objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&extra});
        D3D12_EXPORT_DESC baselineExport{L"controlR94SpotRayGeneration",L"spotRayGeneration",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC baselineLibrary{{baselineRayBytes.data(),baselineRayBytes.size()},1,&baselineExport};
        objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&baselineLibrary});
        D3D12_EXPORT_DESC blockerExport{L"controlBlockerSpotRayGeneration",L"spotRayGeneration",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC blockerLibrary{{kSpotBlockerShader,sizeof(kSpotBlockerShader)},1,&blockerExport};
        if (blocker) objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&blockerLibrary});
        const D3D12_SHADER_BYTECODE optionalRayBytes[]={{kSpotHitXShader,sizeof(kSpotHitXShader)},{kSpotHitYShader,sizeof(kSpotHitYShader)},{kSpotHitZShader,sizeof(kSpotHitZShader)},{kSpotAuthoredClipShader,sizeof(kSpotAuthoredClipShader)},{kSpotProjectionShader,sizeof(kSpotProjectionShader)}};
        static_assert(sizeof(optionalRayNames)/sizeof(*optionalRayNames)==kOptionalRayCount);
        static_assert(sizeof(optionalRayBytes)/sizeof(*optionalRayBytes)==kOptionalRayCount);
        D3D12_EXPORT_DESC optionalRayExports[kOptionalRayCount]{};
        D3D12_DXIL_LIBRARY_DESC optionalRayLibraries[kOptionalRayCount]{};
        if(position) for(unsigned i=0;i<optionalCount;++i){
            optionalRayExports[i]={optionalRayNames[i],L"spotRayGeneration",D3D12_EXPORT_FLAG_NONE};
            optionalRayLibraries[i]={optionalRayBytes[i],1,&optionalRayExports[i]};
            objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&optionalRayLibraries[i]});
        }
        auto custom = *desc; custom.NumSubobjects=static_cast<UINT>(objects.size()); custom.pSubobjects=objects.data();
        HRESULT hr=original(device,&custom,iid,out);
        if(FAILED(hr)&&projection){
            spotProjectionReady.store(false,std::memory_order_release);
            Trace("rt_spot_projection unavailable=1 hr=0x%08lX accepted_shadow_retry=1",static_cast<unsigned long>(hr));
            return CreateAugmentedInternal(device,desc,iid,out,blocker,position,false);
        }
        if(FAILED(hr)&&position){
            spotHitPositionReady.store(false,std::memory_order_release);spotAuthoredClipReady.store(false,std::memory_order_release);
            Trace("rt_spot_hit_position unavailable=1 hr=0x%08lX baseline_retry=1",static_cast<unsigned long>(hr));
            return CreateAugmentedInternal(device,desc,iid,out,blocker,false);
        }
        if (FAILED(hr) && blocker) {
            spotBlockerReady.store(false,std::memory_order_release);
            Trace("rt_spot_blocker unavailable=1 hr=0x%08lX baseline_retry=1",static_cast<unsigned long>(hr));
            return CreateAugmentedInternal(device,desc,iid,out,false,false);
        }
        if (FAILED(hr)) { Trace("rt_spot_state_fallback hr=0x%08lX",static_cast<unsigned long>(hr)); return original(device,desc,iid,out); }
        ComPtr<ID3D12StateObject> state; ComPtr<ID3D12StateObjectProperties> properties;
        if (!out || !*out || FAILED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&state))) || FAILED(state.As(&properties))) return hr;
        // Diagnostic DXR compilation must not be a prerequisite for gameplay.
        if constexpr (rt_spot_capture::GpuCaptureEnabled) {
            const auto captureResult=rt_spot_hit_pipeline::Prepare(device,desc,state.Get());
            if(FAILED(captureResult))Trace("rt_hit_pipeline_unavailable hr=0x%08lX gameplay_preserved=1",static_cast<unsigned long>(captureResult));
            if(SUCCEEDED(captureResult))for(UINT i=0;i<desc->NumSubobjects;++i)
                if(desc->pSubobjects[i].Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE)
                    rt_spot_capture_gpu::PreparePipeline(device,state.Get(),static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(desc->pSubobjects[i].pDesc)->pGlobalRootSignature);
        }
        const void* identifier=properties->GetShaderIdentifier(L"controlFullSpotRayGeneration");
        const void* baselineIdentifier=properties->GetShaderIdentifier(L"controlR94SpotRayGeneration");
        const void* blockerIdentifier=blocker?properties->GetShaderIdentifier(L"controlBlockerSpotRayGeneration"):nullptr;
        if (!identifier || !baselineIdentifier) return hr;
        const bool blockerAvailable=blockerIdentifier!=nullptr;
        const void* optionalRayIdentifiers[kOptionalRayCount]{};
        bool optionalRaysAvailable=position;
        if(position)for(unsigned i=0;i<optionalCount;++i){optionalRayIdentifiers[i]=properties->GetShaderIdentifier(optionalRayNames[i]);optionalRaysAvailable &= optionalRayIdentifiers[i]!=nullptr;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=optionalRaysAvailable?kRayRecordBufferSize:(blockerAvailable?3:2)*kRayRecordStride;buffer.Height=1;
        buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> record;
        HRESULT prepared=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&record));
        void* mapped=nullptr;D3D12_RANGE empty{0,0};
        if (SUCCEEDED(prepared)) prepared=record->Map(0,&empty,&mapped);
        if (SUCCEEDED(prepared)) {
            memset(mapped,0,static_cast<size_t>(buffer.Width));
            memcpy(mapped,identifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            memcpy(static_cast<unsigned char*>(mapped)+kRayRecordStride,baselineIdentifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            if (blockerAvailable) memcpy(static_cast<unsigned char*>(mapped)+2*kRayRecordStride,blockerIdentifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            if(optionalRaysAvailable)for(unsigned i=0;i<optionalCount;++i)memcpy(static_cast<unsigned char*>(mapped)+(3+i)*kRayRecordStride,optionalRayIdentifiers[i],D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            record->Unmap(0,nullptr);
            prepared=state->SetPrivateDataInterface(kRecord,record.Get());
        }
        spotProjectionReady.store(SUCCEEDED(prepared)&&optionalRaysAvailable&&projection,std::memory_order_release);
        spotAuthoredClipReady.store(SUCCEEDED(prepared)&&optionalRaysAvailable,std::memory_order_release);
        Trace("rt_spot_authored_clip ready=%u record_offset=384 per_light_plane=1 receiver_origin_preserved=1 extra_rays=0",unsigned(SUCCEEDED(prepared)&&optionalRaysAvailable));
        spotHitPositionReady.store(SUCCEEDED(prepared)&&optionalRaysAvailable,std::memory_order_release);
        Trace("rt_spot_hit_position ready=%u offsets=192_256_320 native_hit_distance=1 geometry_id_available=0",unsigned(SUCCEEDED(prepared)&&optionalRaysAvailable));
        spotBlockerReady.store(SUCCEEDED(prepared)&&blockerAvailable,std::memory_order_release);
        Trace("rt_spot_blocker ready=%u full_length=1 bands=clear_1_tight_.75_wide_.5_earlier_0",unsigned(SUCCEEDED(prepared)&&blockerAvailable));
        if (SUCCEEDED(prepared)) components.fetch_or(kRayComponent);
        RefreshReady();Trace("rt_spot_state ready=%u native_exports_retained=1 variant=%s replacement=R97_center_probe_with_R94_baseline spot_samples=4 point_samples=1",unsigned(SUCCEEDED(prepared)),variantName);
        return hr;
    } catch (...) {
        if(projection){spotProjectionReady.store(false,std::memory_order_release);return CreateAugmentedInternal(device,desc,iid,out,blocker,position,false);}
        if(position){spotHitPositionReady.store(false,std::memory_order_release);spotAuthoredClipReady.store(false,std::memory_order_release);return CreateAugmentedInternal(device,desc,iid,out,blocker,false);}
        if (blocker) {
            spotBlockerReady.store(false,std::memory_order_release);
            return CreateAugmentedInternal(device,desc,iid,out,false,false);
        }
        return original(device,desc,iid,out);
    }
}
inline HRESULT CreateAugmented(ID3D12Device5* device, const D3D12_STATE_OBJECT_DESC* desc, REFIID iid, void** out) {
    return CreateAugmentedInternal(device,desc,iid,out,true);
}
inline void OnState(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state) noexcept {
    if (active) list->SetPrivateDataInterface(kState,state);
}
inline bool Dispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if (!active || !desc) return false;
    ID3D12StateObject* state=nullptr;UINT size=sizeof(state);
    if (FAILED(list->GetPrivateData(kState,&size,&state)) || !state) return false;
    ID3D12Resource* record=nullptr;size=sizeof(record);
    const HRESULT found=state->GetPrivateData(kRecord,&size,&record);
    if (SUCCEEDED(found) && record) {
        rt_spot_table_audit::Inspect(list,state,desc);
    }
    if (FAILED(found) || !record) {state->Release();return false;}
    active->raySeen=true;
    active->fullRay=active->requested && desc->Depth==1 && desc->RayGenerationShaderRecord.SizeInBytes==32 &&
        RayRecordFits(active->mode,record->GetDesc().Width);
    active->reset=previousFull!=active->fullRay || previousMode!=active->mode;
    if (active->fullRay) {
        if (active->reset) Trace("rt_spot_ray_route mode=%s record_offset=%u blocker_bands=%u raw=%u gpu_confirmed=0",ModeName(active->mode),RayRecordOffset(active->mode),unsigned(active->mode==Mode::RoomBlocker),unsigned(UsesRawSpotVisibility(active->mode)));
        auto custom=*desc;custom.RayGenerationShaderRecord={record->GetGPUVirtualAddress() + RayRecordOffset(active->mode),32};
        rt_shadow_capture::Lookup(list).dispatch(list,&custom);
        if(SupportsLightContextCapture(active->mode))rt_spot_capture_runtime::Record(list,state,desc);
    }
    state->Release();record->Release();return active->fullRay;
}
} // namespace rt_spot_runtime
