#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <d3d12.h>
#include <wrl/client.h>

// Each sample lives on the same recording lease as the character pass.
// Read is called only after its submitted queue fence completes.
struct GICharacterMeasurement {
    UINT64 frame=0;
    Microsoft::WRL::ComPtr<ID3D12Resource> zero, counters, readback;
    struct Sample {
        std::array<UINT,5> counts{};
        UINT location=0;
        float gi[3]{},scene[3]{},proposed[3]{};
        UINT temporal=0,strongTemporal=0,sourceCorrected=0;
    };
    static_assert(sizeof(Sample)==72 && offsetof(Sample,sourceCorrected)==68 && offsetof(Sample,strongTemporal)==64 && offsetof(Sample,temporal)==60 && offsetof(Sample,location)==20 && offsetof(Sample,gi)==24 &&
                  offsetof(Sample,scene)==36 && offsetof(Sample,proposed)==48,"Shader readback layout");
    static constexpr UINT Bytes=sizeof(Sample);
    bool Prepare(ID3D12Device* device, UINT64 sourceFrame) noexcept {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Width=Bytes;desc.Height=desc.DepthOrArraySize=desc.MipLevels=desc.SampleDesc.Count=1;
        D3D12_HEAP_PROPERTIES heap{};heap.CreationNodeMask=heap.VisibleNodeMask=1;
        heap.Type=D3D12_HEAP_TYPE_READBACK;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
                D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return false;
        heap.Type=D3D12_HEAP_TYPE_UPLOAD;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&zero))))return false;
        void* data=nullptr;D3D12_RANGE empty{};
        if(FAILED(zero->Map(0,&empty,&data)))return false;
        std::memset(data,0,Bytes);zero->Unmap(0,nullptr);
        heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
                D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&counters))))return false;
        frame=sourceFrame;
        return true;
    }
    void Transition(ID3D12GraphicsCommandList* list,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) const noexcept {
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={counters.Get(),0,before,after};
        list->ResourceBarrier(1,&barrier);
    }
    void Begin(ID3D12GraphicsCommandList* list) const noexcept {
        list->CopyBufferRegion(counters.Get(),0,zero.Get(),0,Bytes);
        Transition(list,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        list->SetComputeRootUnorderedAccessView(1,counters->GetGPUVirtualAddress());
    }
    void End(ID3D12GraphicsCommandList* list) const noexcept {
        Transition(list,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        list->CopyBufferRegion(readback.Get(),0,counters.Get(),0,Bytes);
    }
    bool Read(Sample& values) const noexcept {
        D3D12_RANGE range{0,Bytes};void* ptr=nullptr;
        if(FAILED(readback->Map(0,&range,&ptr)))return false;
        std::memcpy(&values,ptr,Bytes);D3D12_RANGE empty{};readback->Unmap(0,&empty);
        return true;
    }
};
