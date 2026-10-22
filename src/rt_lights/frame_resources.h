#pragma once
#include "layout.h"
#include "indirect_layout.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <cstring>

namespace rt_lights {
using Microsoft::WRL::ComPtr;

// Bounded scratch storage is reused between batches in command-list order.
// A separate FrameResources instance is required for each in-flight recording.
enum class Surface : unsigned {
    Work, Tasks, Ranges, Counter, Arguments, Visibility, Contributions, Lighting,
    DiffuseAtlas, SpecularAtlas, BaseDiffuse, BaseSpecular, BaseSelection, Count
};

struct BatchCapacity {
    static constexpr UINT Receivers = 32768;
    static constexpr UINT AtlasWidth = 512, AtlasHeight = 512;
    static constexpr UINT Tasks = AtlasWidth * AtlasHeight;
    static constexpr UINT ConstantsPerBatch = 4, GlobalConstants = 4;
    static constexpr UINT ConstantAlignment = 256;
};

class FrameResources {
    struct Allocation {
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        UINT stride = 0;
    };
    static constexpr UINT SurfaceCount = static_cast<UINT>(Surface::Count);
    std::array<Allocation, SurfaceCount> allocations;
    std::array<D3D12_RESOURCE_STATES, SurfaceCount> initialStates{};
    ComPtr<ID3D12DescriptorHeap> descriptors;
    ComPtr<ID3D12Resource> upload;
    unsigned char* mapped = nullptr;
    UINT descriptorStep = 0, uploadCursor = 0, uploadBytes = 0;
    UINT width = 0, height = 0;
    UINT64 residentBytes = 0;

    static D3D12_RESOURCE_DESC Buffer(UINT64 bytes) noexcept {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = bytes; desc.Height = 1;
        desc.DepthOrArraySize = desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return desc;
    }
    static D3D12_RESOURCE_DESC Texture(UINT w, UINT h, DXGI_FORMAT format) noexcept {
        auto desc = Buffer(w);
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Height = h; desc.Format = format;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        return desc;
    }
    void CreateViews(ID3D12Device* device, UINT index) noexcept {
        auto& allocation = allocations[index];
        const auto desc = allocation.resource->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            if (allocation.stride) {
                srv.Buffer.NumElements = uav.Buffer.NumElements = static_cast<UINT>(desc.Width / allocation.stride);
                srv.Buffer.StructureByteStride = uav.Buffer.StructureByteStride = allocation.stride;
            } else {
                srv.Format = uav.Format = DXGI_FORMAT_R32_TYPELESS;
                srv.Buffer.NumElements = uav.Buffer.NumElements = static_cast<UINT>(desc.Width / 4);
                srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
            }
        } else {
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            srv.Format = uav.Format = desc.Format;
            srv.Texture2D.MipLevels = 1;
        }
        device->CreateShaderResourceView(allocation.resource.Get(), &srv, View(static_cast<Surface>(index), false));
        device->CreateUnorderedAccessView(allocation.resource.Get(), nullptr, &uav, View(static_cast<Surface>(index), true));
    }
public:
    FrameResources() = default;
    FrameResources(const FrameResources&) = delete;
    FrameResources& operator=(const FrameResources&) = delete;
    ~FrameResources() { if (mapped) upload->Unmap(0, nullptr); }

    // Worker/startup thread only. Never allocate or map in the recording hook.
    HRESULT Initialize(ID3D12Device* device, UINT w, UINT h, UINT64 budget) noexcept {
        if (!device || width || !w || !h || w > 16384 || h > 16384) return E_INVALIDARG;
        const UINT64 receivers = UINT64(w) * h;
        const UINT64 batches = (receivers + BatchCapacity::Receivers - 1) / BatchCapacity::Receivers;
        const UINT64 constants = (batches * BatchCapacity::ConstantsPerBatch + BatchCapacity::GlobalConstants) * BatchCapacity::ConstantAlignment;
        if (constants > UINT32_MAX) return E_INVALIDARG;
        std::array<D3D12_RESOURCE_DESC, SurfaceCount + 1> descs;
        const auto setBuffer = [&](Surface surface, UINT count, UINT stride) {
            const auto i = static_cast<UINT>(surface);
            descs[i] = Buffer(UINT64(count) * stride); allocations[i].stride = stride;
        };
        descs[static_cast<UINT>(Surface::Work)] = Texture(BatchCapacity::AtlasWidth, BatchCapacity::AtlasHeight, DXGI_FORMAT_R32G32B32A32_UINT);
        setBuffer(Surface::Tasks, BatchCapacity::Tasks, sizeof(ShadowTask));
        setBuffer(Surface::Ranges, BatchCapacity::Receivers, sizeof(ReceiverRange));
        setBuffer(Surface::Counter, 1, sizeof(UINT));
        setBuffer(Surface::Arguments, IndirectLayout::Bytes / 4, 4);
        allocations[static_cast<UINT>(Surface::Arguments)].stride = 0;
        setBuffer(Surface::Visibility, BatchCapacity::Tasks, sizeof(Visibility));
        setBuffer(Surface::Contributions, BatchCapacity::Tasks, sizeof(Contribution));
        setBuffer(Surface::Lighting, BatchCapacity::Receivers, sizeof(Lighting));
        for (Surface surface : {Surface::DiffuseAtlas, Surface::SpecularAtlas})
            descs[static_cast<UINT>(surface)] = Texture(BatchCapacity::AtlasWidth, BatchCapacity::AtlasHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
        // Float32 private bases avoid an extra half-float rounding before commit.
        for (Surface surface : {Surface::BaseDiffuse, Surface::BaseSpecular})
            descs[static_cast<UINT>(surface)] = Texture(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
        descs[static_cast<UINT>(Surface::BaseSelection)] = Texture(w, h, DXGI_FORMAT_R32G32B32A32_UINT);
        descs[SurfaceCount] = Buffer(constants); descs[SurfaceCount].Flags = D3D12_RESOURCE_FLAG_NONE;
        // Committed resources each pay their own allocation alignment.
        UINT64 total = 0;
        for (const auto& desc : descs) {
            const auto info = device->GetResourceAllocationInfo(0, 1, &desc);
            if (info.SizeInBytes == UINT64_MAX || info.SizeInBytes > budget || total > budget - info.SizeInBytes)
                return E_OUTOFMEMORY;
            total += info.SizeInBytes;
        }
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT; heap.CreationNodeMask = heap.VisibleNodeMask = 1;
        for (UINT i = 0; i < SurfaceCount; ++i) {
            const HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &descs[i],
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&allocations[i].resource));
            if (FAILED(hr)) return hr;
        }
        heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &descs[SurfaceCount],
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));
        if (FAILED(hr)) return hr;
        D3D12_RANGE empty{0, 0}; void* data = nullptr;
        hr = upload->Map(0, &empty, &data);
        if (FAILED(hr)) return hr;
        mapped = static_cast<unsigned char*>(data); uploadBytes = static_cast<UINT>(constants);
        D3D12_DESCRIPTOR_HEAP_DESC hd{};
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors = SurfaceCount * 2 + 4 * 49;
        hr = device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&descriptors));
        if (FAILED(hr)) return hr;
        descriptorStep = device->GetDescriptorHandleIncrementSize(hd.Type);
        for (UINT i = 0; i < SurfaceCount; ++i) CreateViews(device, i);
        for (UINT snapshot = 0; snapshot < 4; ++snapshot) {
            auto cpu = Snapshot(snapshot);
            for (UINT index = 0; index < 49; ++index) {
                if (index < 5) device->CreateConstantBufferView(nullptr, cpu);
                else if (index < 41) {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv{}; srv.Format = DXGI_FORMAT_R32_FLOAT;
                    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; srv.Texture2D.MipLevels = 1;
                    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device->CreateShaderResourceView(nullptr, &srv, cpu);
                } else {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{}; uav.Format = DXGI_FORMAT_R32_FLOAT;
                    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    device->CreateUnorderedAccessView(nullptr, nullptr, &uav, cpu);
                }
                cpu.ptr += descriptorStep;
            }
        }
        width = w; height = h; residentBytes = total;
        return S_OK;
    }
    UINT Width() const noexcept { return width; }
    UINT Height() const noexcept { return height; }
    UINT64 ResidentBytes() const noexcept { return residentBytes; }
    D3D12_CPU_DESCRIPTOR_HANDLE Snapshot(UINT index) const noexcept {
        if (index >= 4) return {};
        auto cpu = descriptors->GetCPUDescriptorHandleForHeapStart();
        cpu.ptr += SIZE_T(SurfaceCount * 2 + index * 49) * descriptorStep;
        return cpu;
    }
    ID3D12Resource* Get(Surface surface) const noexcept { return allocations[static_cast<UINT>(surface)].resource.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE View(Surface surface, bool write) const noexcept {
        auto handle = descriptors->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += SIZE_T(static_cast<UINT>(surface) * 2 + unsigned(write)) * descriptorStep;
        return handle;
    }
    void Transition(ID3D12GraphicsCommandList* list, Surface surface, D3D12_RESOURCE_STATES after) noexcept {
        auto& allocation = allocations[static_cast<UINT>(surface)];
        if (allocation.state == after) return;
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = {allocation.resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, allocation.state, after};
        list->ResourceBarrier(1, &barrier); allocation.state = after;
    }
    static void UavBarrier(ID3D12GraphicsCommandList* list) noexcept {
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        list->ResourceBarrier(1, &barrier);
    }
    // Call only after the recording has retired. Upload addresses referenced by
    // earlier dispatches are immutable, including between batches of one frame.
    void BeginRecording() noexcept {
        uploadCursor = 0;
        for (UINT i = 0; i < SurfaceCount; ++i) initialStates[i] = allocations[i].state;
    }
    void DiscardRecording() noexcept {
        for (UINT i = 0; i < SurfaceCount; ++i) allocations[i].state = initialStates[i];
        uploadCursor = 0;
    }
    D3D12_GPU_VIRTUAL_ADDRESS StoreConstants(const void* data, UINT bytes) noexcept {
        if (!mapped || !data || !bytes || bytes > 256 || uploadCursor > uploadBytes - 256) return 0;
        std::memset(mapped + uploadCursor, 0, 256);
        std::memcpy(mapped + uploadCursor, data, bytes);
        const auto address = upload->GetGPUVirtualAddress() + uploadCursor;
        uploadCursor += 256;
        return address;
    }
    void CopyStored(ID3D12GraphicsCommandList* list, Surface target, D3D12_GPU_VIRTUAL_ADDRESS address, UINT bytes) noexcept {
        const auto base = upload->GetGPUVirtualAddress();
        if (address < base || address - base > uploadCursor || bytes > uploadCursor - (address - base)) return;
        Transition(list, target, D3D12_RESOURCE_STATE_COPY_DEST);
        list->CopyBufferRegion(Get(target), 0, upload.Get(), address - base, bytes);
    }
};
}
