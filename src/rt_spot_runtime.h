#pragma once
// Included inside native_lighting, after the capture hooks. The renderer scope
// holds one selection across ray production, history filtering and composition.
namespace rt_spot_runtime {
using CreateComputeFn = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
using PassFn = void (*)(void*, void*, void*, void*);
inline constexpr char kRaySha[] = "3c6bee7eea5b5758dcaf96b400bb7589b4af2031c17bf6ba5ce2343623ddb618";
inline constexpr char kLightSha[] = "6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621";
inline constexpr char kTemporalSha[] = "feb37637481fed22a3cd4d256da613d1f61aec70fc11ff5e42aa2a585101fedb";
inline const GUID kVariant = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x01}};
inline const GUID kKind = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x02}};
inline const GUID kRecord = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x03}};
inline const GUID kState = {0xe631a601,0x6651,0x4911,{0x80,0x90,0x11,0x71,0x14,0x28,0x30,0x04}};
inline std::vector<unsigned char> rayBytes, lightBytes, resetBytes;
inline SRWLOCK installLock = SRWLOCK_INIT;
inline std::atomic<unsigned> components{0}, reports{0};
inline std::atomic<bool> disabled{false};
inline RRAlbedoCallPatch passPatch{};
inline PassFn originalPass = nullptr;
inline bool filesAttempted = false;
inline std::atomic<bool> filesReady{false}, scopeReady{false};
inline constexpr unsigned kRayComponent = 1, kLightComponent = 2, kResetComponent = 4;
struct Pass {
    uint64_t selectionState = 0;
    bool requested = false, raySeen = false, fullRay = false, reset = false, composed = false;
};
inline thread_local Pass* active = nullptr;
inline thread_local bool previousFull = false;
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
        last = {true, fullRR, contact, temporal, size, step};
    }
}
inline void RefreshReady() noexcept {
    if (scopeReady && components.load() == 7 && !disabled.load()) readyMask.fetch_or(kRtSpotReadyBit);
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
    pass.requested = !previous && !disabled.load() && components.load() == 7 &&
        LiveSelection::GetMode(pass.selectionState) == Mode::RtSpot;
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
            if (pass.fullRay && pass.composed) selection.Observe(pass.selectionState,kDirectBit);
            previousFull = pass.fullRay;
        }
    }
}
inline void Install() noexcept {
    AcquireSRWLockExclusive(&installLock);
    if (!filesAttempted) {
        filesAttempted = true;
        char sha[65]{}; bool loaded = false;
        filesReady = PTReflectionLoadSidecar(L"rt_spot_full.dxil","RT_SPOT_RAY",rayBytes,sha,loaded) &&
            !strcmp(sha,"1fc49d54946bf53e7abe5a23cf1e28c229ebba843e517c68f6b2ce2ab9762e40") &&
            PTReflectionLoadSidecar(L"rt_spot_light.dxbc","RT_SPOT_LIGHT",lightBytes,sha,loaded) &&
            !strcmp(sha,"a844888cdd5478605574157b323cb3376369ae9517c4954bf38bee21b664ecd7") &&
            PTReflectionLoadSidecar(L"rt_spot_reset.dxbc","RT_SPOT_RESET",resetBytes,sha,loaded) &&
            !strcmp(sha,"3081daebf178680b5f7e117a836e4494520c0e760de2fc1ee49d3cfb3a8d089f");
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
    if (!desc || !native || (desc->CS.BytecodeLength != 50752 && desc->CS.BytecodeLength != 4868)) return;
    const unsigned kind = Matches(desc->CS,kLightSha) ? kLightComponent : Matches(desc->CS,kTemporalSha) ? kResetComponent : 0;
    if (!kind) return;
    Install();
    if (!filesReady) return;
    auto custom = *desc;
    const auto& bytes = kind == kLightComponent ? lightBytes : resetBytes;
    custom.CS = {bytes.data(),bytes.size()}; custom.CachedPSO = {};
    ID3D12PipelineState* variant = nullptr;
    HRESULT hr = create(device,&custom,IID_PPV_ARGS(&variant));
    ID3D12PipelineState* owner = nullptr;
    if (SUCCEEDED(hr)) hr = native->QueryInterface(IID_PPV_ARGS(&owner));
    if (SUCCEEDED(hr)) hr = owner->SetPrivateDataInterface(kVariant,variant);
    if (SUCCEEDED(hr)) hr = owner->SetPrivateData(kKind,sizeof(kind),&kind);
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
    const bool replace = kind == kLightComponent ? active->fullRay : kind == kResetComponent && active->raySeen && active->reset;
    if (!replace) return nullptr;
    ID3D12PipelineState* alternate = nullptr; size = sizeof(alternate);
    if (FAILED(native->GetPrivateData(kVariant,&size,&alternate)) || !alternate) return nullptr;
    if (kind == kLightComponent) active->composed = true;
    return alternate;
}
inline HRESULT CreateAugmented(ID3D12Device5* device, const D3D12_STATE_OBJECT_DESC* desc, REFIID iid, void** out) {
    using Microsoft::WRL::ComPtr;
    auto original = rt_shadow_capture::originalState;
    if (!desc || desc->Type != D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE || !filesReady)
        return original(device,desc,iid,out);
    bool target = false;
    for (UINT i=0;i<desc->NumSubobjects;++i)
        if (desc->pSubobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY && desc->pSubobjects[i].pDesc)
            target |= Matches(static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc)->DXILLibrary,kRaySha);
    if (!target) return original(device,desc,iid,out);
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
        D3D12_DXIL_LIBRARY_DESC extra{{rayBytes.data(),rayBytes.size()},1,&exportDesc};
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
        RefreshReady();Trace("rt_spot_state ready=%u native_exports_retained=1",unsigned(SUCCEEDED(prepared)));
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
    active->reset=previousFull!=active->fullRay;
    if (active->fullRay) {
        auto custom=*desc;custom.RayGenerationShaderRecord={record->GetGPUVirtualAddress(),32};
        rt_shadow_capture::Lookup(list).dispatch(list,&custom);
    }
    record->Release();return active->fullRay;
}
} // namespace rt_spot_runtime
