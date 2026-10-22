#pragma once
// Included inside native_lighting, after the capture hooks. The renderer scope
// holds one selection across ray production, history filtering and composition.
namespace rt_spot_runtime {
using CreateComputeFn = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
using PassFn = void (*)(void*, void*, void*, void*);
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
inline std::vector<unsigned char> rayBytes, lightBytes, resetBytes, historyBytes;
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
inline constexpr unsigned kRequiredComponents = 1|2|4|8|16;
struct Pass {
    uint64_t selectionState = 0;
    Mode mode = Mode::Native;
    bool requested = false, raySeen = false, fullRay = false, reset = false, composed = false;
};
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
        Trace("rt_spot_history filter_policy=mode_specific point_history=native spotlight_only=1 transition_reset=1 spot_samples=4 sample_pattern=r71_symmetric closest_hit_per_ray=1 mean_blocker=1 fixed_emitter_radius=1 bounded_penumbra=1 per_sample_endpoint=0 near_distance_cutoff=1 baseline=R89_mean_blocker point_samples=1");
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
    __try { originalPass(a,b,c,d); }
    __finally {
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
                Trace("rt_spot_sample_policy native_shadow_samples=%d replacement=R89_mean_blocker spot_samples=4 point_samples=1 engine_option_unchanged=1",nativeSamples);
            }
            if (pass.reset && pass.fullRay)
                Trace("rt_spot_test seq=%llu mode=%s raw=%u maps=%u expanded_casters=%u",
                    LiveSelection::Sequence(pass.selectionState),ModeName(pass.mode),
                    unsigned(pass.mode==Mode::RtSpotRaw),unsigned(pass.mode==Mode::RtSpotMapGuard),
                    unsigned(ExpandsSpotCasters(pass.mode)));
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
            PTReflectionLoadSidecar(L"rt_spot_raw_history.dxbc","RT_SPOT_RAW",rawHistoryBytes,sha,loaded) &&
            !strcmp(sha,"e5075b9dbbaacbb814b292d7bef94d477d1102a85c195c9515f76ca1af3651d0") &&
            PTReflectionLoadSidecar(L"rt_spot_raw_spatial0.dxbc","RT_SPOT_RAW",rawSpatialBytes[0],sha,loaded) &&
            !strcmp(sha,"42418aedec88af1380d6e57b0152e5186a8ac93ca8efc1d5a3837892fa8c5008") &&
            PTReflectionLoadSidecar(L"rt_spot_raw_spatial1.dxbc","RT_SPOT_RAW",rawSpatialBytes[1],sha,loaded) &&
            !strcmp(sha,"f84b55c052931f960ef28c78b270faa69884c69626a30f6b8666728c64511628") &&
            PTReflectionLoadSidecar(L"rt_spot_full.dxil","RT_SPOT_RAY",rayBytes,sha,loaded) &&
            !strcmp(sha,"39f75f7b953389d233ea6115bc2fb374fcf897dab023079aa9a3383d1807d98d") &&
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
    if (!desc || !native || (desc->CS.BytecodeLength != 50752 && desc->CS.BytecodeLength != 4868 && desc->CS.BytecodeLength != 12360)) return;
    unsigned kind = Matches(desc->CS,kLightSha) ? kLightComponent : Matches(desc->CS,kTemporalSha) ? kResetComponent : 0;
    int spatialIndex = -1;
    for (int i=0;i<2;++i) if (Matches(desc->CS,kSpatialSha[i])) {
        kind = kSpatialComponents[i]; spatialIndex = i; break;
    }
    if (!kind) return;
    Install();
    if (!filesReady) return;
    auto custom = *desc;
    const auto& bytes = spatialIndex >= 0 ? spatialBytes[spatialIndex] : kind == kLightComponent ? lightBytes : resetBytes;
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
    if (SUCCEEDED(hr) && (kind == kResetComponent || spatialIndex >= 0)) {
        const auto& raw = spatialIndex >= 0 ? rawSpatialBytes[spatialIndex] : rawHistoryBytes;
        custom.CS = {raw.data(),raw.size()};
        ID3D12PipelineState* variantRaw = nullptr;
        hr = create(device,&custom,IID_PPV_ARGS(&variantRaw));
        if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(kRawVariant,variantRaw);
        if (variantRaw) variantRaw->Release();
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
    if (!active || !native) return nullptr;
    unsigned kind = 0; UINT size = sizeof(kind);
    if (FAILED(native->GetPrivateData(kKind,&size,&kind))) return nullptr;
    const bool spatial = kind == kSpatialComponents[0] || kind == kSpatialComponents[1];
    const bool replace = (kind == kLightComponent || spatial) ? active->fullRay : kind == kResetComponent && active->raySeen && (active->reset || active->fullRay);
    if (!replace) return nullptr;
    ID3D12PipelineState* alternate = nullptr; size = sizeof(alternate);
    if (kind == kLightComponent && active->mode == Mode::RtSpotMapGuard) {
        active->composed = true;
        native->AddRef();
        return native;
    }
    const bool raw = active->fullRay && active->mode == Mode::RtSpotRaw &&
        (spatial || kind == kResetComponent);
    const GUID& variant = raw ? kRawVariant : kind == kResetComponent && !active->reset ? kHistoryVariant : kVariant;
    if (FAILED(native->GetPrivateData(variant,&size,&alternate)) || !alternate) return nullptr;
    if (kind == kLightComponent) active->composed = true;
    return alternate;
}
inline HRESULT CreateAugmented(ID3D12Device5* device, const D3D12_STATE_OBJECT_DESC* desc, REFIID iid, void** out) {
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
    try {
        std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        objects.reserve(desc->NumSubobjects+1);
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
                associations[i].NumExports=static_cast<UINT>(names[i].size()); associations[i].pExports=names[i].data();
            }
            associations[i].pSubobjectToAssociate=&objects[targetIndex]; objects[i].pDesc=&associations[i];
        }
        D3D12_EXPORT_DESC exportDesc{L"controlFullSpotRayGeneration",L"spotRayGeneration",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC extra{{replacementBytes->data(),replacementBytes->size()},1,&exportDesc};
        objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&extra});
        auto custom = *desc; custom.NumSubobjects=static_cast<UINT>(objects.size()); custom.pSubobjects=objects.data();
        HRESULT hr=original(device,&custom,iid,out);
        if (FAILED(hr)) { Trace("rt_spot_state_fallback hr=0x%08lX",static_cast<unsigned long>(hr)); return original(device,desc,iid,out); }
        ComPtr<ID3D12StateObject> state; ComPtr<ID3D12StateObjectProperties> properties;
        if (!out || !*out || FAILED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&state))) || FAILED(state.As(&properties))) return hr;
        const void* identifier=properties->GetShaderIdentifier(L"controlFullSpotRayGeneration");
        if (!identifier) return hr;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=64;buffer.Height=1;
        buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> record;
        HRESULT prepared=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&record));
        void* mapped=nullptr;D3D12_RANGE empty{0,0};
        if (SUCCEEDED(prepared)) prepared=record->Map(0,&empty,&mapped);
        if (SUCCEEDED(prepared)) {
            memset(mapped,0,64);memcpy(mapped,identifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);record->Unmap(0,nullptr);
            prepared=state->SetPrivateDataInterface(kRecord,record.Get());
        }
        if (SUCCEEDED(prepared)) components.fetch_or(kRayComponent);
        RefreshReady();Trace("rt_spot_state ready=%u native_exports_retained=1 variant=%s replacement=R89_mean_blocker spot_samples=4 point_samples=1",unsigned(SUCCEEDED(prepared)),variantName);
        return hr;
    } catch (...) { return original(device,desc,iid,out); }
}
inline void OnState(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state) noexcept {
    if (active) list->SetPrivateDataInterface(kState,state);
}
inline bool Dispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if (!active || !desc) return false;
    ID3D12StateObject* state=nullptr;UINT size=sizeof(state);
    if (FAILED(list->GetPrivateData(kState,&size,&state)) || !state) return false;
    ID3D12Resource* record=nullptr;size=sizeof(record);
    const HRESULT found=state->GetPrivateData(kRecord,&size,&record);state->Release();
    if (FAILED(found) || !record) return false;
    active->raySeen=true;
    active->fullRay=active->requested && desc->Depth==1 && desc->RayGenerationShaderRecord.SizeInBytes==32;
    active->reset=previousFull!=active->fullRay || previousMode!=active->mode;
    if (active->fullRay) {
        auto custom=*desc;custom.RayGenerationShaderRecord={record->GetGPUVirtualAddress(),32};
        rt_shadow_capture::Lookup(list).dispatch(list,&custom);
    }
    record->Release();return active->fullRay;
}
} // namespace rt_spot_runtime
