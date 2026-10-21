#pragma once
// Included inside native_lighting after Trace and PatchSlot. This observer never
// changes a descriptor, shader, root signature, state object or native result.
namespace rt_shadow_capture {
using CreateRoot = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, UINT, const void*, SIZE_T, REFIID, void**);
using CreateState = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device5*, const D3D12_STATE_OBJECT_DESC*, REFIID, void**);
using CreateSrv = void (STDMETHODCALLTYPE*)(ID3D12Device*, ID3D12Resource*, const D3D12_SHADER_RESOURCE_VIEW_DESC*, D3D12_CPU_DESCRIPTOR_HANDLE);
inline CreateRoot originalRoot = nullptr;
inline CreateState originalState = nullptr;
inline CreateSrv originalSrv = nullptr;
inline SRWLOCK installLock = SRWLOCK_INIT, captureLock = SRWLOCK_INIT;
inline char captured[128][65]{};
inline unsigned capturedCount = 0;
inline size_t capturedBytes = 0;
inline std::atomic<unsigned> stateReports{0}, rootReports{0}, sceneReports{0};
inline std::atomic<bool> limitReported{false};
inline constexpr size_t kBlobLimit = 2 * 1024 * 1024, kSessionLimit = 8 * 1024 * 1024;

// Store exact shader/root bytes inside the single session log. One write/flush
// per unique blob, at creation time; no per-frame readback or disk capture.
inline bool Blob(const char* kind, const void* data, size_t size) noexcept {
    if (!data || !size || size > kBlobLimit) {
        Trace("rt_blob_rejected kind=%s bytes=%llu", kind, static_cast<unsigned long long>(size));
        return false;
    }
    char sha[65]{};
    if (!PTReflectionSha256(static_cast<const unsigned char*>(data), size, sha)) {
        Trace("rt_blob_rejected kind=%s reason=hash_failed", kind); return false;
    }
    AcquireSRWLockExclusive(&captureLock);
    for (unsigned i=0; i<capturedCount; ++i) if (!strcmp(captured[i],sha)) {
        ReleaseSRWLockExclusive(&captureLock); return true;
    }
    if (capturedCount == 128 || size > kSessionLimit-capturedBytes) {
        ReleaseSRWLockExclusive(&captureLock);
        if (!limitReported.exchange(true)) Trace("rt_capture_limit_reached max_raw_bytes=%llu", static_cast<unsigned long long>(kSessionLimit));
        return false;
    }
    bool saved = false;
    try {
        constexpr char digits[]="0123456789abcdef";
        char heading[192]{};
        snprintf(heading,sizeof(heading),"rt_blob_begin kind=%s sha=%s bytes=%llu\r\n",kind,sha,static_cast<unsigned long long>(size));
        std::string block(heading);
        block.reserve(size*3+4096);
        const auto* bytes=static_cast<const unsigned char*>(data);
        for (size_t offset=0; offset<size; offset+=128) {
            snprintf(heading,sizeof(heading),"rt_blob_data sha=%s offset=%llu hex=",sha,static_cast<unsigned long long>(offset));
            block+=heading;
            const size_t end=(offset+128<size)?offset+128:size;
            for(size_t i=offset;i<end;++i) { block+=digits[bytes[i]>>4]; block+=digits[bytes[i]&15]; }
            block+="\r\n";
        }
        block+="rt_blob_end sha=";block+=sha;block+="\r\n";
        AcquireSRWLockExclusive(&reportLock);
        saved=report.OpenDefault() && report.Write(block.data(),static_cast<DWORD>(block.size()));
        reportHealthy.store(saved);
        ReleaseSRWLockExclusive(&reportLock);
    } catch (...) { saved=false; }
    if(saved) { strcpy_s(captured[capturedCount++],sha);capturedBytes+=size; }
    ReleaseSRWLockExclusive(&captureLock);
    Trace("rt_blob_saved kind=%s sha=%s bytes=%llu saved=%u",kind,sha,static_cast<unsigned long long>(size),unsigned(saved));
    return saved;
}

inline std::atomic<unsigned> computeCreatedMask{0}, computeSavedMask{0}, computeBoundMask{0};
inline void RecordCompute(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
    if (!desc || !desc->CS.pShaderBytecode || !IsCandidateSize(desc->CS.BytecodeLength)) return;
    char sha[65]{};
    if (!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode), desc->CS.BytecodeLength, sha)) return;
    const unsigned bit = ComputeCandidateBit(desc->CS.BytecodeLength, sha);
    if (!bit) return;
    const bool first = !(computeCreatedMask.fetch_or(bit) & bit);
    if (Blob("compute_cs", desc->CS.pShaderBytecode, desc->CS.BytecodeLength))
        computeSavedMask.fetch_or(bit);
    if (first) Trace("rt_compute_created bit=%u sha=%s bytes=%llu root=%p role=unverified", bit, sha,
        static_cast<unsigned long long>(desc->CS.BytecodeLength), desc->pRootSignature);
}
inline void RecordComputeBinding(size_t bytes, const char* sha) noexcept {
    const unsigned bit = ComputeCandidateBit(bytes, sha);
    if (bit && !(computeBoundMask.fetch_or(bit) & bit))
        Trace("rt_compute_bound bit=%u sha=%s gpu_confirmed=0", bit, sha);
}

inline void RecordDirect(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
    Blob("direct_cs",desc->CS.pShaderBytecode,desc->CS.BytecodeLength);
    static std::atomic<unsigned> reports{0};
    if(reports.fetch_add(1)<32) Trace("rt_direct_root object=%p",desc->pRootSignature);
}
inline HRESULT STDMETHODCALLTYPE RootHook(ID3D12Device* device,UINT node,const void* bytes,SIZE_T size,REFIID iid,void** out) {
    const HRESULT hr=originalRoot(device,node,bytes,size,iid,out);
    if(SUCCEEDED(hr) && out && *out && rootReports.fetch_add(1)<256) {
        char sha[65]{};PTReflectionSha256(static_cast<const unsigned char*>(bytes),size,sha);
        Trace("rt_root object=%p sha=%s bytes=%llu",*out,sha,static_cast<unsigned long long>(size));
        Blob("root_signature",bytes,size);
    }
    return hr;
}
inline void STDMETHODCALLTYPE SrvHook(ID3D12Device* device,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination) {
    originalSrv(device,resource,desc,destination);
    if(desc && desc->ViewDimension==D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE && sceneReports.fetch_add(1)<128)
        Trace("rt_scene_srv cpu=%llu gpu_va=%llu sample_only=1",static_cast<unsigned long long>(destination.ptr),desc->RaytracingAccelerationStructure.Location);
}
inline HRESULT STDMETHODCALLTYPE StateHook(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    const HRESULT hr=originalState(device,desc,iid,out);
    if(!desc || FAILED(hr) || !out || !*out || stateReports.fetch_add(1)>=128) return hr;
    Trace("rt_state object=%p type=%u subobjects=%u",*out,unsigned(desc->Type),desc->NumSubobjects);
    for(UINT i=0;i<desc->NumSubobjects;++i) {
        const auto& object=desc->pSubobjects[i];
        if(!object.pDesc) continue;
        if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY) {
            const auto& library=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc);
            char sha[65]{};PTReflectionSha256(static_cast<const unsigned char*>(library.DXILLibrary.pShaderBytecode),library.DXILLibrary.BytecodeLength,sha);
            Trace("rt_library state=%p index=%u sha=%s exports=%u",*out,i,sha,library.NumExports);
            Blob("dxil_library",library.DXILLibrary.pShaderBytecode,library.DXILLibrary.BytecodeLength);
            for(UINT j=0;j<library.NumExports && j<128;++j)
                Trace("rt_export state=%p library=%u name=%ls rename=%ls",*out,i,
                    library.pExports[j].Name?library.pExports[j].Name:L"",
                    library.pExports[j].ExportToRename?library.pExports[j].ExportToRename:L"");
        } else if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP) {
            const auto& group=*static_cast<const D3D12_HIT_GROUP_DESC*>(object.pDesc);
            Trace("rt_hit_group state=%p index=%u name=%ls closest=%ls any=%ls intersection=%ls",*out,i,
                group.HitGroupExport?group.HitGroupExport:L"",group.ClosestHitShaderImport?group.ClosestHitShaderImport:L"",
                group.AnyHitShaderImport?group.AnyHitShaderImport:L"",group.IntersectionShaderImport?group.IntersectionShaderImport:L"");
        } else if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE || object.Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE) {
            auto* root=object.Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE ?
                static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(object.pDesc)->pLocalRootSignature :
                static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(object.pDesc)->pGlobalRootSignature;
            Trace("rt_state_root state=%p index=%u local=%u root=%p",*out,i,unsigned(object.Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE),root);
        } else if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
            const auto& association=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(object.pDesc);
            // Locate by equality; do not subtract an unrelated pointer.
            UINT target=desc->NumSubobjects;
            for(UINT j=0;j<desc->NumSubobjects;++j) if(association.pSubobjectToAssociate==desc->pSubobjects+j) {target=j;break;}
            for(UINT j=0;j<association.NumExports && j<128;++j)
                Trace("rt_association state=%p target=%u export=%ls",*out,target,association.pExports[j]);
        }
    }
    return hr;
}
using SetState = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,ID3D12StateObject*);
using Dispatch = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
struct ListHooks { void** table=nullptr; SetState set=nullptr; Dispatch dispatch=nullptr; };
inline ListHooks listHooks[16]{};
inline SRWLOCK listLock=SRWLOCK_INIT;
inline std::atomic<unsigned> rayReports{0}, stateBindings{0};
inline ListHooks Lookup(ID3D12GraphicsCommandList4* list) noexcept {
    auto** table=*reinterpret_cast<void***>(list);
    ListHooks found{};
    AcquireSRWLockShared(&listLock);
    for(const auto& entry:listHooks) if(entry.table==table) {found=entry;break;}
    ReleaseSRWLockShared(&listLock);
    return found;
}
inline void STDMETHODCALLTYPE SetStateHook(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state) {
    const auto hooks=Lookup(list);
    hooks.set(list,state);
    if(stateBindings.fetch_add(1)<128) Trace("rt_state_bound list=%p state=%p",list,state);
}
inline void STDMETHODCALLTYPE DispatchHook(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc) {
    const auto hooks=Lookup(list);
    hooks.dispatch(list,desc);
    if(desc && rayReports.fetch_add(1)<128)
        Trace("rt_dispatch list=%p extent=%ux%ux%u raygen=%llu hit_start=%llu hit_bytes=%llu hit_stride=%llu miss_start=%llu miss_bytes=%llu miss_stride=%llu gpu_confirmed=0",
            list,desc->Width,desc->Height,desc->Depth,desc->RayGenerationShaderRecord.StartAddress,
            desc->HitGroupTable.StartAddress,desc->HitGroupTable.SizeInBytes,desc->HitGroupTable.StrideInBytes,
            desc->MissShaderTable.StartAddress,desc->MissShaderTable.SizeInBytes,desc->MissShaderTable.StrideInBytes);
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    ID3D12GraphicsCommandList4* rays=nullptr;
    if(FAILED(list->QueryInterface(IID_PPV_ARGS(&rays)))) return;
    auto** table=*reinterpret_cast<void***>(rays);
    AcquireSRWLockExclusive(&listLock);
    ListHooks* slot=nullptr;
    bool exists=false;
    for(auto& entry:listHooks) {
        if(entry.table==table) {exists=true;break;}
        if(!entry.table && !slot) slot=&entry;
    }
    if(!exists && slot) {
        *slot={table,reinterpret_cast<SetState>(table[75]),reinterpret_cast<Dispatch>(table[76])};
        const bool set=PatchSlot(&table[75],reinterpret_cast<void*>(slot->set),reinterpret_cast<void*>(&SetStateHook));
        const bool dispatch=set && PatchSlot(&table[76],reinterpret_cast<void*>(slot->dispatch),reinterpret_cast<void*>(&DispatchHook));
        if(!dispatch && set) PatchSlot(&table[75],reinterpret_cast<void*>(&SetStateHook),reinterpret_cast<void*>(slot->set));
        if(!set) *slot={}; // Retain forwarding state after a partial installation.
        Trace("rt_command_hooks set=%u dispatch=%u",unsigned(set),unsigned(dispatch));
    }
    ReleaseSRWLockExclusive(&listLock);
    rays->Release();
}
inline void Install(ID3D12Device* device) noexcept {
    AcquireSRWLockExclusive(&installLock);
    void** table=*reinterpret_cast<void***>(device);
    if(!originalRoot) {
        originalRoot=reinterpret_cast<CreateRoot>(table[16]);
        if(!PatchSlot(&table[16],reinterpret_cast<void*>(originalRoot),reinterpret_cast<void*>(&RootHook))) originalRoot=nullptr;
    }
    if(!originalSrv) {
        originalSrv=reinterpret_cast<CreateSrv>(table[18]);
        if(!PatchSlot(&table[18],reinterpret_cast<void*>(originalSrv),reinterpret_cast<void*>(&SrvHook))) originalSrv=nullptr;
    }
    ID3D12Device5* rays=nullptr;
    if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&rays)))) {
        auto** rayTable=*reinterpret_cast<void***>(rays);
        if(!originalState) {
            originalState=reinterpret_cast<CreateState>(rayTable[62]);
            if(!PatchSlot(&rayTable[62],reinterpret_cast<void*>(originalState),reinterpret_cast<void*>(&StateHook))) originalState=nullptr;
        }
        rays->Release();
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};
    const HRESULT feature=device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options));
    ReleaseSRWLockExclusive(&installLock);
    Trace("rt_capture_hooks root=%u srv=%u state=%u feature_hr=0x%08lX dxr_tier=%u replacement_active=0",
        unsigned(originalRoot!=nullptr),unsigned(originalSrv!=nullptr),unsigned(originalState!=nullptr),static_cast<unsigned long>(feature),unsigned(options.RaytracingTier));
}
inline void Summary() noexcept {
    Trace("rt_compute_summary expected_mask=%u created_mask=%u saved_mask=%u bound_mask=%u missing_saved_mask=%u replacement_active=0",
        kExpectedComputeMask, computeCreatedMask.load(), computeSavedMask.load(), computeBoundMask.load(),
        kExpectedComputeMask & ~computeSavedMask.load());
    AcquireSRWLockShared(&captureLock);
    const unsigned count=capturedCount;
    const size_t bytes=capturedBytes;
    ReleaseSRWLockShared(&captureLock);
    Trace("rt_capture_summary blobs=%u raw_bytes=%llu roots_seen=%u states_seen=%u scene_srvs_seen=%u state_bindings_seen=%u ray_dispatches_seen=%u limit_reached=%u replacement_active=0",
        count,static_cast<unsigned long long>(bytes),rootReports.load(),stateReports.load(),sceneReports.load(),stateBindings.load(),rayReports.load(),unsigned(limitReported.load()));
}
} // namespace rt_shadow_capture
