#pragma once
// Independent pipeline preparation only. No diagnostic DispatchRays is recorded
// until exact native ray inputs, private descriptors and GPU ownership are wired.
namespace rt_spot_hit_pipeline {
#include "../build/rt_spot_hit_capture.h"
inline const GUID kPipeline={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0x00,0x00,0x00,0x00,0x01}};
inline const GUID kIdentifiers={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0x00,0x00,0x00,0x00,0x02}};
inline HRESULT Prepare(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* native,ID3D12Object* owner) noexcept {
    const D3D12_DXIL_LIBRARY_DESC* library=nullptr;
    const D3D12_GLOBAL_ROOT_SIGNATURE* global=nullptr;
    const D3D12_LOCAL_ROOT_SIGNATURE* local=nullptr;
    unsigned libraries=0,globals=0,locals=0,opaque=0,alpha=0;
    for(UINT i=0;i<native->NumSubobjects;++i) {
        const auto& object=native->pSubobjects[i];if(!object.pDesc)return E_INVALIDARG;
        if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY){library=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc);++libraries;}
        if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE){global=static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(object.pDesc);++globals;}
        if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE){local=static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(object.pDesc);++locals;}
        if(object.Type==D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP) {
            const auto& group=*static_cast<const D3D12_HIT_GROUP_DESC*>(object.pDesc);
            if(!group.HitGroupExport || !group.ClosestHitShaderImport || group.Type!=D3D12_HIT_GROUP_TYPE_TRIANGLES || group.IntersectionShaderImport)return E_INVALIDARG;
            if(wcscmp(group.ClosestHitShaderImport,L"shadowClosestHit"))return E_INVALIDARG;
            if(!wcscmp(group.HitGroupExport,L"HitGroup_0_0") && !group.AnyHitShaderImport)++opaque;
            else if(!wcscmp(group.HitGroupExport,L"HitGroup_1_0") && group.AnyHitShaderImport && !wcscmp(group.AnyHitShaderImport,L"shadowAlphaTestAnyHit"))++alpha;
            else return E_INVALIDARG;
        }
    }
    if(libraries!=1 || globals!=1 || locals!=1 || opaque!=1 || alpha!=1)return E_INVALIDARG;
    D3D12_EXPORT_DESC nativeAlpha{L"shadowAlphaTestAnyHit",nullptr,D3D12_EXPORT_FLAG_NONE};
    D3D12_DXIL_LIBRARY_DESC nativeLibrary{library->DXILLibrary,1,&nativeAlpha};
    D3D12_DXIL_LIBRARY_DESC captureLibrary{{kSpotHitCaptureShader,sizeof(kSpotHitCaptureShader)},0,nullptr};
    D3D12_HIT_GROUP_DESC groups[2]{};
    groups[0].HitGroupExport=L"CaptureOpaque";groups[1].HitGroupExport=L"CaptureAlpha";
    for(auto& group:groups){group.Type=D3D12_HIT_GROUP_TYPE_TRIANGLES;group.ClosestHitShaderImport=L"captureClosestHit";}
    groups[1].AnyHitShaderImport=L"shadowAlphaTestAnyHit";
    D3D12_RAYTRACING_SHADER_CONFIG config{32,8};
    D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
    D3D12_STATE_SUBOBJECT objects[9]={
        {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&nativeLibrary},
        {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&captureLibrary},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&groups[0]},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&groups[1]},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,global},
        {D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,local},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline},
        {D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,nullptr}
    };
    LPCWSTR localExports[]={L"CaptureOpaque",L"CaptureAlpha"};
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{&objects[6],2,localExports};objects[8].pDesc=&association;
    D3D12_STATE_OBJECT_DESC description{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,9,objects};
    Microsoft::WRL::ComPtr<ID3D12StateObject> capture;
    HRESULT result=rt_shadow_capture::originalState(device,&description,IID_PPV_ARGS(&capture));
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
    if(SUCCEEDED(result))result=capture.As(&properties);
    unsigned char identifiers[4][32]{};
    LPCWSTR exports[]={L"CaptureOpaque",L"CaptureAlpha",L"captureRayGeneration",L"captureMiss"};
    for(unsigned i=0;i<4 && SUCCEEDED(result);++i) {
        const auto* id=properties->GetShaderIdentifier(exports[i]);
        if(!id)result=E_FAIL;else std::memcpy(identifiers[i],id,32);
    }
    if(SUCCEEDED(result))result=owner->SetPrivateDataInterface(kPipeline,capture.Get());
    if(SUCCEEDED(result))result=owner->SetPrivateData(kIdentifiers,sizeof(identifiers),identifiers);
    Trace("rt_hit_pipeline ready=%u hr=0x%08lX payload=32 attributes=8 native_alpha=1 isolated_state=1 gpu_hit_capture=0",unsigned(SUCCEEDED(result)),static_cast<unsigned long>(result));
    return result;
}
}
