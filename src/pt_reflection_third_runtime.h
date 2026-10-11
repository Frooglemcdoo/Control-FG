#pragma once
// Private storage for two secondary vertices. The hash-verified native root
// has four tables (0,1,6,7) and four CBVs (2..5); no constants to replay.
static const GUID kPTThirdState={0x8860bd18,0x6b4c,0x40c0,{0xaa,0x5a,0x37,0xbe,0xc2,0x08,0x7f,0x21}};
static SRWLOCK ptThirdLock=SRWLOCK_INIT;
static SRWLOCK ptThirdInstallLock=SRWLOCK_INIT;
static ID3D12RootSignature* ptThirdNativeRoot=nullptr;
static ID3D12RootSignature* ptThirdRoot=nullptr;
static ID3D12Resource* ptThirdHits=nullptr;
static std::atomic<unsigned long long> ptThirdDispatches{0};
static std::atomic<unsigned long long> ptThirdWrittenFrame{~0ull},ptThirdConsumed{0},ptThirdRawDispatches{0};
static constexpr UINT64 kPTThirdBytes=8388608ull*64ull;

struct PTThirdListState {
    ID3D12GraphicsCommandList* list=nullptr;
    ID3D12GraphicsCommandList* alias=nullptr;
    ID3D12RootSignature* root=nullptr;
    UINT64 args[8]{};
    unsigned valid=0;
    bool target=false;
};
// Reset invalidates entries before a recycled command list can use old values.
static PTThirdListState ptThirdLists[2048]{};
static PTThirdListState* PTThirdFindLocked(ID3D12GraphicsCommandList* list,bool create) {
    PTThirdListState* empty=nullptr;
    for(auto& s:ptThirdLists){if(s.list==list||s.alias==list)return &s;if(!s.list&&!empty)empty=&s;}
    if(create&&empty){*empty={};empty->list=list;return empty;}
    return nullptr;
}
static void PTThirdTrackRoot(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root) {
    AcquireSRWLockExclusive(&ptThirdLock);
    auto* s=PTThirdFindLocked(list,true);
    if(s&&s->root!=root){s->root=root;s->valid=0;}
    ReleaseSRWLockExclusive(&ptThirdLock);
}
static bool PTThirdSnapshot(ID3D12GraphicsCommandList* list,PTThirdListState* out,unsigned required=0xfdu) {
    AcquireSRWLockShared(&ptThirdLock);
    auto* s=PTThirdFindLocked(list,false);
    bool okay=s&&s->root==ptThirdNativeRoot&&(s->valid&required)==required;
    if(okay)*out=*s;
    ReleaseSRWLockShared(&ptThirdLock);
    return okay;
}

static ID3D12RootSignature* PTThirdGetRoot(ID3D12Device* device,ID3D12RootSignature* native) {
    if(!device||!native)return nullptr;
    AcquireSRWLockExclusive(&ptThirdLock);
    if(ptThirdRoot){auto* r=ptThirdNativeRoot==native?ptThirdRoot:nullptr;ReleaseSRWLockExclusive(&ptThirdLock);return r;}
    ID3D12VersionedRootSignatureDeserializer* decoder=nullptr;
    ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
    HRESULT hr=E_FAIL;
    try {
        UINT bytes=0;native->GetPrivateData(kPTAuditRootBytes,&bytes,nullptr);
        if(bytes<32||bytes>65536)throw 1;
        std::vector<unsigned char> data(bytes);
        if(FAILED(native->GetPrivateData(kPTAuditRootBytes,&bytes,data.data())))throw 2;
        char sha[65]{};
        if(!PTReflectionSha256(data.data(),bytes,sha)||strcmp(sha,"451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3"))throw 3;
        using Decode=HRESULT (WINAPI*)(const void*,SIZE_T,REFIID,void**);
        using Encode=HRESULT (WINAPI*)(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*,ID3DBlob**,ID3DBlob**);
        auto module=GetModuleHandleW(L"d3d12.dll");
        auto decode=reinterpret_cast<Decode>(GetProcAddress(module,"D3D12CreateVersionedRootSignatureDeserializer"));
        auto encode=reinterpret_cast<Encode>(GetProcAddress(module,"D3D12SerializeVersionedRootSignature"));
        if(!decode||!encode)throw 4;
        hr=decode(data.data(),bytes,__uuidof(ID3D12VersionedRootSignatureDeserializer),reinterpret_cast<void**>(&decoder));
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* original=nullptr;
        if(FAILED(hr)||!decoder||FAILED(decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1,&original)))throw 5;
        if(original->Desc_1_1.NumParameters!=8u)throw 6;
        D3D12_ROOT_PARAMETER1 params[10]{};
        memcpy(params,original->Desc_1_1.pParameters,8*sizeof(params[0]));
        params[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[8].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        params[8].Descriptor.ShaderRegister=0;
        params[8].Descriptor.RegisterSpace=99;
        params[8].Descriptor.Flags=D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
        params[9].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[9].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        params[9].Constants={0,99,1};
        auto desc=*original;desc.Desc_1_1.NumParameters=10;desc.Desc_1_1.pParameters=params;
        hr=encode(&desc,&blob,&errors);
        if(FAILED(hr)||!blob)throw 7;
        hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),__uuidof(ID3D12RootSignature),reinterpret_cast<void**>(&ptThirdRoot));
        if(SUCCEEDED(hr)&&ptThirdRoot){native->AddRef();ptThirdNativeRoot=native;}
    }catch(...){ }
    if(errors)errors->Release();if(blob)blob->Release();if(decoder)decoder->Release();
    auto* result=ptThirdRoot;
    ReleaseSRWLockExclusive(&ptThirdLock);
    Log("PT_THIRD_PRIVATE_ROOT ready=%u native=%p custom=%p hr=0x%08lX parameters=10 root_dwords=15 uav=u0_space99",unsigned(result!=nullptr),native,result,hr);
    return result;
}

static bool PTThirdEnsureBuffer(ID3D12Device* device) {
    AcquireSRWLockExclusive(&ptThirdLock);
    if(ptThirdHits){ReleaseSRWLockExclusive(&ptThirdLock);return true;}
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    heap.CreationNodeMask=1;heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width=kPTThirdBytes;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    HRESULT hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,__uuidof(ID3D12Resource),reinterpret_cast<void**>(&ptThirdHits));
    if(ptThirdHits)ptThirdHits->SetName(L"Control-FG private secondary and third reflection hits");
    bool ready=ptThirdHits!=nullptr;
    ReleaseSRWLockExclusive(&ptThirdLock);
    Log("PT_THIRD_PRIVATE_BUFFER ready=%u hr=0x%08lX bytes=%llu stride=32 records_per_pixel=2 resize_reallocation=0",unsigned(ready),hr,kPTThirdBytes);
    return ready;
}

using PTThirdTableFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using PTThirdCBVFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_VIRTUAL_ADDRESS);
using PTThirdResetFn=HRESULT (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using PTThirdSetStateFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,ID3D12StateObject*);
using PTThirdDispatchFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
using PTThirdCreateListFn=HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_COMMAND_LIST_TYPE,ID3D12CommandAllocator*,ID3D12PipelineState*,REFIID,void**);
// A compute list and a direct list may have different implementations. Keep
// each original function with its owning vtable, never with the first list.
static void STDMETHODCALLTYPE PTReflectionHookSetComputeRootSignature(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
static void STDMETHODCALLTYPE PTReflectionHookSetPipelineState(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
struct PTThirdVtable {
    void** table=nullptr;
    void* original[77]{};
    bool ready=false;
    bool extended=false;
};
static PTThirdVtable ptThirdVtables[32]{};
static SRWLOCK ptThirdVtableLock=SRWLOCK_INIT;
static std::atomic<unsigned> ptThirdDirectReady{0},ptThirdCaptureFailures{0};
static void* PTThirdOriginal(ID3D12GraphicsCommandList* list,unsigned slot) {
    void* result=nullptr;auto table=*reinterpret_cast<void***>(list);
    AcquireSRWLockShared(&ptThirdVtableLock);
    for(auto& v:ptThirdVtables)if(v.table==table){result=v.original[slot];break;}
    ReleaseSRWLockShared(&ptThirdVtableLock);
    return result;
}
static PTThirdCreateListFn ptThirdCreateList=nullptr;

static void PTThirdBind(ID3D12GraphicsCommandList* list,const PTThirdListState& s,bool custom) {
    reinterpret_cast<PTSetComputeRootSignatureFn>(PTThirdOriginal(list,29))(list,custom?ptThirdRoot:s.root);
    for(unsigned i=0;i<8;++i){
        if(!(s.valid&(1u<<i)))continue;
        if(i==0||i==1||i==6||i==7){D3D12_GPU_DESCRIPTOR_HANDLE h{};h.ptr=s.args[i];reinterpret_cast<PTThirdTableFn>(PTThirdOriginal(list,31))(list,i,h);}
        else reinterpret_cast<PTThirdCBVFn>(PTThirdOriginal(list,37))(list,i,s.args[i]);
    }
    if(custom)list->SetComputeRootUnorderedAccessView(8,ptThirdHits->GetGPUVirtualAddress());
}
static void PTThirdBarrier(ID3D12GraphicsCommandList* list) {
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=ptThirdHits;
    list->ResourceBarrier(1,&b);
}
static void STDMETHODCALLTYPE PTThirdHookTable(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE h) {
    if(index<8){AcquireSRWLockExclusive(&ptThirdLock);auto* s=PTThirdFindLocked(list,true);if(s){s->args[index]=h.ptr;s->valid|=1u<<index;}ReleaseSRWLockExclusive(&ptThirdLock);}
    reinterpret_cast<PTThirdTableFn>(PTThirdOriginal(list,31))(list,index,h);
}
static void STDMETHODCALLTYPE PTThirdHookCBV(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_VIRTUAL_ADDRESS a) {
    if(index<8){AcquireSRWLockExclusive(&ptThirdLock);auto* s=PTThirdFindLocked(list,true);if(s){s->args[index]=a;s->valid|=1u<<index;}ReleaseSRWLockExclusive(&ptThirdLock);}
    reinterpret_cast<PTThirdCBVFn>(PTThirdOriginal(list,37))(list,index,a);
}
static HRESULT STDMETHODCALLTYPE PTThirdHookReset(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso) {
    HRESULT hr=reinterpret_cast<PTThirdResetFn>(PTThirdOriginal(list,10))(list,allocator,pso);
    if(SUCCEEDED(hr)){AcquireSRWLockExclusive(&ptThirdLock);auto* s=PTThirdFindLocked(list,true);if(s){auto* alias=s->alias;*s={};s->list=list;s->alias=alias;}ReleaseSRWLockExclusive(&ptThirdLock);}
    return hr;
}
static void STDMETHODCALLTYPE PTThirdHookSetState(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state) {
    UINT tag=0,bytes=sizeof(tag);HRESULT tagHr=E_POINTER;
    if(state)tagHr=state->GetPrivateData(kPTThirdState,&bytes,&tag);
    static std::atomic<unsigned long long> binds{0};const auto bind=binds.fetch_add(1)+1;
    if(bind<=16||(bind%240)==0)Log("PT_THIRD_STATE_BIND call=%llu list=%p state=%p tag=%u tag_hr=0x%08lX",bind,list,state,tag,tagHr);
    AcquireSRWLockExclusive(&ptThirdLock);auto* s=PTThirdFindLocked(list,true);if(s)s->target=tag==1;ReleaseSRWLockExclusive(&ptThirdLock);
    reinterpret_cast<PTThirdSetStateFn>(PTThirdOriginal(list,75))(list,state);
}
static void STDMETHODCALLTYPE PTThirdHookDispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc) {
    const auto raw=ptThirdRawDispatches.fetch_add(1)+1;
    PTThirdListState s{};
    bool target=false;
    AcquireSRWLockShared(&ptThirdLock);auto* tracked=PTThirdFindLocked(list,false);if(tracked)target=tracked->target;ReleaseSRWLockShared(&ptThirdLock);
    if(raw<=8||(raw%960)==0)Log("PT_THIRD_RAW_DISPATCH call=%llu tagged=%u list=%p",raw,unsigned(target),list);
    if(!target){reinterpret_cast<PTThirdDispatchFn>(PTThirdOriginal(list,76))(list,desc);return;}
    if(!desc||!PTThirdSnapshot(list,&s,0xcdu)){
        Log("PT_THIRD_DISPATCH blocked=1 reason=incomplete_root_snapshot");return;
    }
    ID3D12Device* device=nullptr;list->GetDevice(__uuidof(ID3D12Device),reinterpret_cast<void**>(&device));
    bool ready=device&&PTThirdEnsureBuffer(device);if(device)device->Release();
    if(!ready){Log("PT_THIRD_DISPATCH blocked=1 reason=buffer_allocation");return;}
    PTThirdBarrier(list);PTThirdBind(list,s,true);
    reinterpret_cast<PTThirdDispatchFn>(PTThirdOriginal(list,76))(list,desc);
    PTThirdBarrier(list);PTThirdBind(list,s,false);
    unsigned long long frame=~0ull;DWORD fault=0;
    if(!ReadEngineFrameSafe(&frame,&fault))frame=~0ull;
    ptThirdWrittenFrame.store(frame);
    auto call=ptThirdDispatches.fetch_add(1)+1;
    if(call<=4||(call%240)==0)Log("PT_THIRD_DISPATCH call=%llu ready=1 width=%u height=%u root_restored=1 bytes=%llu",call,desc->Width,desc->Height,kPTThirdBytes);
}

static bool PTThirdInstallVtable(void** table,bool extended) {
    AcquireSRWLockExclusive(&ptThirdVtableLock);
    PTThirdVtable* entry=nullptr;

    for(auto& v:ptThirdVtables){
        if(v.table==table){
            // Another feature may wrap our hook and keep it as its next call.
            // Never adopt that wrapper as our original: that creates a cycle.
            bool ready=v.ready;ReleaseSRWLockExclusive(&ptThirdVtableLock);return ready;
        }
        if(!v.table&&!entry)entry=&v;
    }
    if(!entry){ReleaseSRWLockExclusive(&ptThirdVtableLock);return false;}
    const unsigned slots[]={10,25,29,31,37,75,76};
    void* hooks[]={reinterpret_cast<void*>(&PTThirdHookReset),
        reinterpret_cast<void*>(&PTReflectionHookSetPipelineState),
        reinterpret_cast<void*>(&PTReflectionHookSetComputeRootSignature),
        reinterpret_cast<void*>(&PTThirdHookTable),reinterpret_cast<void*>(&PTThirdHookCBV),
        reinterpret_cast<void*>(&PTThirdHookSetState),reinterpret_cast<void*>(&PTThirdHookDispatch)};
    // Capture every original before publishing any hook. Readers wait on this
    // lock until the entry is complete, including on a partial install failure.
    entry->table=table;
    extended=extended||entry->extended;
    unsigned count=extended?7u:5u;
    for(unsigned i=0;i<count;++i)if(!entry->original[slots[i]])entry->original[slots[i]]=table[slots[i]];
    bool okay=true;
    for(unsigned i=0;i<count;++i)
        okay=PTAuditPatch(&table[slots[i]],hooks[i],&entry->original[slots[i]])&&okay;
    entry->ready=okay;entry->extended=extended;
    ReleaseSRWLockExclusive(&ptThirdVtableLock);
    Log("PT_THIRD_VTABLE_CAPTURE ready=%u vtable=%p slots=%u per_vtable_originals=1 immutable_originals=1",unsigned(okay),table,count);
    return okay;
}
static bool PTThirdInstallList(ID3D12GraphicsCommandList* list) {
    if(!list)return false;
    auto type=list->GetType();
    if(type!=D3D12_COMMAND_LIST_TYPE_DIRECT&&type!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return false;
    ID3D12GraphicsCommandList4* ray=nullptr;
    if(FAILED(list->QueryInterface(__uuidof(ID3D12GraphicsCommandList4),reinterpret_cast<void**>(&ray)))){
        ptThirdCaptureFailures.fetch_add(1);return false;
    }
    auto v=*reinterpret_cast<void***>(list);auto rv=*reinterpret_cast<void***>(ray);
    bool okay=PTThirdInstallVtable(v,v==rv);
    if(rv!=v)okay=PTThirdInstallVtable(rv,true)&&okay;
    AcquireSRWLockExclusive(&ptThirdLock);
    auto* s=PTThirdFindLocked(list,true);if(s)s->alias=ray;else okay=false;
    ReleaseSRWLockExclusive(&ptThirdLock);
    ray->Release();
    if(!okay)ptThirdCaptureFailures.fetch_add(1);
    else if(type==D3D12_COMMAND_LIST_TYPE_DIRECT)ptThirdDirectReady.store(1);
    return okay;
}
// Existing hook chains are owned by all cooperating features. Inspect only;
// in particular RRClampStrength::Set forwards into our original slot-25 hook.
static void PTThirdPrepareRayCapture() noexcept {
    static std::atomic<unsigned long long> checks{0};
    const auto call=checks.fetch_add(1)+1;
    if(call>8&&(call%240)!=0)return;
    EngineCommandContextSnapshot command{};
    if(!ReadEngineCommandContext(nullptr,&command)||!command.commandList)return;
    auto* list=static_cast<ID3D12GraphicsCommandList*>(command.commandList);
    auto table=*reinterpret_cast<void***>(list);
    Log("PT_THIRD_CHAIN_AUDIT call=%llu list=%p root_hook=%u state_hook=%u dispatch_hook=%u pso_hook=%u mutation=0",
        call,list,unsigned(table[29]==reinterpret_cast<void*>(&PTReflectionHookSetComputeRootSignature)),
        unsigned(table[75]==reinterpret_cast<void*>(&PTThirdHookSetState)),
        unsigned(table[76]==reinterpret_cast<void*>(&PTThirdHookDispatch)),
        unsigned(table[25]==reinterpret_cast<void*>(&PTReflectionHookSetPipelineState)));
}
static bool PTThirdConsumeFresh() {
    unsigned long long frame=~0ull;DWORD fault=0;
    if(!ReadEngineFrameSafe(&frame,&fault)||frame!=ptThirdWrittenFrame.load())return false;
    const auto produced=ptThirdDispatches.load();
    auto consumed=ptThirdConsumed.load();
    return produced>consumed&&ptThirdConsumed.compare_exchange_strong(consumed,produced);
}

static HRESULT STDMETHODCALLTYPE PTThirdHookCreateList(ID3D12Device* device,UINT node,D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso,REFIID iid,void** out) {
    HRESULT hr=ptThirdCreateList(device,node,type,allocator,pso,iid,out);
    if(SUCCEEDED(hr)&&out&&*out&&(type==D3D12_COMMAND_LIST_TYPE_DIRECT||type==D3D12_COMMAND_LIST_TYPE_COMPUTE)){
        ID3D12GraphicsCommandList* list=nullptr;
        if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(__uuidof(ID3D12GraphicsCommandList),reinterpret_cast<void**>(&list)))){
            bool okay=PTThirdInstallList(list);
            Log("PT_THIRD_LIST_CAPTURE ready=%u type=%u",unsigned(okay),unsigned(type));list->Release();
        }
    }
    return hr;
}
static void PTThirdInstallDevice(ID3D12Device* device) {
    AcquireSRWLockExclusive(&ptThirdInstallLock);
    bool okay=PTAuditPatch(&(*reinterpret_cast<void***>(device))[12],reinterpret_cast<void*>(&PTThirdHookCreateList),reinterpret_cast<void**>(&ptThirdCreateList));
    ReleaseSRWLockExclusive(&ptThirdInstallLock);
    Log("PT_THIRD_DEVICE_CAPTURE ready=%u",unsigned(okay));
}

static HRESULT PTThirdCreateState(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    bool target=false;
    if(desc&&desc->pSubobjects)for(UINT i=0;i<desc->NumSubobjects;++i){
        const auto& sub=desc->pSubobjects[i];
        if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY&&sub.pDesc){
            const auto& lib=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(sub.pDesc);
            if(!ptReflectionP2Bytes.empty()&&lib.DXILLibrary.BytecodeLength==ptReflectionP2Bytes.size()&&
                !memcmp(lib.DXILLibrary.pShaderBytecode,ptReflectionP2Bytes.data(),ptReflectionP2Bytes.size()))target=true;
        }
    }
    if(!target&&desc&&desc->pSubobjects&&!ptReflectionP2Bytes.empty()){
        // The engine can later rebuild from its cached NATIVE shader record,
        // bypassing the one-time technique lookup substitution. Replace only
        // the exact audited variant-000 library, before shader IDs/SBTs exist.
        // Never mark or switch an already-created native pipeline at dispatch.
        try {
            std::vector<D3D12_STATE_SUBOBJECT> subs(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
            std::vector<D3D12_DXIL_LIBRARY_DESC> libs(desc->NumSubobjects);
            std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
            bool replaced=false;
            for(UINT i=0;i<desc->NumSubobjects;++i){
                if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY&&subs[i].pDesc){
                    libs[i]=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subs[i].pDesc);
                    const auto& code=libs[i].DXILLibrary;
                    if(code.BytecodeLength==39008u&&code.pShaderBytecode){
                        char sha[65]{};
                        if(PTReflectionSha256(static_cast<const unsigned char*>(code.pShaderBytecode),code.BytecodeLength,sha)&&
                           !strcmp(sha,"c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e")){
                            libs[i].DXILLibrary={ptReflectionP2Bytes.data(),ptReflectionP2Bytes.size()};
                            subs[i].pDesc=&libs[i];replaced=true;
                        }
                    }
                }else if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION&&subs[i].pDesc){
                    associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subs[i].pDesc);
                    auto idx=associations[i].pSubobjectToAssociate-desc->pSubobjects;
                    if(idx<0||static_cast<UINT>(idx)>=desc->NumSubobjects)return E_INVALIDARG;
                    associations[i].pSubobjectToAssociate=&subs[static_cast<size_t>(idx)];subs[i].pDesc=&associations[i];
                }
            }
            if(replaced){
                auto patched=*desc;patched.pSubobjects=subs.data();
                Log("PT_THIRD_NATIVE_REBUILD matched=1 native_bytes=39008 variant=0 replacement=p2 shader_ids_created_after_replacement=1");
                return PTThirdCreateState(device,&patched,iid,out);
            }
        }catch(...){return E_OUTOFMEMORY;}
    }
    if(!target)return ptAuditCreateState(device,desc,iid,out);
    try {
        if(!ptThirdDirectReady.load()||ptThirdCaptureFailures.load()){
            Log("PT_THIRD_ACTIVATION blocked=1 direct_ready=%u capture_failures=%u fallback=p1",
                ptThirdDirectReady.load(),ptThirdCaptureFailures.load());throw 9;
        }
        std::vector<D3D12_STATE_SUBOBJECT> subs(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        std::vector<D3D12_GLOBAL_ROOT_SIGNATURE> globals(desc->NumSubobjects);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        bool replaced=false;
        for(UINT i=0;i<desc->NumSubobjects;++i){
            auto& sub=subs[i];
            if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE){
                globals[i]=*static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(sub.pDesc);
                globals[i].pGlobalRootSignature=PTThirdGetRoot(device,globals[i].pGlobalRootSignature);
                if(!globals[i].pGlobalRootSignature)throw 1;
                sub.pDesc=&globals[i];replaced=true;
            }else if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION){
                associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(sub.pDesc);
                auto idx=associations[i].pSubobjectToAssociate-desc->pSubobjects;
                if(idx<0||static_cast<UINT>(idx)>=desc->NumSubobjects)throw 2;
                associations[i].pSubobjectToAssociate=&subs[static_cast<size_t>(idx)];sub.pDesc=&associations[i];
            }
        }
        if(!replaced)throw 3;
        if(!PTThirdEnsureBuffer(device))throw 10;
        auto custom=*desc;custom.pSubobjects=subs.data();
        HRESULT hr=ptAuditCreateState(device,&custom,iid,out);
        if(SUCCEEDED(hr)&&out&&*out){
            ID3D12StateObject* object=nullptr;
            if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(__uuidof(ID3D12StateObject),reinterpret_cast<void**>(&object)))){
                UINT one=1;object->SetPrivateData(kPTThirdState,sizeof(one),&one);object->Release();
            }
            Log("PT_THIRD_STATE_CREATE ready=1 global_root_replaced=1 local_root_unchanged=1 payload=4");return hr;
        }
        Log("PT_THIRD_STATE_CREATE ready=0 hr=0x%08lX fallback=p1",hr);
    }catch(...){Log("PT_THIRD_STATE_CREATE ready=0 reason=root_clone fallback=p1");}
    // If root integration fails, compile the known native-root P1 library.
    // The post-composite stays disabled until a private-buffer ray dispatch ran.
    if(ptReflectionP1Bytes.empty())return E_FAIL;
    try {
        std::vector<D3D12_STATE_SUBOBJECT> subs(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);
        std::vector<D3D12_DXIL_LIBRARY_DESC> libs(desc->NumSubobjects);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        for(UINT i=0;i<desc->NumSubobjects;++i){
            if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY){
                libs[i]=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subs[i].pDesc);
                if(libs[i].DXILLibrary.BytecodeLength==ptReflectionP2Bytes.size()){
                    libs[i].DXILLibrary={ptReflectionP1Bytes.data(),ptReflectionP1Bytes.size()};subs[i].pDesc=&libs[i];
                }
            }else if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION){
                associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subs[i].pDesc);
                auto idx=associations[i].pSubobjectToAssociate-desc->pSubobjects;
                if(idx<0||static_cast<UINT>(idx)>=desc->NumSubobjects)return E_INVALIDARG;
                associations[i].pSubobjectToAssociate=&subs[static_cast<size_t>(idx)];subs[i].pDesc=&associations[i];
            }
        }
        auto fallback=*desc;fallback.pSubobjects=subs.data();return ptAuditCreateState(device,&fallback,iid,out);
    }catch(...){return E_OUTOFMEMORY;}
}
