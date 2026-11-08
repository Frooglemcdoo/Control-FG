#pragma once
// Included inside native_lighting after gi_sequence. All history belongs to
// the augmented state object. No scene mirror or CPU readback is involved.
namespace gi_temporal {
using Microsoft::WRL::ComPtr;
#include "../build/gi_temporal.generated.h"
inline constexpr wchar_t exportName[]=L"TemporalGI";
inline const GUID historyKey={0x338e0001,0x8821,0x4421,{0x90,1,2,3,4,5,6,7}};
inline const GUID recordKey={0x338e0002,0x8821,0x4421,{0x90,1,2,3,4,5,6,7}};
inline const GUID queueKey={0x338e0003,0x8821,0x4421,{0x90,1,2,3,4,5,6,7}};
inline SRWLOCK prepareLock=SRWLOCK_INIT;
inline const GUID failedKey={0x338e0004,0x8821,0x4421,{0x90,1,2,3,4,5,6,7}};

inline HRESULT Create(gi_sequence::Create next,ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    if(!desc || !desc->NumSubobjects || !desc->pSubobjects)return next(device,desc,iid,out);
    const auto& last=desc->pSubobjects[desc->NumSubobjects-1];
    if(last.Type!=D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY || !last.pDesc)return next(device,desc,iid,out);
    const auto code=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(last.pDesc)->DXILLibrary.pShaderBytecode;
    const bool second=code==gi_sequence::kGIOwnedRay1;
    if(!second && code!=gi_sequence::kGIOwnedRay0)return next(device,desc,iid,out);
    try {
        D3D12_ROOT_PARAMETER params[2]{};
        params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[0].Descriptor={0,17};
        params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[1].Constants={0,17,1};
        D3D12_ROOT_SIGNATURE_DESC rd{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
        ComPtr<ID3DBlob> blob;ComPtr<ID3D12RootSignature> local;
        if(FAILED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,nullptr)) ||
           FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&local))))
            return next(device,desc,iid,out);
        const UINT count=desc->NumSubobjects;
        std::vector<D3D12_STATE_SUBOBJECT> subs(desc->pSubobjects,desc->pSubobjects+count);
        subs.reserve(count+3);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(count);
        std::vector<std::vector<LPCWSTR>> names(count);
        for(UINT i=0;i<count;++i){
            if(subs[i].Type!=D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)continue;
            associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subs[i].pDesc);
            auto& a=associations[i];UINT target=0;
            while(target<count && a.pSubobjectToAssociate!=desc->pSubobjects+target)++target;
            if(target==count)return next(device,desc,iid,out);
            if(subs[target].Type!=D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE){
                for(UINT n=0;n<a.NumExports;++n)if(!wcscmp(a.pExports[n],L"controlNewGIRayGen")){
                    names[i].assign(a.pExports,a.pExports+a.NumExports);names[i].push_back(exportName);
                    ++a.NumExports;a.pExports=names[i].data();break;
                }
            }
            a.pSubobjectToAssociate=subs.data()+target;subs[i].pDesc=&a;
        }
        D3D12_EXPORT_DESC entry{exportName,nullptr,D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC lib{second?D3D12_SHADER_BYTECODE{kTemporal1,sizeof(kTemporal1)}:
            D3D12_SHADER_BYTECODE{kTemporal0,sizeof(kTemporal0)},1,&entry};
        ID3D12RootSignature* root=local.Get();LPCWSTR name=exportName;
        subs.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib});
        subs.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&root});
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc{&subs.back(),1,&name};
        subs.push_back({D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&assoc});
        auto augmented=*desc;augmented.NumSubobjects=UINT(subs.size());augmented.pSubobjects=subs.data();
        void* candidate=nullptr;const auto hr=next(device,&augmented,iid,&candidate);
        if(SUCCEEDED(hr)&&candidate){*out=candidate;return hr;}
        if(candidate)static_cast<IUnknown*>(candidate)->Release();
        Log("GI_TEMPORAL_R338 state_fallback=1 hr=0x%08lX",static_cast<unsigned long>(hr));
    }catch(...){}
    return next(device,desc,iid,out);
}
template<class T> inline ComPtr<T> Read(ID3D12StateObject* state,const GUID& key) noexcept {
    ComPtr<T> result;UINT bytes=sizeof(T*);state->GetPrivateData(key,&bytes,result.GetAddressOf());return result;
}
inline ComPtr<ID3D12Resource> Prepare(ID3D12StateObject* state,ID3D12CommandQueue* queue,UINT w,UINT h) noexcept {
    ComPtr<ID3D12StateObjectProperties> props;ComPtr<ID3D12Device> device;
    if(FAILED(state->QueryInterface(IID_PPV_ARGS(&props)))||FAILED(state->GetDevice(IID_PPV_ARGS(&device))))return {};
    const auto identifier=props->GetShaderIdentifier(exportName);if(!identifier)return {};
    const UINT capacity=(std::min)(1048576u,((w+1)/2)*((h+1)/2));
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Alignment=0;
    buffer.Width=UINT64(capacity)*80;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;
    buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;buffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
    ComPtr<ID3D12Resource> history,record;
    if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&history))))return {};
    hp.Type=D3D12_HEAP_TYPE_UPLOAD;buffer.Width=64;buffer.Flags=D3D12_RESOURCE_FLAG_NONE;
    if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&record))))return {};
    void* address=nullptr;D3D12_RANGE empty{0,0};
    if(FAILED(record->Map(0,&empty,&address)))return {};
    auto* data=static_cast<unsigned char*>(address);memset(data,0,64);memcpy(data,identifier,32);
    const auto gpu=history->GetGPUVirtualAddress();memcpy(data+32,&gpu,8);memcpy(data+40,&capacity,4);record->Unmap(0,nullptr);
    if(FAILED(state->SetPrivateDataInterface(historyKey,history.Get())) || FAILED(state->SetPrivateDataInterface(queueKey,queue)) ||
       FAILED(state->SetPrivateDataInterface(recordKey,record.Get())))return {};
    Log("GI_TEMPORAL_R338 history_ready=1 cells=%u bytes=%llu",capacity,UINT64(capacity)*80);
    return record;
}
inline void Record(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state,const D3D12_DISPATCH_RAYS_DESC& desc) noexcept {
    if(!desc.Width||!desc.Height||desc.Width>16384||desc.Height>16384||desc.Depth!=1)return;
    PTGlassRuntime::Context ctx{};
    if(!PTGlassRuntime::ReadContext(&ctx)||ctx.rays!=list)return;
    auto record=Read<ID3D12Resource>(state,recordKey);
    if(!record){
        AcquireSRWLockExclusive(&prepareLock);
        record=Read<ID3D12Resource>(state,recordKey);
        if(!record){
            UINT failed=0,bytes=sizeof(failed);
            if(FAILED(state->GetPrivateData(failedKey,&bytes,&failed))){
                record=Prepare(state,ctx.queue,desc.Width,desc.Height);
                if(!record){failed=1;state->SetPrivateData(failedKey,sizeof(failed),&failed);Log("GI_TEMPORAL_R338 history_unavailable=1");}
            }
        }
        ReleaseSRWLockExclusive(&prepareLock);
    }
    if(!record||Read<ID3D12CommandQueue>(state,queueKey).Get()!=ctx.queue)return;
    // Order the engine's hit UAV writes and the two history planes on the
    // same verified game queue. Downstream GI6 then sees the accepted writes.
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
    list->ResourceBarrier(1,&barrier);
    auto temporal=desc;temporal.RayGenerationShaderRecord={record->GetGPUVirtualAddress(),64};
    rt_shadow_capture::Lookup(list).dispatch(list,&temporal);
    list->ResourceBarrier(1,&barrier);
    settings_check::Record(settings_check::GIReuse);
    ++gi_reuse::temporalDispatches;
}
}
