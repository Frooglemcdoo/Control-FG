#pragma once
#include <d3d12.h>
#include "pt_scene_dispatch_plan.h"
#include <wrl/client.h>
// Private 60-byte payload pipeline. The global root is supplied by the admitted
// native pass; local roots and hit identifiers never reuse the native payload ABI.
namespace PTScenePipeline {
using Microsoft::WRL::ComPtr;
class Pipeline {
    ComPtr<ID3D12RootSignature> global,local;
    ComPtr<ID3D12StateObject> state;
    ComPtr<ID3D12StateObjectProperties> properties;
    std::array<PTShaderTableAdapter::Identifier,4> hits{};
    PTShaderTableAdapter::Identifier raygen{},surfaceMiss{},shadowMiss{},callable{};
    bool hasCallable=false;
    bool Identifier(const wchar_t* name,PTShaderTableAdapter::Identifier& result){
        const void* id=properties->GetShaderIdentifier(name);if(!id)return false;std::memcpy(result.data(),id,result.size());return true;
    }
public:
    static constexpr UINT PayloadBytes=60,AttributeBytes=8,HitStride=128;
    const char* failureStage="not_started";HRESULT deviceBefore=S_OK,deviceAfter=S_OK;
    HRESULT Initialize(ID3D12Device5* device,ID3D12RootSignature* root,const void* bytes,std::size_t size,const wchar_t* rayGeneration,const wchar_t* callableName=nullptr){
        if(state||!device||!root||!bytes||!size||!rayGeneration)return E_INVALIDARG;
        failureStage="device_before_initialize";deviceBefore=device->GetDeviceRemovedReason();deviceAfter=deviceBefore;
        if(FAILED(deviceBefore))return deviceBefore;
        failureStage="serialize_local_root";
        D3D12_ROOT_PARAMETER params[4]{};
        for(UINT i=0;i<3;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[i].Descriptor={i,3};}
        params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[3].Constants={0,3,12};
        D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=4;desc.pParameters=params;desc.Flags=D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        ComPtr<ID3DBlob> serialized,error;
        HRESULT hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error);if(FAILED(hr))return hr;
        ComPtr<ID3D12RootSignature> createdLocal;
        failureStage="create_local_root";
        hr=device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&createdLocal));if(FAILED(hr)){deviceAfter=device->GetDeviceRemovedReason();return hr;}
        const wchar_t* names[]={L"SurfaceOpaque",L"ShadowOpaque",L"SurfaceAlpha",L"ShadowAlpha"};
        D3D12_HIT_GROUP_DESC groups[4]{};
        for(UINT i=0;i<4;++i){groups[i].HitGroupExport=names[i];groups[i].Type=D3D12_HIT_GROUP_TYPE_TRIANGLES;
            groups[i].ClosestHitShaderImport=i%2?L"PTVisibilityClosestHit":L"PTSceneClosestHit";
            if(i>=2)groups[i].AnyHitShaderImport=i%2?L"PTVisibilityAlphaHit":L"PTSceneAlphaHit";
        }
        D3D12_DXIL_LIBRARY_DESC library{};library.DXILLibrary={bytes,size};
        D3D12_RAYTRACING_SHADER_CONFIG config{PayloadBytes,AttributeBytes};D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
        auto* localPointer=createdLocal.Get();
        D3D12_STATE_SUBOBJECT objects[10]{};objects[0]={D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library};
        for(UINT i=0;i<4;++i)objects[1+i]={D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&groups[i]};
        objects[5]={D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config};
        objects[6]={D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&root};
        objects[7]={D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&localPointer};
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{&objects[7],4,names};
        objects[8]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association};
        objects[9]={D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline};
        const D3D12_STATE_OBJECT_DESC stateDesc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,10,objects};
        failureStage="device_before_state_object";deviceBefore=device->GetDeviceRemovedReason();deviceAfter=deviceBefore;
        if(FAILED(deviceBefore))return deviceBefore;
        failureStage="create_state_object";
        ComPtr<ID3D12StateObject> candidate;hr=device->CreateStateObject(&stateDesc,IID_PPV_ARGS(&candidate));
        deviceAfter=device->GetDeviceRemovedReason();if(FAILED(hr))return hr;
        failureStage="state_object_properties";
        hr=candidate.As(&properties);if(FAILED(hr))return hr;
        failureStage="shader_identifiers";
        bool valid=Identifier(rayGeneration,raygen)&&Identifier(L"PTSceneMiss",surfaceMiss)&&Identifier(L"PTVisibilityMiss",shadowMiss);
        for(UINT i=0;i<4;++i)valid=valid&&Identifier(names[i],hits[i]);
        if(callableName)valid=valid&&Identifier(callableName,callable);
        if(!valid){properties.Reset();return E_FAIL;}
        hasCallable=callableName!=nullptr;failureStage="ready";
        state=std::move(candidate);global=root;local=std::move(createdLocal);return S_OK;
    }
    ID3D12StateObject* State()const{return state.Get();}
    ID3D12RootSignature* Root()const{return global.Get();}
    const PTShaderTableAdapter::Identifier& SurfaceAlpha()const{return hits[2];}
    bool HasCallable()const{return hasCallable;}
    const PTShaderTableAdapter::Identifier& Callable()const{return callable;}
    const PTShaderTableAdapter::Identifier& RayGeneration()const{return raygen;}
    const PTShaderTableAdapter::Identifier& SurfaceMiss()const{return surfaceMiss;}
    const PTShaderTableAdapter::Identifier& ShadowMiss()const{return shadowMiss;}
    bool Exports(std::uint64_t nativePipeline,const std::array<PTShaderTableAdapter::Identifier,4>& native,PTSceneDispatch::ExportSet& out)const{
        out={};if(!state||!nativePipeline)return false;
        for(const auto& id:native)if(id==PTShaderTableAdapter::Identifier{})return false;
        if(native[0]==native[2]||native[1]==native[3])return false;
        out.nativePipeline=nativePipeline;out.privatePipeline=reinterpret_cast<UINT_PTR>(state.Get());out.rays.resize(2);
        for(UINT i=0;i<2;++i)out.rays[i]={native[i],native[i+2],hits[i],hits[i+2]};return true;
    }
};
}
