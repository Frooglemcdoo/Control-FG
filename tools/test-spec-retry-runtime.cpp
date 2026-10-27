#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>
static bool hashMatches=true;
static bool PTReflectionSha256(const unsigned char*,size_t,char* out){std::strcpy(out,hashMatches?"c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e":"unknown");return true;}
static void Trace(const char*,...){}
#include "../src/spec_retry_runtime.h"
static const D3D12_STATE_OBJECT_DESC* nativeDesc=nullptr;
static unsigned nativeCalls=0,replacementCalls=0;
static bool reject=false;
static void Require(bool value){if(!value)throw std::runtime_error("SPEC retry creation contract failed");}
static HRESULT STDMETHODCALLTYPE Next(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC* desc,REFIID,void** out){
    if(desc==nativeDesc){++nativeCalls;*out=reinterpret_cast<void*>(1);return S_OK;}
    ++replacementCalls;
    Require(desc->NumSubobjects==3&&desc->Type==nativeDesc->Type);
    const auto& lib=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[0].pDesc);
    const auto& nativeLib=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(nativeDesc->pSubobjects[0].pDesc);
#ifdef CONTROL_SPEC_PHASE_PRECISION
    Require(lib.DXILLibrary.pShaderBytecode==spec_retry::kSpecPhasePrecisionShader);
#else
    Require(lib.DXILLibrary.pShaderBytecode==spec_retry::kSpecRetryReuseShader);
#endif
    Require(lib.NumExports==nativeLib.NumExports&&lib.pExports==nativeLib.pExports);
    const auto& assoc=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[2].pDesc);
    Require(assoc.pSubobjectToAssociate==desc->pSubobjects+1);
    Require(desc->pSubobjects[1].pDesc==nativeDesc->pSubobjects[1].pDesc);
    *out=reject?nullptr:reinterpret_cast<void*>(2);return reject?E_FAIL:S_OK;
}
int main(){try{
    unsigned char source=0;
    D3D12_EXPORT_DESC exportDesc{L"reflectionRayGeneration",nullptr,D3D12_EXPORT_FLAG_NONE};
    D3D12_DXIL_LIBRARY_DESC library{{&source,39008},1,&exportDesc};
    D3D12_RAYTRACING_SHADER_CONFIG config{4,8};
    D3D12_STATE_SUBOBJECT objects[3]={{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library},{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config},{D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,nullptr}};
    LPCWSTR name=L"reflectionRayGeneration";
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc{objects+1,1,&name};objects[2].pDesc=&assoc;
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,3,objects};nativeDesc=&desc;
    void* out=nullptr;
    auto run=[&](){Require(SUCCEEDED(spec_retry::CreateState(&Next,nullptr,&desc,__uuidof(ID3D12StateObject),&out)));};
    run();Require(out==reinterpret_cast<void*>(2)&&replacementCalls==1&&nativeCalls==0);
    Require(library.DXILLibrary.pShaderBytecode==&source&&assoc.pSubobjectToAssociate==objects+1);
    reject=true;run();Require(out==reinterpret_cast<void*>(1)&&replacementCalls==2&&nativeCalls==1&&spec_retry::failures==1);
    reject=false;hashMatches=false;run();Require(replacementCalls==2&&nativeCalls==2);
    hashMatches=true;library.DXILLibrary.BytecodeLength=39007;run();Require(replacementCalls==2&&nativeCalls==3);
    library.DXILLibrary.BytecodeLength=39008;assoc.pSubobjectToAssociate=nullptr;run();Require(replacementCalls==2&&nativeCalls==4);
    assoc.pSubobjectToAssociate=objects+1;desc.Type=D3D12_STATE_OBJECT_TYPE_COLLECTION;run();Require(replacementCalls==2&&nativeCalls==5);
    Require(spec_retry::created==1);
    std::puts("PASS: exact-match substitution, association rebasing, unchanged input, failed creation fallback, unknown/size/layout/type passthrough");
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
