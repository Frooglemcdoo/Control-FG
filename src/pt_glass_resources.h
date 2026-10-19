#pragma once
#include "pt_glass_resource_policy.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <mutex>

// Mod-owned resources only. Callbacks must bracket the ACTUAL queue submission,
// not Present/frame-number guesses. This component installs no hooks itself.
namespace PTGlass {
class Resources {
public:
    struct Slot {
        Ticket ticket;
        Microsoft::WRL::ComPtr<ID3D12Resource> hits, radiance, dispatch;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptors;
        std::uint32_t width=0,height=0;
        std::uint64_t allocationBytes=0;
    };
private:
    std::mutex mutex;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    std::array<Slot,SlotCount> slots{};
    std::uint64_t nextFence=1,budget=0;
    bool stopping=false,failed=false;
    void PollLocked() noexcept {
        if(!fence)return;
        const auto completed=fence->GetCompletedValue();
        if(completed==UINT64_MAX){failed=true;return;}
        for(auto& slot:slots)slot.ticket.Complete(completed);
    }
    static void Release(Slot& slot) noexcept {
        slot.descriptors.Reset();slot.hits.Reset();slot.radiance.Reset();slot.dispatch.Reset();
        slot.width=slot.height=0;slot.allocationBytes=0;slot.ticket={};
    }
    HRESULT Allocate(Slot& slot,UINT width,UINT height) noexcept {
        UINT64 hitBytes=0;if(!HitBytes(width,height,&hitBytes))return E_INVALIDARG;
        D3D12_RESOURCE_DESC desc[3]{};
        desc[0].Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        desc[0].Width=hitBytes;desc[0].Height=1;desc[0].DepthOrArraySize=1;
        desc[0].MipLevels=1;desc[0].SampleDesc.Count=1;
        desc[0].Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc[0].Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc[1].Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc[1].Width=width;desc[1].Height=height;desc[1].DepthOrArraySize=1;
        desc[1].MipLevels=1;desc[1].SampleDesc.Count=1;
        desc[1].Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc[1].Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc[2]=desc[0];desc[2].Width=512;desc[2].Flags=D3D12_RESOURCE_FLAG_NONE;
        const auto info=device->GetResourceAllocationInfo(0,3,desc);
        if(info.SizeInBytes==UINT64_MAX || info.SizeInBytes>budget)return E_OUTOFMEMORY;
        UINT64 used=0;for(const auto& other:slots)used+=other.allocationBytes;
        if(used>budget-info.SizeInBytes)return E_OUTOFMEMORY;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        heap.CreationNodeMask=heap.VisibleNodeMask=1;
        Slot fresh;
        HRESULT hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc[0],
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&fresh.hits));
        if(SUCCEEDED(hr))hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc[1],
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&fresh.radiance));
        heap.Type=D3D12_HEAP_TYPE_UPLOAD;
        if(SUCCEEDED(hr))hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc[2],
            D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&fresh.dispatch));
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.NumDescriptors=5; // hits UAV/SRV, radiance UAV/SRV; CPU staging only.
        if(SUCCEEDED(hr))hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&fresh.descriptors));
        if(FAILED(hr))return hr;
        const UINT step=device->GetDescriptorHandleIncrementSize(hd.Type);
        auto cpu=fresh.descriptors->GetCPUDescriptorHandleForHeapStart();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
        uav.Buffer.NumElements=width*height;uav.Buffer.StructureByteStride=static_cast<UINT>(HitStride);
        device->CreateUnorderedAccessView(fresh.hits.Get(),nullptr,&uav,cpu);cpu.ptr+=step;
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Buffer.NumElements=width*height;srv.Buffer.StructureByteStride=static_cast<UINT>(HitStride);
        device->CreateShaderResourceView(fresh.hits.Get(),&srv,cpu);cpu.ptr+=step;
        uav={};uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;uav.Format=desc[1].Format;
        device->CreateUnorderedAccessView(fresh.radiance.Get(),nullptr,&uav,cpu);cpu.ptr+=step;
        srv={};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Format=desc[1].Format;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(fresh.radiance.Get(),&srv,cpu);
        cpu.ptr+=step;
        D3D12_CONSTANT_BUFFER_VIEW_DESC cb{fresh.dispatch->GetGPUVirtualAddress(),256};
        device->CreateConstantBufferView(&cb,cpu);
        slot.dispatch=std::move(fresh.dispatch);
        slot.hits=std::move(fresh.hits);slot.radiance=std::move(fresh.radiance);
        slot.descriptors=std::move(fresh.descriptors);slot.width=width;slot.height=height;
        slot.allocationBytes=info.SizeInBytes;slot.ticket.phase=Phase::Idle;
        return S_OK;
    }
public:
    Resources()=default;
    Resources(const Resources&)=delete;
    Resources& operator=(const Resources&)=delete;
    ~Resources() {
        // Normal teardown requires StopAndTrim()==true. If the owner violates
        // that contract, retain pending COM references instead of freeing GPU work.
        for(auto& slot:slots)if(!slot.ticket.Releasable()){
            slot.hits.Detach();slot.radiance.Detach();slot.descriptors.Detach();slot.dispatch.Detach();
        }
    }
    HRESULT Initialize(ID3D12Device* d,ID3D12CommandQueue* q,UINT64 byteBudget) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if(device || !d || !q || !byteBudget)return E_INVALIDARG;
        const auto type=q->GetDesc().Type;
        if(type!=D3D12_COMMAND_LIST_TYPE_DIRECT && type!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return E_INVALIDARG;
        Microsoft::WRL::ComPtr<ID3D12Device> queueDevice;
        HRESULT hr=q->GetDevice(IID_PPV_ARGS(&queueDevice));
        if(FAILED(hr)||queueDevice.Get()!=d)return E_INVALIDARG;
        hr=d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
        if(SUCCEEDED(hr)){device=d;queue=q;budget=byteBudget;}
        return hr;
    }
    // recording must identify a command-list recording generation, not just
    // a reused COM pointer. Returned slot remains stable through fence completion.
    HRESULT Acquire(UINT64 recording,UINT width,UINT height,Slot** result) noexcept {
        if(!result)return E_POINTER;*result=nullptr;
        std::lock_guard<std::mutex> lock(mutex);PollLocked();
        if(!device || stopping || failed || !recording)return E_FAIL;
        UINT64 bytes=0;if(!HitBytes(width,height,&bytes))return E_INVALIDARG;
        for(auto& slot:slots)if(slot.ticket.phase==Phase::Recording && slot.ticket.recording==recording)
            return E_UNEXPECTED; // Never alias two dispatch outputs while recording.
        // Retire old extents only after their queues have finished.
        for(auto& slot:slots)if(slot.ticket.Releasable() && (slot.width!=width || slot.height!=height))Release(slot);
        Slot* candidate=nullptr;
        for(auto& slot:slots)if(slot.ticket.Releasable() && slot.hits){candidate=&slot;break;}
        if(!candidate)for(auto& slot:slots)if(slot.ticket.Releasable()){candidate=&slot;break;}
        if(!candidate)return DXGI_ERROR_WAS_STILL_DRAWING;
        if(!candidate->hits){const HRESULT hr=Allocate(*candidate,width,height);if(FAILED(hr))return hr;}
        if(!candidate->ticket.Lease(recording))return E_UNEXPECTED;
        *result=candidate;return S_OK;
    }
    // Call only after ExecuteCommandLists containing this recording returns,
    // on this SAME queue. A failed signal quarantines the resources permanently.
    HRESULT Submitted(UINT64 recording) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        Slot* found=nullptr;
        for(auto& slot:slots)if(slot.ticket.phase==Phase::Recording && slot.ticket.recording==recording){found=&slot;break;}
        if(!found || failed || nextFence==UINT64_MAX)return E_UNEXPECTED;
        const UINT64 value=nextFence++;
        const HRESULT hr=queue->Signal(fence.Get(),value);
        if(FAILED(hr)){found->ticket.phase=Phase::Quarantined;failed=true;return hr;}
        return found->ticket.Submit(recording,value)?S_OK:E_UNEXPECTED;
    }
    // Only after a recorded list is RESET/DISCARDED without submission.
    bool Discarded(UINT64 recording) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        for(auto& slot:slots)if(slot.ticket.Discard(recording))return true;
        return false;
    }
    bool StopAndTrim() noexcept {
        std::lock_guard<std::mutex> lock(mutex);stopping=true;PollLocked();
        bool finished=true;
        for(auto& slot:slots){if(slot.ticket.Releasable())Release(slot);else finished=false;}
        return finished;
    }
    UINT64 ResidentBytes() noexcept {
        std::lock_guard<std::mutex> lock(mutex);UINT64 bytes=0;
        for(const auto& slot:slots)bytes+=slot.allocationBytes;return bytes;
    }
};
}
