// CPU-only prerequisite for private third-hit storage. Native API arguments,
// shader code, resource bindings and returned HRESULTs remain unchanged.
#pragma once

static const GUID kPTAuditRootBytes={0x5e163d71,0x2b4a,0x49f7,{0xa3,0x81,0x6f,0xc2,0x9a,0x84,0x17,0x50}};
static const GUID kPTAuditStateTarget={0x68d9ac52,0x83ef,0x420a,{0xb4,0x8f,0x03,0xd9,0xab,0xe2,0x25,0x1f}};
static const GUID kPTAuditRootLogged={0x45e68f30,0xd19e,0x4df5,{0x93,0x48,0xbb,0x0e,0x53,0x6a,0x2e,0x71}};
using PTAuditCreateRootFn=HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const void*,SIZE_T,REFIID,void**);
using PTAuditCreateStateFn=HRESULT (STDMETHODCALLTYPE*)(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
static PTAuditCreateRootFn ptAuditCreateRoot=nullptr;
static PTAuditCreateStateFn ptAuditCreateState=nullptr;
static SRWLOCK ptAuditInstallLock=SRWLOCK_INIT;
static std::atomic<unsigned> ptAuditStates{0u};

static void PTAuditDescribeRoot(ID3D12RootSignature* root,const char* role,unsigned state) {
    if(!root)return;
    UINT logged=0,loggedSize=sizeof(logged);
    if(SUCCEEDED(root->GetPrivateData(kPTAuditRootLogged,&loggedSize,&logged))&&logged){
        Log("PT_THIRD_ROOT_REF state=%u role=%s root=%p",state,role,root);
        return;
    }
    UINT bytes=0;
    HRESULT hr=root->GetPrivateData(kPTAuditRootBytes,&bytes,nullptr);
    if(FAILED(hr)||bytes<32u||bytes>65536u){
        Log("PT_THIRD_ROOT state=%u role=%s root=%p ready=0 reason=serialized_root_unavailable hr=0x%08lX",state,role,root,hr);
        return;
    }
    std::vector<unsigned char> blob(bytes);
    hr=root->GetPrivateData(kPTAuditRootBytes,&bytes,blob.data());
    using DeserializeFn=HRESULT (WINAPI*)(const void*,SIZE_T,REFIID,void**);
    auto deserialize=reinterpret_cast<DeserializeFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12CreateVersionedRootSignatureDeserializer"));
    ID3D12VersionedRootSignatureDeserializer* decoder=nullptr;
    if(SUCCEEDED(hr)&&deserialize)
        hr=deserialize(blob.data(),bytes,__uuidof(ID3D12VersionedRootSignatureDeserializer),reinterpret_cast<void**>(&decoder));
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* version=nullptr;
    if(decoder)hr=decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1,&version);
    if(!decoder||FAILED(hr)||!version){
        Log("PT_THIRD_ROOT state=%u role=%s root=%p ready=0 reason=decode hr=0x%08lX",state,role,root,hr);
        if(decoder)decoder->Release();
        return;
    }
    const auto& desc=version->Desc_1_1;
    unsigned cost=0;
    for(UINT i=0;i<desc.NumParameters;++i){
        const auto& p=desc.pParameters[i];
        cost+=p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE?1u:
            p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS?p.Constants.Num32BitValues:2u;
    }
    char sha[65]{};PTReflectionSha256(blob.data(),bytes,sha);
    Log("PT_THIRD_ROOT state=%u role=%s root=%p ready=1 parameters=%u static_samplers=%u flags=0x%X dwords=%u append_buffer_dwords=2 room=%u sha256=%s",
        state,role,root,desc.NumParameters,desc.NumStaticSamplers,unsigned(desc.Flags),cost,unsigned(cost<=62u),sha);
    for(UINT i=0;i<desc.NumParameters;++i){
        const auto& p=desc.pParameters[i];
        if(p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE){
            Log("PT_THIRD_TABLE root=%p param=%u visibility=%u ranges=%u",root,i,unsigned(p.ShaderVisibility),p.DescriptorTable.NumDescriptorRanges);
            for(UINT j=0;j<p.DescriptorTable.NumDescriptorRanges;++j){
                const auto& r=p.DescriptorTable.pDescriptorRanges[j];
                Log("PT_THIRD_RANGE root=%p param=%u range=%u type=%u count=%u reg=%u space=%u offset=%u flags=0x%X",
                    root,i,j,unsigned(r.RangeType),r.NumDescriptors,r.BaseShaderRegister,r.RegisterSpace,r.OffsetInDescriptorsFromTableStart,unsigned(r.Flags));
            }
        }else if(p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS){
            Log("PT_THIRD_CONSTANTS root=%p param=%u visibility=%u reg=%u space=%u values=%u",root,i,unsigned(p.ShaderVisibility),p.Constants.ShaderRegister,p.Constants.RegisterSpace,p.Constants.Num32BitValues);
        }else{
            Log("PT_THIRD_DESCRIPTOR root=%p param=%u type=%u visibility=%u reg=%u space=%u flags=0x%X",root,i,unsigned(p.ParameterType),unsigned(p.ShaderVisibility),p.Descriptor.ShaderRegister,p.Descriptor.RegisterSpace,unsigned(p.Descriptor.Flags));
        }
    }
    for(UINT i=0;i<desc.NumStaticSamplers;++i){
        const auto& s=desc.pStaticSamplers[i];
        Log("PT_THIRD_SAMPLER root=%p reg=%u space=%u visibility=%u filter=%u address=%u_%u_%u",root,s.ShaderRegister,s.RegisterSpace,unsigned(s.ShaderVisibility),unsigned(s.Filter),unsigned(s.AddressU),unsigned(s.AddressV),unsigned(s.AddressW));
    }
    decoder->Release();
    logged=1;root->SetPrivateData(kPTAuditRootLogged,sizeof(logged),&logged);
}

static HRESULT STDMETHODCALLTYPE PTAuditHookCreateRoot(ID3D12Device* device,UINT node,
    const void* data,SIZE_T bytes,REFIID iid,void** out) {
    const HRESULT hr=ptAuditCreateRoot(device,node,data,bytes,iid,out);
    if(SUCCEEDED(hr)&&out&&*out&&data&&bytes>=32u&&bytes<=65536u){
        ID3D12RootSignature* root=nullptr;
        if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(__uuidof(ID3D12RootSignature),reinterpret_cast<void**>(&root)))){
            root->SetPrivateData(kPTAuditRootBytes,static_cast<UINT>(bytes),data);
            root->Release();
        }
    }
    return hr;
}

static HRESULT PTThirdCreateState(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
static void PTThirdInstallDevice(ID3D12Device*);

static HRESULT STDMETHODCALLTYPE PTAuditHookCreateState(ID3D12Device5* device,
    const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    const HRESULT hr=PTThirdCreateState(device,desc,iid,out);
    // State creation uses the same unmodified descriptor, even on audit failure.
    try {
        bool target=false;
        if(desc&&desc->NumSubobjects<=4096u&&desc->pSubobjects){
            for(UINT i=0;i<desc->NumSubobjects;++i){
                const auto& s=desc->pSubobjects[i];
                if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION&&s.pDesc){
                    auto* collection=static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(s.pDesc)->pExistingCollection;
                    UINT tagged=0,size=sizeof(tagged);
                    if(collection&&SUCCEEDED(collection->GetPrivateData(kPTAuditStateTarget,&size,&tagged))&&tagged)target=true;
                }
                if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY&&s.pDesc){
                    const auto& lib=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(s.pDesc);
                    if(lib.DXILLibrary.BytecodeLength==ptReflectionP2Bytes.size()&&
                       !ptReflectionP2Bytes.empty()&&lib.DXILLibrary.pShaderBytecode&&
                       !memcmp(lib.DXILLibrary.pShaderBytecode,ptReflectionP2Bytes.data(),ptReflectionP2Bytes.size()))target=true;
                }
            }
        }
        if(target){
            if(SUCCEEDED(hr)&&out&&*out){
                ID3D12StateObject* object=nullptr;
                if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(__uuidof(ID3D12StateObject),reinterpret_cast<void**>(&object)))){
                    UINT tagged=1;object->SetPrivateData(kPTAuditStateTarget,sizeof(tagged),&tagged);object->Release();
                }
            }
            const unsigned state=ptAuditStates.fetch_add(1)+1u;
            if(state<=8u){
                Log("PT_THIRD_STATE state=%u hr=0x%08lX object=%p type=%u subobjects=%u third_bounce_requested=1",state,hr,(out?*out:nullptr),unsigned(desc->Type),desc->NumSubobjects);
                for(UINT i=0;i<desc->NumSubobjects;++i){
                    const auto& s=desc->pSubobjects[i];if(!s.pDesc)continue;
                    if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE)
                        PTAuditDescribeRoot(static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(s.pDesc)->pGlobalRootSignature,"ray_global",state);
                    else if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE)
                        PTAuditDescribeRoot(static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(s.pDesc)->pLocalRootSignature,"ray_local",state);
                    else if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG){
                        const auto& c=*static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(s.pDesc);
                        Log("PT_THIRD_SHADER_CONFIG state=%u payload=%u attributes=%u",state,c.MaxPayloadSizeInBytes,c.MaxAttributeSizeInBytes);
                    }else if(s.Type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG){
                        const auto& c=*static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(s.pDesc);
                        Log("PT_THIRD_PIPELINE_CONFIG state=%u recursion=%u",state,c.MaxTraceRecursionDepth);
                    }
                }
            }
        }
    }catch(...){Log("PT_THIRD_AUDIT exception=cpp native_result_preserved=1");}
    return hr;
}

static bool PTAuditPatch(void** slot,void* hook,void** original) {
    if(*slot==hook)return *original!=nullptr;
    if(*original&&*original!=*slot)return false;
    DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
    void* expected=*slot;
    *original=expected; // Publish before the hook can be called on another thread.
    MemoryBarrier();
    void* previous=InterlockedCompareExchangePointer(slot,hook,expected);
    DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);
    return previous==expected;
}

static void PTAuditInstall(ID3D12Device* device) {
    if(!device)return;
    AcquireSRWLockExclusive(&ptAuditInstallLock);
    bool rootReady=PTAuditPatch(&(*reinterpret_cast<void***>(device))[16],
        reinterpret_cast<void*>(&PTAuditHookCreateRoot),reinterpret_cast<void**>(&ptAuditCreateRoot));
    ID3D12Device5* rayDevice=nullptr;
    bool rayReady=false;
    if(SUCCEEDED(device->QueryInterface(__uuidof(ID3D12Device5),reinterpret_cast<void**>(&rayDevice)))){
        // ID3D12Device5::CreateStateObject, inherited IUnknown/Device0..4 slots.
        rayReady=PTAuditPatch(&(*reinterpret_cast<void***>(rayDevice))[62],
            reinterpret_cast<void*>(&PTAuditHookCreateState),reinterpret_cast<void**>(&ptAuditCreateState));
        rayDevice->Release();
    }
    ReleaseSRWLockExclusive(&ptAuditInstallLock);
    PTThirdInstallDevice(device);
    Log("PT_THIRD_AUDIT_INSTALL root_capture=%u state_capture=%u third_bounce_requested=1",unsigned(rootReady),unsigned(rayReady));
}
