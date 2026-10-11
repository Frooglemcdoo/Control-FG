#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace PTGlass {
// Verified Steam d3d module cceb99cb...: root parameter 0 is b4..8,
// t0..35, u0..7. Root 7 is the separate material table. Never replace heaps
// or copy FROM the native shader-visible table to build this snapshot.
struct LightingTableLayout {
    static constexpr UINT Count=49, Srv=5, Uav=41;
    static constexpr UINT Dispatch=2, Hits=Srv+28, Output=Uav;
    static constexpr UINT NativeMaterialPrefix=0x3b920;
    static constexpr std::uint64_t NativeRequired=3ull|(((1ull<<23)-1)<<6);
    static constexpr std::uint64_t CustomRequired=(1ull<<Dispatch)|(1ull<<Hits)|(1ull<<Output);
};

struct DescriptorRange {
    ID3D12DescriptorHeap* heap=nullptr;
    UINT first=0,count=0;
};

// One recording thread owns a shadow. Observe writes while the native glass
// ConstantTable::use populates its table. Unsupported/missed writes must abort
// that recording; no previous recording's descriptors satisfy Ready().
class LightingTableShadow {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> staging;
    DescriptorRange native{};
    UINT step=0;
    std::uint64_t epoch=0,seen=0;
    bool published=false,configured=false;
    D3D12_CPU_DESCRIPTOR_HANDLE At(UINT i) const noexcept {
        auto h=staging->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*step;return h;
    }
    bool Valid(const DescriptorRange& r,bool visible) const noexcept {
        if(!r.heap||!r.count)return false;
        const auto d=r.heap->GetDesc();
        if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||
           bool(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=visible||
           r.first>d.NumDescriptors||r.count>d.NumDescriptors-r.first)return false;
        Microsoft::WRL::ComPtr<ID3D12Device> owner;
        return SUCCEEDED(r.heap->GetDevice(IID_PPV_ARGS(&owner)))&&owner.Get()==device.Get();
    }
public:
    HRESULT Initialize(ID3D12Device* d) noexcept {
        if(!d||device)return E_INVALIDARG;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.NumDescriptors=LightingTableLayout::Count;
        hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        const HRESULT hr=d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&staging));
        if(SUCCEEDED(hr)){device=d;step=d->GetDescriptorHandleIncrementSize(hd.Type);}
        return hr;
    }
    bool Begin(std::uint64_t recording,const DescriptorRange& nativeTable) noexcept {
        epoch=seen=0;published=configured=false;native={};
        if(!device||!recording||!Valid(nativeTable,true)||nativeTable.count!=LightingTableLayout::Count)return false;
        native=nativeTable;epoch=recording;
        // Initialize unused slots too. Required bindings still need observed
        // native writes; null defaults cannot make an incomplete snapshot ready.
        for(UINT i=0;i<5;++i)device->CreateConstantBufferView(nullptr,At(i));
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_FLOAT;
        srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        for(UINT i=5;i<41;++i)device->CreateShaderResourceView(nullptr,&srv,At(i));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_FLOAT;
        uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        for(UINT i=41;i<49;++i)device->CreateUnorderedAccessView(nullptr,nullptr,&uav,At(i));
        return true;
    }
    // Hook adapters must map a native destination handle to its table index,
    // and pass only a verified CPU source heap. GPU-visible sources are rejected.
    bool Copy(std::uint64_t recording,UINT destination,const DescriptorRange& source) noexcept {
        if(recording!=epoch||!epoch||published||destination>=49||!Valid(source,false)||source.count>49-destination)return false;
        auto src=source.heap->GetCPUDescriptorHandleForHeapStart();src.ptr+=SIZE_T(source.first)*step;
        device->CopyDescriptorsSimple(source.count,At(destination),src,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        seen|=((1ull<<source.count)-1)<<destination;configured=false;return true;
    }
    bool Constant(std::uint64_t recording,UINT destination,const D3D12_CONSTANT_BUFFER_VIEW_DESC& cb) noexcept {
        if(recording!=epoch||!epoch||published||destination>=5||!cb.BufferLocation||
           cb.BufferLocation%256||!cb.SizeInBytes||cb.SizeInBytes%256||cb.SizeInBytes>65536)return false;
        device->CreateConstantBufferView(&cb,At(destination));seen|=1ull<<destination;configured=false;return true;
    }
    bool ConfigureSecondary(std::uint64_t recording,const D3D12_CONSTANT_BUFFER_VIEW_DESC& dispatch,
                            const DescriptorRange& hits,const DescriptorRange& output) noexcept {
        configured=false;
        if(hits.count!=1||output.count!=1||!Valid(hits,false)||!Valid(output,false))return false;
        if(!Constant(recording,LightingTableLayout::Dispatch,dispatch)||
           !Copy(recording,LightingTableLayout::Hits,hits)||!Copy(recording,LightingTableLayout::Output,output))return false;
        configured=true;return true;
    }
    bool Ready(std::uint64_t recording) const noexcept {
        constexpr auto required=LightingTableLayout::NativeRequired|LightingTableLayout::CustomRequired;
        return epoch&&recording==epoch&&!published&&configured&&(seen&required)==required;
    }
    // Destination must be a fresh, fence-retired allocation supplied by the
    // native allocator. Caller retains that allocation through GPU completion.
    // Reject heap rollover: preserving native root 7 requires the SAME heap.
    bool Publish(std::uint64_t recording,const DescriptorRange& fresh,
                 D3D12_GPU_DESCRIPTOR_HANDLE* customTable) noexcept {
        if(!customTable)return false;customTable->ptr=0;
        if(!Ready(recording)||!Valid(fresh,true)||fresh.heap!=native.heap||fresh.count!=49||
           fresh.first<LightingTableLayout::NativeMaterialPrefix||
           (fresh.first<native.first+native.count&&native.first<fresh.first+fresh.count))return false;
        auto dst=fresh.heap->GetCPUDescriptorHandleForHeapStart();dst.ptr+=SIZE_T(fresh.first)*step;
        device->CopyDescriptorsSimple(49,dst,At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        *customTable=fresh.heap->GetGPUDescriptorHandleForHeapStart();customTable->ptr+=UINT64(fresh.first)*step;
        published=true;return true;
    }
};

// Must run immediately around a dispatch with the verified native root already
// bound. Only root parameter 0 and the PSO change, leaving b0..3, sampler roots,
// heap bindings, and the global material table intact. No engine cache writes.
class ScopedLightingBinding {
    ID3D12GraphicsCommandList* list;
    ID3D12PipelineState* nativePso;
    D3D12_GPU_DESCRIPTOR_HANDLE nativeTable;
public:
    ScopedLightingBinding(ID3D12GraphicsCommandList* commandList,ID3D12PipelineState* original,
        D3D12_GPU_DESCRIPTOR_HANDLE originalTable,ID3D12PipelineState* custom,
        D3D12_GPU_DESCRIPTOR_HANDLE customTable) noexcept:
        list(commandList),nativePso(original),nativeTable(originalTable) {
        list->SetPipelineState(custom);list->SetComputeRootDescriptorTable(0,customTable);
    }
    ~ScopedLightingBinding(){list->SetComputeRootDescriptorTable(0,nativeTable);list->SetPipelineState(nativePso);}
    ScopedLightingBinding(const ScopedLightingBinding&)=delete;
    ScopedLightingBinding& operator=(const ScopedLightingBinding&)=delete;
};
}
