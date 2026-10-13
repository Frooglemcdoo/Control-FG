#pragma once
#include "pt_owned_scene_runtime.h"
#include "pt_scene_capture_runtime.h"
#include "pt_scene_descriptors.h"
#include "pt_native_buffer_observer.h"
#include <atomic>
// Session owner, independent of the timed M11 diagnostic. Dormant until the
// renderer starts it BEFORE scene loading. Geometry copies are opt-in at session start; no latest-scene selection.
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
    bool Start(UINT increment,bool preserveGeometry=true){
        if(active||descriptors||!increment)return false;
        if(preserveGeometry&&!capture.PreserveGeometry())return false;
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
    void InvalidateDescriptors(){if(Active())descriptors->InvalidateAll("descriptor_copy_null_ranges");}
    void CopySimple(U64 destination,U64 source,U64 count){if(Active())descriptors->CopySimple(destination,source,count);}
    void Copy(const std::vector<PTSceneDescriptors::Range>& dst,const std::vector<PTSceneDescriptors::Range>& src){if(Active())descriptors->Copy(dst,src);}
    struct SelectionDiagnostic {const char* reason="descriptor_input";U64 handle=0,address=0,token=0,bytes=0,currentToken=0;};
    PTSceneDescriptors::Index::Stats DescriptorStatistics(){return descriptors?descriptors->Statistics():PTSceneDescriptors::Index::Stats{false,"descriptor_inactive",0,0,0,0};}
    bool Select(ID3D12DescriptorHeap* heap,UINT tableFirst,UINT srvSlot,PTSceneDescriptors::Acceleration& result,SelectionDiagnostic* diagnostic=nullptr){
        SelectionDiagnostic local;auto& info=diagnostic?*diagnostic:local;info={};
        result={};if(!Active()||!heap||srvSlot>=36)return false;
        const auto d=heap->GetDesc();const U64 index=U64(tableFirst)+5+srvSlot;
        info.reason="descriptor_heap_or_slot";
        if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||index>=d.NumDescriptors)return false;
        const U64 base=heap->GetCPUDescriptorHandleForHeapStart().ptr;
        info.reason="descriptor_handle_overflow";if(index>(UINT64_MAX-base)/step)return false;
        info.handle=base+index*step;
        const auto selected=descriptors->Read(info.handle,&info.reason);
        info.address=selected.address;info.token=selected.token;info.bytes=selected.bytes;
        if(!selected.token)return false;
        PTOwnedScene::Allocation allocation{};
        info.reason="descriptor_owner_missing";
        if(!resources.Identify(selected.address,selected.bytes,allocation))return false;
        info.currentToken=allocation.token;info.reason="descriptor_owner_generation";
        if(allocation.token!=selected.token)return false;
        info.reason="none";result=selected;return true;
    }
    PTOwnedScene::Runtime& Resources(){return resources;}
    Capture& Scenes(){return capture;}
};
inline Owner owner;
}
