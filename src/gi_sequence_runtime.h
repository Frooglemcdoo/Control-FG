#pragma once
// Diagnostic creation-time replacement: applies to GI in every mode for this build.
namespace gi_sequence {
#include "../build/gi_sequence.generated.h"
using Create = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
inline std::atomic<unsigned long long> matched{0},created{0},failures{0};
inline HRESULT CreateState(Create next,ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    if(!desc||!out||!desc->pSubobjects||desc->Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
        return next(device,desc,iid,out);
    UINT index=desc->NumSubobjects;unsigned variant=0;
    for(UINT i=0;i<desc->NumSubobjects;++i){
        const auto& object=desc->pSubobjects[i];
        if(object.Type!=D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY||!object.pDesc)continue;
        const auto& code=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc)->DXILLibrary;
        if(!code.pShaderBytecode||(code.BytecodeLength!=24472u&&code.BytecodeLength!=27904u))continue;
        char sha[65]{};unsigned v=code.BytecodeLength==24472u?0u:1u;
        const char* expected=v==0?"6297319e1fec315c06ee75993e797b51def658e7d41f1588d7df3feb41a1ea2f":"1ff8a93460d94f58fc0e70afff994a334f1c45be292e3e858a63f51070e5734a";
        if(PTReflectionSha256(static_cast<const unsigned char*>(code.pShaderBytecode),code.BytecodeLength,sha)&&!std::strcmp(sha,expected)){
            if(index!=desc->NumSubobjects)return next(device,desc,iid,out);
            index=i;variant=v;
        }
    }
    if(index==desc->NumSubobjects)return next(device,desc,iid,out);
    ++matched;
    try {
        std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        auto library=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(objects[index].pDesc);
        library.DXILLibrary=variant==0?D3D12_SHADER_BYTECODE{kGISequenceShader0,sizeof(kGISequenceShader0)}:D3D12_SHADER_BYTECODE{kGISequenceShader1,sizeof(kGISequenceShader1)};
        objects[index].pDesc=&library;
        // Associations point INTO the descriptor array; rebase every one.
        for(UINT i=0;i<desc->NumSubobjects;++i){
            if(objects[i].Type!=D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)continue;
            if(!objects[i].pDesc)return next(device,desc,iid,out);
            associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
            UINT target=desc->NumSubobjects;
            for(UINT j=0;j<desc->NumSubobjects;++j)
                if(associations[i].pSubobjectToAssociate==desc->pSubobjects+j){target=j;break;}
            if(target==desc->NumSubobjects)return next(device,desc,iid,out);
            associations[i].pSubobjectToAssociate=objects.data()+target;
            objects[i].pDesc=&associations[i];
        }
        auto replacement=*desc;replacement.pSubobjects=objects.data();
        void* candidate=nullptr;
        const HRESULT hr=next(device,&replacement,iid,&candidate);
        if(SUCCEEDED(hr)&&candidate){
            *out=candidate;++created;
            Log("GI_SEQUENCE_R264 applied=1 variant=%u frame_mask=255 counter=69z rays=unchanged native_fallback=0 scope=diagnostic_all_gi",variant);
            return hr;
        }
        if(candidate)static_cast<IUnknown*>(candidate)->Release();
        ++failures;
        Log("gi_sequence_create build=R264 created=0 hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));
    }catch(...){++failures;Log("gi_sequence_create build=R264 exception=1 native_fallback=1");}
    return next(device,desc,iid,out);
}
}
