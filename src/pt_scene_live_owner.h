#pragma once
#include "pt_owned_scene_runtime.h"
#include "pt_scene_capture_runtime.h"
#include "pt_scene_descriptors.h"
#include "pt_native_buffer_observer.h"
#include <atomic>
// Session owner, independent of the timed M11 diagnostic. Dormant until the
// renderer starts it BEFORE scene loading. No GPU work or latest-scene selection.
namespace PTSceneLive {
using U64=std::uint64_t;
using Microsoft::WRL::ComPtr;
class Owner {
    PTOwnedScene::Runtime resources;
    Capture capture{resources};
    std::unique_ptr<PTSceneDescriptors::Index> descriptors;
    std::atomic<bool> active{false};
    UINT step=0;
    static bool NativeASDescriptor(void* buffer,U64& handle) noexcept {
        __try {if(!buffer)return false;handle=*reinterpret_cast<U64*>(static_cast<unsigned char*>(buffer)+0x10);return handle!=0;}
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
public:
    bool Start(UINT increment){
        if(active||descriptors||!increment)return false;
        step=increment;descriptors=std::make_unique<PTSceneDescriptors::Index>(step);
        active.store(true,std::memory_order_release);return true;
    }
    bool Active() const noexcept {return active.load(std::memory_order_acquire);}
    void Created(void* buffer,bool accelerationDescriptor){
        if(!Active())return;
        PTNativeBufferObserver::Snapshot b{};
        if(!PTNativeBufferObserver::Read(buffer,b))return;
        const auto status=resources.Created(reinterpret_cast<UINT_PTR>(buffer),reinterpret_cast<IUnknown*>(b.resource),b.address,b.bytes);
        if(accelerationDescriptor){
            U64 handle=0;if(!NativeASDescriptor(buffer,handle))return;
            PTOwnedScene::Allocation allocation{};
            if(status==1&&resources.Identify(b.address,256,allocation))
                descriptors->Write(handle,{b.address,allocation.token,allocation.bytes});
            else descriptors->Write(handle);
        }
    }
    PTOwnedScene::Runtime::Release BeforeRelease(void* buffer){return Active()?resources.BeforeRelease(reinterpret_cast<UINT_PTR>(buffer)):PTOwnedScene::Runtime::Release{};}
    void AfterRelease(PTOwnedScene::Runtime::Release& lease){resources.AfterRelease(lease);}
    void Geometry(const PTSceneObserver::Event& event){if(Active())resources.Geometry(event);}
    void Overwrite(D3D12_CPU_DESCRIPTOR_HANDLE handle,const D3D12_SHADER_RESOURCE_VIEW_DESC* srv=nullptr){
        if(!Active())return;
        PTOwnedScene::Allocation allocation{};
        if(srv&&srv->ViewDimension==D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE&&
           resources.Identify(srv->RaytracingAccelerationStructure.Location,256,allocation))
            descriptors->Write(handle.ptr,{allocation.address,allocation.token,allocation.bytes});
        else descriptors->Write(handle.ptr);
    }
    void Heap(ID3D12DescriptorHeap* heap){
        if(!Active()||!heap)return;const auto d=heap->GetDesc();
        if(d.Type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)descriptors->Invalidate({heap->GetCPUDescriptorHandleForHeapStart().ptr,d.NumDescriptors});
    }
    void InvalidateDescriptors(){if(Active())descriptors->Invalidate({});}
    void Copy(const std::vector<PTSceneDescriptors::Range>& dst,const std::vector<PTSceneDescriptors::Range>& src){if(Active())descriptors->Copy(dst,src);}
    bool Select(ID3D12DescriptorHeap* heap,UINT tableFirst,UINT srvSlot,PTSceneDescriptors::Acceleration& result){
        result={};if(!Active()||!heap||srvSlot>=36)return false;
        const auto d=heap->GetDesc();const U64 index=U64(tableFirst)+5+srvSlot;
        if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||index>=d.NumDescriptors)return false;
        const U64 base=heap->GetCPUDescriptorHandleForHeapStart().ptr;
        if(index>(UINT64_MAX-base)/step)return false;
        const auto selected=descriptors->Read(base+index*step);
        PTOwnedScene::Allocation allocation{};
        if(!selected.token||!resources.Identify(selected.address,selected.bytes,allocation)||allocation.token!=selected.token)return false;
        result=selected;return true;
    }
    PTOwnedScene::Runtime& Resources(){return resources;}
    Capture& Scenes(){return capture;}
};
inline Owner owner;
}
