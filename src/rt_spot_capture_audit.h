#pragma once
// Included inside native_lighting, after Trace/PatchSlot. Read-only preparation
// for hit capture: no command recording, shader replacement or queue waits.
namespace rt_spot_capture_audit {
using Microsoft::WRL::ComPtr;
inline constexpr UINT kMaximumOwners = 32;
inline constexpr UINT64 kRetainedUploadBudget = 512ull * 1024 * 1024;
inline const GUID kRootBytes = {0xd8139341,0x0c53,0x46b7,{0xb1,0x29,0x48,0x22,0x18,0x00,0x00,0x01}};
struct Owner {
    ComPtr<ID3D12Resource> resource;
    UINT64 address = 0, bytes = 0;
};
inline Owner owners[kMaximumOwners];
inline SRWLOCK ownerLock = SRWLOCK_INIT, installLock = SRWLOCK_INIT;
inline UINT64 retainedBytes = 0;
inline std::atomic<unsigned> rejectedOwners{0}, attempts{0};
inline std::atomic<bool> captured{false};
using CreateCommitted = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_HEAP_PROPERTIES*,
    D3D12_HEAP_FLAGS, const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES,
    const D3D12_CLEAR_VALUE*, REFIID, void**);
using CreatePlaced = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, ID3D12Heap*, UINT64,
    const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, REFIID, void**);
inline CreateCommitted originalCommitted = nullptr;
inline CreatePlaced originalPlaced = nullptr;

inline void ObserveOwner(IUnknown* object, D3D12_RESOURCE_STATES initialState) noexcept {
    if (!object || captured.load(std::memory_order_relaxed) || attempts.load(std::memory_order_relaxed) >= 64 || initialState != D3D12_RESOURCE_STATE_GENERIC_READ) return;
    ComPtr<ID3D12Resource> resource;
    if (FAILED(object->QueryInterface(IID_PPV_ARGS(&resource)))) return;
    const auto desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || desc.Width < 65536 || desc.Width > kRetainedUploadBudget) return;
    D3D12_HEAP_PROPERTIES heap{}; D3D12_HEAP_FLAGS flags{};
    if (FAILED(resource->GetHeapProperties(&heap,&flags)) || heap.Type != D3D12_HEAP_TYPE_UPLOAD) return;
    const auto address = resource->GetGPUVirtualAddress();
    if (!rt_spot_capture::ContainsGpuRange(address,desc.Width,address,desc.Width)) return;
    if (!TryAcquireSRWLockExclusive(&ownerLock)) { ++rejectedOwners; return; }
    if (captured.load(std::memory_order_relaxed) || attempts.load(std::memory_order_relaxed) >= 64) { ReleaseSRWLockExclusive(&ownerLock); return; }
    bool stored = false;
    for (auto& owner : owners) if (owner.resource.Get() == resource.Get()) { stored = true; break; }
    if (!stored && desc.Width <= kRetainedUploadBudget - retainedBytes) {
        for (auto& owner : owners) if (!owner.resource) {
            owner.resource = resource; owner.address = address; owner.bytes = desc.Width;
            retainedBytes += desc.Width; stored = true; break;
        }
    }
    if (!stored) ++rejectedOwners;
    ReleaseSRWLockExclusive(&ownerLock);
}
inline HRESULT STDMETHODCALLTYPE CommittedHook(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap,
    D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state,
    const D3D12_CLEAR_VALUE* clear, REFIID iid, void** result) {
    const HRESULT hr = originalCommitted(device,heap,flags,desc,state,clear,iid,result);
    if (SUCCEEDED(hr) && result && *result) ObserveOwner(static_cast<IUnknown*>(*result),state);
    return hr;
}
inline HRESULT STDMETHODCALLTYPE PlacedHook(ID3D12Device* device, ID3D12Heap* heap, UINT64 offset,
    const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state,
    const D3D12_CLEAR_VALUE* clear, REFIID iid, void** result) {
    const HRESULT hr = originalPlaced(device,heap,offset,desc,state,clear,iid,result);
    if (SUCCEEDED(hr) && result && *result) ObserveOwner(static_cast<IUnknown*>(*result),state);
    return hr;
}
inline void RememberRoot(IUnknown* object, const void* bytes, SIZE_T size) noexcept {
    if (!object || !bytes || !size || size > 8192) return;
    ComPtr<ID3D12RootSignature> root;
    if (SUCCEEDED(object->QueryInterface(IID_PPV_ARGS(&root)))) root->SetPrivateData(kRootBytes,static_cast<UINT>(size),bytes);
}
inline void DescribeRoot(ID3D12RootSignature* root, bool local) noexcept {
    unsigned char bytes[8192]{}; UINT size = sizeof(bytes);
    if (!root || FAILED(root->GetPrivateData(kRootBytes,&size,bytes))) {
        Trace("rt_hit_setup_root local=%u captured=0",unsigned(local)); return;
    }
    ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
    if (FAILED(D3D12CreateVersionedRootSignatureDeserializer(bytes,size,IID_PPV_ARGS(&decoder)))) return;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc = nullptr;
    if (FAILED(decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&desc)) || !desc) return;
    Trace("rt_hit_setup_root local=%u parameters=%u samplers=%u flags=%u",unsigned(local),desc->Desc_1_0.NumParameters,desc->Desc_1_0.NumStaticSamplers,unsigned(desc->Desc_1_0.Flags));
    for (UINT i = 0; i < desc->Desc_1_0.NumParameters && i < 32; ++i) {
        const auto& parameter = desc->Desc_1_0.pParameters[i];
        if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            for (UINT j = 0; j < parameter.DescriptorTable.NumDescriptorRanges && j < 16; ++j) {
                const auto& range = parameter.DescriptorTable.pDescriptorRanges[j];
                Trace("rt_hit_setup_range local=%u parameter=%u type=%u count=%u reg=%u space=%u offset=%u",unsigned(local),i,unsigned(range.RangeType),range.NumDescriptors,range.BaseShaderRegister,range.RegisterSpace,range.OffsetInDescriptorsFromTableStart);
            }
        } else if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            Trace("rt_hit_setup_constants local=%u parameter=%u count=%u reg=%u space=%u",unsigned(local),i,parameter.Constants.Num32BitValues,parameter.Constants.ShaderRegister,parameter.Constants.RegisterSpace);
        } else {
            Trace("rt_hit_setup_descriptor local=%u parameter=%u type=%u reg=%u space=%u",unsigned(local),i,unsigned(parameter.ParameterType),parameter.Descriptor.ShaderRegister,parameter.Descriptor.RegisterSpace);
        }
    }
}
inline void DescribeState(const D3D12_STATE_OBJECT_DESC* desc) noexcept {
    static std::atomic<unsigned> descriptions{0};
    if (!desc || descriptions.fetch_add(1) >= 2) return;
    for (UINT i = 0; i < desc->NumSubobjects; ++i) {
        const auto& object = desc->pSubobjects[i]; if (!object.pDesc) continue;
        if (object.Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE)
            DescribeRoot(static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(object.pDesc)->pLocalRootSignature,true);
        else if (object.Type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE)
            DescribeRoot(static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(object.pDesc)->pGlobalRootSignature,false);
        else if (object.Type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG) {
            const auto& config = *static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(object.pDesc);
            Trace("rt_hit_setup_payload bytes=%u attributes=%u",config.MaxPayloadSizeInBytes,config.MaxAttributeSizeInBytes);
        }
    }
}
inline void Hex(const unsigned char* bytes, UINT count, char* out) noexcept {
    static constexpr char digits[] = "0123456789abcdef";
    for (UINT i = 0; i < count; ++i) { out[2*i] = digits[bytes[i] >> 4]; out[2*i+1] = digits[bytes[i] & 15]; }
    out[2*count] = 0;
}
inline void InspectTable(ID3D12StateObject* state, const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if (!state || !desc || captured.load(std::memory_order_acquire)) return;
    if (attempts.load(std::memory_order_relaxed) >= 64) return;
    const unsigned attempt = attempts.fetch_add(1);
    if (attempt >= 64) return;
    const auto& table = desc->HitGroupTable;
    if (!rt_spot_capture::NativeHitTableLayout(table.StartAddress,table.SizeInBytes,table.StrideInBytes)) {
        if (!attempt) Trace("rt_hit_setup_table unsupported_layout=1 bytes=%llu stride=%llu",table.SizeInBytes,table.StrideInBytes);
        return;
    }
    Owner found;
    if (!TryAcquireSRWLockShared(&ownerLock)) return;
    for (const auto& owner : owners) if (owner.resource && rt_spot_capture::ContainsGpuRange(owner.address,owner.bytes,table.StartAddress,table.SizeInBytes)) { found = owner; break; }
    const auto retained = retainedBytes;
    ReleaseSRWLockShared(&ownerLock);
    if (!found.resource) {
        if (!attempt || attempt == 63) Trace("rt_hit_setup_table upload_owner=0 attempt=%u retained_upload_bytes=%llu rejected_owners=%u",attempt,retained,rejectedOwners.load());
        return;
    }
    const UINT64 offset = table.StartAddress - found.address;
    D3D12_RANGE range{static_cast<SIZE_T>(offset),static_cast<SIZE_T>(offset + table.SizeInBytes)};
    void* mapped = nullptr;
    const HRESULT hr = found.resource->Map(0,&range,&mapped);
    if (FAILED(hr) || !mapped) { Trace("rt_hit_setup_map failed=1 hr=0x%08lX",static_cast<unsigned long>(hr)); return; }
    const UINT64 count = table.SizeInBytes / 96;
    const UINT64 indices[] = {0, count > 1 ? 1 : 0, count / 2, count - 1};
    unsigned char first[4][96]{}, second[4][96]{};
    const auto* source = static_cast<const unsigned char*>(mapped) + offset;
    for (UINT i = 0; i < 4; ++i) std::memcpy(first[i],source + indices[i]*96,96);
    MemoryBarrier();
    for (UINT i = 0; i < 4; ++i) std::memcpy(second[i],source + indices[i]*96,96);
    D3D12_RANGE written{0,0}; found.resource->Unmap(0,&written);
    if (std::memcmp(first,second,sizeof(first))) { Trace("rt_hit_setup_table unstable_sample=1 retry=1"); return; }
    if (captured.exchange(true,std::memory_order_acq_rel)) return;
    Trace("rt_hit_setup_table upload_owner=1 owner_bytes=%llu table_offset=%llu table_bytes=%llu records=%llu sample_stable=1 gpu_hit_capture=0 retained_upload_bytes=%llu",found.bytes,offset,table.SizeInBytes,count,retained);
    ComPtr<ID3D12StateObjectProperties> properties;
    if (SUCCEEDED(state->QueryInterface(IID_PPV_ARGS(&properties)))) {
        for (const auto* name : {L"HitGroup_0_0",L"HitGroup_1_0"}) {
            const void* identifier = properties->GetShaderIdentifier(name);
            if (identifier) { char hex[65]{}; Hex(static_cast<const unsigned char*>(identifier),32,hex); Trace("rt_hit_setup_identifier name=%ls bytes=%s",name,hex); }
        }
    }
    for (UINT i = 0; i < 4; ++i) { char hex[193]{}; Hex(first[i],96,hex); Trace("rt_hit_setup_record index=%llu bytes=%s",indices[i],hex); }
}
// Called only by the existing diagnostic UI worker. Releasing unrelated upload
// buffers here avoids render-thread destruction and ends temporary retention.
inline void ReleaseFinishedOwners() noexcept {
    if (!captured.load(std::memory_order_acquire) && attempts.load(std::memory_order_relaxed) < 64) return;
    if (!TryAcquireSRWLockExclusive(&ownerLock)) return;
    for (auto& owner : owners) owner = {};
    retainedBytes = 0;
    ReleaseSRWLockExclusive(&ownerLock);
}
inline void Install(ID3D12Device* device) noexcept {
    if (!device) return;
    AcquireSRWLockExclusive(&installLock);
    auto** table = *reinterpret_cast<void***>(device);
    if (!originalCommitted) {
        originalCommitted = reinterpret_cast<CreateCommitted>(table[27]);
        if (!PatchSlot(&table[27],reinterpret_cast<void*>(originalCommitted),reinterpret_cast<void*>(&CommittedHook))) originalCommitted = nullptr;
    }
    if (!originalPlaced) {
        originalPlaced = reinterpret_cast<CreatePlaced>(table[29]);
        if (!PatchSlot(&table[29],reinterpret_cast<void*>(originalPlaced),reinterpret_cast<void*>(&PlacedHook))) originalPlaced = nullptr;
    }
    ReleaseSRWLockExclusive(&installLock);
    Trace("rt_hit_setup_hooks committed=%u placed=%u bounded_upload_retention=1",unsigned(originalCommitted != nullptr),unsigned(originalPlaced != nullptr));
}
} // namespace rt_spot_capture_audit
