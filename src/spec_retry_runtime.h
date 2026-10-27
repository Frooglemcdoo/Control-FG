#pragma once
// Included inside native_lighting. Creation-only substitution for the exact
// native reflection library observed in the R175 capture; unknown variants pass.
namespace spec_retry {
#include "../build/spec_retry_reuse.generated.h"
#ifdef CONTROL_SPEC_PHASE_PRECISION
#include "../build/spec_phase_precision.generated.h"
#endif
using Create = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
inline std::atomic<unsigned long long> matched{0},created{0},failures{0};
inline HRESULT CreateState(Create next,ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    if(!desc||!out||!desc->pSubobjects||desc->Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
        return next(device,desc,iid,out);
    UINT index=desc->NumSubobjects;
    for(UINT i=0;i<desc->NumSubobjects;++i){
        const auto& object=desc->pSubobjects[i];
        if(object.Type!=D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY||!object.pDesc)continue;
        const auto& code=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc)->DXILLibrary;
        if(!code.pShaderBytecode||code.BytecodeLength!=39008u)continue;
        char sha[65]{};
        if(PTReflectionSha256(static_cast<const unsigned char*>(code.pShaderBytecode),code.BytecodeLength,sha)&&
           !std::strcmp(sha,"c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e")){
            // Multiple matching libraries are not an observed/validated layout.
            if(index!=desc->NumSubobjects)return next(device,desc,iid,out);
            index=i;
        }
    }
    if(index==desc->NumSubobjects)return next(device,desc,iid,out);
    ++matched;
    try {
        std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        auto library=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(objects[index].pDesc);
        library.DXILLibrary={kSpecRetryReuseShader,sizeof(kSpecRetryReuseShader)};
#ifdef CONTROL_SPEC_PHASE_PRECISION
        library.DXILLibrary={kSpecPhasePrecisionShader,sizeof(kSpecPhasePrecisionShader)};
#endif
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
            Trace("spec_retry_create build=R190 created=1 variant=0 cache_values=4 rays=unchanged");
#ifdef CONTROL_SPEC_PHASE_PRECISION
            Trace("SPEC_PHASE_PRECISION build=R190 applied=1 native_fallback=0 fractional_bits=24 sequence=native_r2 rays=unchanged weights=unchanged");
#endif
            return hr;
        }
        if(candidate)static_cast<IUnknown*>(candidate)->Release();
        ++failures;
        Trace("spec_retry_create build=R190 created=0 hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));
    }catch(...){++failures;Trace("spec_retry_create build=R190 exception=1 native_fallback=1");}
    return next(device,desc,iid,out);
}
}
