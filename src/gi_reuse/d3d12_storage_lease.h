#pragma once
#include <d3d12.h>
#include <wrl/client.h>

namespace gi_reuse {
inline const GUID storageKindKey={0xa574c93e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x01,0x11}};
inline const GUID storageHeapKey={0xa574c93e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x01,0x12}};
enum class BufferStorageKind : UINT { Unknown, Committed, Placed };
// Called only after a successful observed committed/placed resource creation.
// Private interface data owns the heap for exactly the resource object's life,
// including inventory eviction/readmission. No global heap/address registry.
inline bool RetainCreatedStorage(ID3D12Resource* resource,ID3D12Heap* heap=nullptr) noexcept {
    if (!resource || resource->GetDesc().Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER) return false;
    auto kind=BufferStorageKind::Unknown;
    if (FAILED(resource->SetPrivateData(storageKindKey,sizeof(kind),&kind))) return false;
    if (FAILED(resource->SetPrivateDataInterface(storageHeapKey,heap))) return false;
    kind=heap?BufferStorageKind::Placed:BufferStorageKind::Committed;
    return SUCCEEDED(resource->SetPrivateData(storageKindKey,sizeof(kind),&kind));
}
struct BufferStorageLease {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    Microsoft::WRL::ComPtr<ID3D12Heap> heap;
    BufferStorageKind kind=BufferStorageKind::Unknown;
    UINT64 address=0,bytes=0; // Immutable resource description captured with the lease.
};
inline bool AcquireStorageLease(ID3D12Resource* resource,BufferStorageLease& lease) noexcept {
    lease={};
    if (!resource) return false;
    BufferStorageKind kind=BufferStorageKind::Unknown; UINT bytes=sizeof(kind);
    if (FAILED(resource->GetPrivateData(storageKindKey,&bytes,&kind)) || bytes!=sizeof(kind)) return false;
    if (kind!=BufferStorageKind::Committed && kind!=BufferStorageKind::Placed) return false;
    Microsoft::WRL::ComPtr<ID3D12Heap> heap;
    if (kind==BufferStorageKind::Placed) {
        Microsoft::WRL::ComPtr<IUnknown> object; bytes=sizeof(IUnknown*);
        if (FAILED(resource->GetPrivateData(storageHeapKey,&bytes,object.GetAddressOf())) ||
            bytes!=sizeof(IUnknown*) || !object || FAILED(object.As(&heap))) return false;
    }
    const auto desc=resource->GetDesc();const auto address=resource->GetGPUVirtualAddress();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!address||!desc.Width||desc.Width>UINT64_MAX-address)return false;
    lease.resource=resource; lease.heap=heap; lease.kind=kind;lease.address=address;lease.bytes=desc.Width;
    return true;
}
// A lease proves object/backing-heap retention only. It does not prove alias
// activation, tile mapping, residency, resource states or unchanged GPU bytes.
}
