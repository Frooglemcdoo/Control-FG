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
static constexpr UINT64 kPTThirdBytes=8388608ull*96ull;

struct PTThirdListState {
    ID3D12GraphicsCommandList* list=nullptr;
    ID3D12GraphicsCommandList* alias=nullptr;
    ID3D12RootSignature* root=nullptr;
    UINT64 args[8]{};
    unsigned valid=0;
    bool target=false;
    UINT64 extraRecord=0;
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
    Log("PT_THIRD_PRIVATE_BUFFER ready=%u hr=0x%08lX bytes=%llu stride=32 records_per_pixel=3 resize_reallocation=0",unsigned(ready),hr,kPTThirdBytes);
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
    UINT64 tag=0;UINT bytes=sizeof(tag);HRESULT tagHr=E_POINTER;
    if(state)tagHr=state->GetPrivateData(kPTThirdState,&bytes,&tag);
    static std::atomic<unsigned long long> binds{0};const auto bind=binds.fetch_add(1)+1;
    if(bind<=16||(bind%240)==0)Log("PT_THIRD_STATE_BIND call=%llu list=%p state=%p extra_record=%llu tag_hr=0x%08lX",bind,list,state,tag,tagHr);
    AcquireSRWLockExclusive(&ptThirdLock);auto* s=PTThirdFindLocked(list,true);if(s){s->target=SUCCEEDED(tagHr)&&tag!=0;s->extraRecord=tag;}ReleaseSRWLockExclusive(&ptThirdLock);
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
    PTThirdBind(list,s,true);
    // The engine's original shader identifier executes the ORIGINAL native
    // raygen, closest-hit, any-hit, miss, and shadow exports in this same state.
    reinterpret_cast<PTThirdDispatchFn>(PTThirdOriginal(list,76))(list,desc);
    const bool extra=PTReflectionUserRequested()&&s.extraRecord&&desc->Depth==1u&&
        static_cast<UINT64>(desc->Width)*desc->Height<=8388608ull;
    if(extra){
        // Order all native UAVs, not only the private hit buffer.
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
        list->ResourceBarrier(1,&barrier);
        auto additional=*desc;
        additional.RayGenerationShaderRecord.StartAddress=s.extraRecord;
        additional.RayGenerationShaderRecord.SizeInBytes=D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        // Hit/miss tables and their local-root records belong to this SAME state.
        // Native hit shaders do not publish UAV writes with shader barriers.
        // Separate dispatches provide the required API memory boundary.
        for(UINT stage=0;stage<4u;++stage){
            list->SetComputeRoot32BitConstant(9,stage,0);
            reinterpret_cast<PTThirdDispatchFn>(PTThirdOriginal(list,76))(list,&additional);
            list->ResourceBarrier(1,&barrier);
        }
        unsigned long long frame=~0ull;DWORD fault=0;
        if(!ReadEngineFrameSafe(&frame,&fault))frame=~0ull;
        ptThirdWrittenFrame.store(frame);
        auto call=ptThirdDispatches.fetch_add(1)+1;
        if(call<=4||(call%240)==0)Log("PT_THIRD_DISPATCH call=%llu ready=1 width=%u height=%u native_first=1 extra_stages=4 native_layers_restored=1",call,desc->Width,desc->Height);
    }
    PTThirdBind(list,s,false);
    if(raw<=8||(raw%960)==0)Log("PT_NATIVE_PRESERVED_DISPATCH native=1 extra=%u root_restored=1",unsigned(extra));
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

// R6: native bytes and native exports are retained. Only a separately named
// raygen is appended. All shader identifiers are obtained from the SAME state.
static bool PTThirdIsNative(const D3D12_STATE_OBJECT_DESC* desc) {
    if(!desc||!desc->pSubobjects||desc->Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)return false;
    for(UINT i=0;i<desc->NumSubobjects;++i){
        const auto& sub=desc->pSubobjects[i];
        if(sub.Type!=D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY||!sub.pDesc)continue;
        const auto& code=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(sub.pDesc)->DXILLibrary;
        char sha[65]{};
        if(code.BytecodeLength==39008u&&code.pShaderBytecode&&
           PTReflectionSha256(static_cast<const unsigned char*>(code.pShaderBytecode),code.BytecodeLength,sha)&&
           !strcmp(sha,"c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e"))return true;
    }
    return false;
}
static HRESULT PTThirdCreateState(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    if(!PTThirdIsNative(desc)||!ptReflectionP2InjectionEnabled||ptReflectionP2Bytes.empty()||
       !ptThirdDirectReady.load()||ptThirdCaptureFailures.load())return ptAuditCreateState(device,desc,iid,out);
    ID3D12StateObject* object=nullptr;
    ID3D12Resource* record=nullptr;
    ID3D12RootSignature* emptyLocal=nullptr;
    ID3D12StateObjectProperties* props=nullptr;
    ID3DBlob* blob=nullptr;
    try {
        const UINT n=desc->NumSubobjects;
        // Allocate at final size before relocating pointers into the array.
        std::vector<D3D12_STATE_SUBOBJECT> subs(n+4);
        memcpy(subs.data(),desc->pSubobjects,n*sizeof(subs[0]));
        std::vector<D3D12_GLOBAL_ROOT_SIGNATURE> globals(n);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(n);
        std::vector<std::vector<LPCWSTR>> names(n);
        bool rootReady=false;
        for(UINT i=0;i<n;++i){
            if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE){
                globals[i]=*static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(subs[i].pDesc);
                globals[i].pGlobalRootSignature=PTThirdGetRoot(device,globals[i].pGlobalRootSignature);
                if(!globals[i].pGlobalRootSignature)throw 1;
                subs[i].pDesc=&globals[i];rootReady=true;
            }else if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION){
                associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subs[i].pDesc);
                auto idx=associations[i].pSubobjectToAssociate-desc->pSubobjects;
                if(idx<0||static_cast<UINT>(idx)>=n)throw 2;
                const auto kind=desc->pSubobjects[idx].Type;
                if(kind==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG&&associations[i].NumExports){
                    names[i].assign(associations[i].pExports,associations[i].pExports+associations[i].NumExports);
                    names[i].push_back(L"PTExtraRayGeneration");
                    associations[i].pExports=names[i].data();associations[i].NumExports=static_cast<UINT>(names[i].size());
                }
                associations[i].pSubobjectToAssociate=&subs[static_cast<size_t>(idx)];subs[i].pDesc=&associations[i];
            }
        }
        if(!rootReady||!PTThirdEnsureBuffer(device))throw 3;
        D3D12_EXPORT_DESC extraExport{L"PTExtraRayGeneration",L"reflectionRayGeneration",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC extraLib{{ptReflectionP2Bytes.data(),ptReflectionP2Bytes.size()},1,&extraExport};
        subs[n]={D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&extraLib};
        // No local-root arguments are required for the additional raygen.
        D3D12_ROOT_SIGNATURE_DESC localDesc{};localDesc.Flags=D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        using Serialize=HRESULT(WINAPI*)(const D3D12_ROOT_SIGNATURE_DESC*,D3D_ROOT_SIGNATURE_VERSION,ID3DBlob**,ID3DBlob**);
        auto serialize=reinterpret_cast<Serialize>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
        if(!serialize||FAILED(serialize(&localDesc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,nullptr)))throw 4;
        HRESULT hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),__uuidof(ID3D12RootSignature),reinterpret_cast<void**>(&emptyLocal));
        blob->Release();blob=nullptr;if(FAILED(hr))throw 5;
        D3D12_LOCAL_ROOT_SIGNATURE local{emptyLocal};
        subs[n+1]={D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&local};
        LPCWSTR extraName=L"PTExtraRayGeneration";
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localAssoc{&subs[n+1],1,&extraName};
        subs[n+2]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&localAssoc};
        // Explicitly associate the cloned global root with the extra export;
        // this also covers native descriptions with no global association.
        UINT globalIndex=0;for(UINT i=0;i<n;++i)if(subs[i].Type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE)globalIndex=i;
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION globalAssoc{&subs[globalIndex],1,&extraName};
        subs[n+3]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&globalAssoc};
        auto custom=*desc;custom.NumSubobjects=n+4;custom.pSubobjects=subs.data();
        hr=ptAuditCreateState(device,&custom,__uuidof(ID3D12StateObject),reinterpret_cast<void**>(&object));
        if(FAILED(hr)||!object){Log("PT_NATIVE_EXTRA_STATE ready=0 hr=0x%08lX fallback=original_native",hr);throw 6;}
        if(FAILED(object->QueryInterface(__uuidof(ID3D12StateObjectProperties),reinterpret_cast<void**>(&props))))throw 7;
        const void* identifier=props->GetShaderIdentifier(extraName);if(!identifier)throw 8;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=64;rd.Height=1;
        rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,__uuidof(ID3D12Resource),reinterpret_cast<void**>(&record));
        if(FAILED(hr))throw 9;
        void* mapped=nullptr;D3D12_RANGE noRead{0,0};
        if(FAILED(record->Map(0,&noRead,&mapped)))throw 10;
        memset(mapped,0,64);memcpy(mapped,identifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);record->Unmap(0,nullptr);
        props->Release();props=nullptr;
        UINT64 address=record->GetGPUVirtualAddress();
        if((address% D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)!=0||FAILED(object->SetPrivateData(kPTThirdState,sizeof(address),&address)))throw 11;
        hr=object->QueryInterface(iid,out);if(FAILED(hr))throw 12;
        // Retain GPU-visible record for process lifetime: no in-flight retirement.
        record=nullptr;emptyLocal->Release();emptyLocal=nullptr;object->Release();object=nullptr;
        Log("PT_NATIVE_EXTRA_STATE ready=1 native_dxil_unchanged=1 native_exports_unchanged=1 same_state_shader_tables=1");
        return hr;
    }catch(...){
        if(props)props->Release();if(blob)blob->Release();if(record)record->Release();
        if(emptyLocal)emptyLocal->Release();if(object)object->Release();
        Log("PT_NATIVE_EXTRA_STATE fallback=original_native no_p1_replacement=1");
        return ptAuditCreateState(device,desc,iid,out);
    }
}
