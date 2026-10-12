#pragma once
#include "pt_glass_resources.h"
#include "pt_glass_descriptors.h"
#include "pt_glass_vtable_call.h"
#include "pt_native_job_runtime.h"
#include "pt_scene_live_owner.h"
#include "pt_scene_queue_runtime.h"
#include <intrin.h>
#include <unordered_map>
#include <memory>
// G1: Steam hash-locked glass continuation. Original exports and native passes
// remain present; only a fully prepared dispatch selects the additional raygen.
namespace PTGlassRuntime {
using Microsoft::WRL::ComPtr;
using CreateState=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
using CopySimple=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
using CreateCbv=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using SetRayPso=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,ID3D12StateObject*);
using DispatchRays=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
using Execute=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*);
using Reset=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using CreateSrv=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateUav=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateHeap=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CopyAll=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
static CreateSrv createSrv=nullptr;static CreateUav createUav=nullptr;static CreateHeap createHeap=nullptr;static CopyAll copyAll=nullptr;
static CreateState createState=nullptr;static CopySimple copySimple=nullptr;static CreateCbv createCbv=nullptr;
// Invoke the exact interface supplied by the native call site. A QueryInterface
// result can be a different interposer object and must not select the hook target.
static void NativeSetRayPso(ID3D12GraphicsCommandList4* list,ID3D12StateObject* pso){
    reinterpret_cast<SetRayPso>((*reinterpret_cast<void***>(list))[75])(list,pso);
}
static void NativeDispatchRays(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc){
    reinterpret_cast<DispatchRays>((*reinterpret_cast<void***>(list))[76])(list,desc);
}
static auto setRayPso=&NativeSetRayPso;static auto dispatchRays=&NativeDispatchRays;static Execute execute=nullptr;static Reset reset=nullptr;
static ComPtr<ID3D12Device> device;static ComPtr<ID3D12CommandQueue> queue;
static ComPtr<ID3D12RootSignature> lightingRoot;static ComPtr<ID3D12PipelineState> nativeLighting,secondaryLighting,compositeLighting;
static std::vector<unsigned char> rayBytes,secondaryBytes,compositeBytes;
static std::mutex lock;static PTGlass::Resources resources;
struct Pipeline {ComPtr<ID3D12StateObject> owner;unsigned char raygen[32]{};};
static std::unordered_map<ID3D12StateObject*,Pipeline> pipelines;
struct Pending {ID3D12CommandList* list;UINT64 serial;PTGlass::SubmissionHold hold{};};static std::vector<Pending> pending;
static std::atomic<UINT64> sequence{1},dispatchCount{0};static std::atomic<bool> enabled{false},queueReady{false};
static thread_local bool mirrorGuard=false,rayScope=false;static thread_local unsigned captureKind=0;
static thread_local ID3D12StateObject* currentRayPso=nullptr;
static void Skip(const char* stage,UINT64 mask=0,HRESULT hr=S_OK) {
    static std::atomic<UINT64> count{0};const auto n=++count;
    if(n<=12||n%600==0)Log("PT_GLASS_SKIP stage=%s observed=%llx hr=0x%08lX count=%llu",stage,mask,static_cast<unsigned long>(hr),n);
}
struct Context {void* manager=nullptr;ID3D12GraphicsCommandList* list=nullptr;ID3D12GraphicsCommandList4* rays=nullptr;ID3D12CommandQueue* queue=nullptr;PTGlass::DescriptorRange table;D3D12_GPU_DESCRIPTOR_HANDLE gpu{};};
struct Pass {UINT64 serial=0;unsigned mode=1;Context rayContext{},lightingContext{};std::unique_ptr<PTGlass::LightingTableShadow> ray,light;PTGlass::Resources::Slot* slot=nullptr;bool traced=false;};
static thread_local Pass pass;
static bool ReadContext(Context* out) noexcept {
    *out={};if(!verifiedD3d)return false;
    __try {
        auto* b=reinterpret_cast<unsigned char*>(verifiedD3d);
        const DWORD index=*reinterpret_cast<DWORD*>(b+0x1115fc);
        auto** vector=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        auto* tls=vector[index];if(!tls)return false;
        auto* manager=*reinterpret_cast<unsigned char**>(tls+0x20);
        if(!manager)manager=*reinterpret_cast<unsigned char**>(b+0x111c10);
        auto* context=*reinterpret_cast<unsigned char**>(tls+8);
        if(!context)context=*reinterpret_cast<unsigned char**>(b+0x111c18);
        auto* nativeQueue=*reinterpret_cast<unsigned char**>(tls+0x18);
        if(!nativeQueue)nativeQueue=*reinterpret_cast<unsigned char**>(b+0x111c38);
        if(!manager||!context||!nativeQueue)return false;
        auto* provider=*reinterpret_cast<unsigned char**>(manager+0x50);if(!provider)return false;
        out->list=*reinterpret_cast<ID3D12GraphicsCommandList**>(context+0x18);
        out->rays=*reinterpret_cast<ID3D12GraphicsCommandList4**>(context+0x20);
        out->queue=*reinterpret_cast<ID3D12CommandQueue**>(nativeQueue+0x10);
        out->table.heap=*reinterpret_cast<ID3D12DescriptorHeap**>(provider);
        out->gpu=*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(manager+0x30);
        const auto base=*reinterpret_cast<UINT64*>(provider+0x10);
        const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(!out->list||!out->queue||!out->table.heap||out->gpu.ptr<base||(out->gpu.ptr-base)%step)return false;
        const auto at=(out->gpu.ptr-base)/step;if(at>UINT32_MAX)return false;
        out->table.first=static_cast<UINT>(at);out->table.count=49;out->manager=manager;
        return out->list->GetType()==D3D12_COMMAND_LIST_TYPE_DIRECT&&out->queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool Fresh(const Context& context,PTGlass::DescriptorRange* out) noexcept {
    *out={};
    __try {
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};D3D12_CPU_DESCRIPTOR_HANDLE cpu{};unsigned char* provider=nullptr;
        using Allocate=void(*)(void*,UINT64,D3D12_GPU_DESCRIPTOR_HANDLE*,D3D12_CPU_DESCRIPTOR_HANDLE*,unsigned char**);
        auto fn=reinterpret_cast<Allocate>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x1aa00);
        fn(context.manager,49,&gpu,&cpu,&provider);
        if(!provider||*reinterpret_cast<ID3D12DescriptorHeap**>(provider)!=context.table.heap)return false;
        const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const UINT64 base=*reinterpret_cast<UINT64*>(provider+0x10);
        if(gpu.ptr<base||(gpu.ptr-base)%step||(gpu.ptr-base)/step>UINT32_MAX)return false;
        *out={context.table.heap,static_cast<UINT>((gpu.ptr-base)/step),49};return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool Patch(void* object,UINT index,void* hook,void** original) noexcept {
    if(!object)return false;auto** slot=&(*reinterpret_cast<void***>(object))[index];
    if(*slot==hook)return *original!=nullptr;
    if(*original&&*original!=*slot)return false;
    DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
    *original=*slot;InterlockedExchangePointer(slot,hook);DWORD ignored=0;
    return VirtualProtect(slot,sizeof(void*),old,&ignored)!=FALSE;
}
static PTGlass::LightingTableShadow* ActiveShadow() {
    if(!captureKind||mirrorGuard||!enabled)return nullptr;
    auto& holder=captureKind==1?pass.ray:pass.light;
    auto& context=captureKind==1?pass.rayContext:pass.lightingContext;
    if(!holder){
        if(!ReadContext(&context))return nullptr;
        mirrorGuard=true;holder=std::make_unique<PTGlass::LightingTableShadow>();
        const bool good=SUCCEEDED(holder->Initialize(device.Get()))&&holder->Begin(pass.serial,context.table);
        mirrorGuard=false;if(!good){holder.reset();return nullptr;}
    }
    return holder.get();
}
static void STDMETHODCALLTYPE CopyHook(ID3D12Device* d,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE dst,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type){
    copySimple(d,count,dst,src,type);
    if(type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)return;
    if(PTSceneLive::owner.Active()&&count)PTSceneLive::owner.Copy({{dst.ptr,count}},{{src.ptr,count}});
    if(auto* s=ActiveShadow()){mirrorGuard=true;s->ObserveNativeCopy(pass.serial,dst,count,src);mirrorGuard=false;}
}
static void STDMETHODCALLTYPE CbvHook(ID3D12Device* d,const D3D12_CONSTANT_BUFFER_VIEW_DESC* cb,D3D12_CPU_DESCRIPTOR_HANDLE dst){
    createCbv(d,cb,dst);PTSceneLive::owner.Overwrite(dst);if(!cb)return;
    if(auto* s=ActiveShadow()){mirrorGuard=true;s->ObserveNativeConstant(pass.serial,dst,*cb);mirrorGuard=false;}
}
static void STDMETHODCALLTYPE SrvHook(ID3D12Device* d,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst){
    createSrv(d,resource,desc,dst);PTSceneLive::owner.Overwrite(dst,desc);
}
static void STDMETHODCALLTYPE UavHook(ID3D12Device* d,ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst){
    createUav(d,resource,counter,desc,dst);PTSceneLive::owner.Overwrite(dst);
}
static HRESULT STDMETHODCALLTYPE HeapHook(ID3D12Device* d,const D3D12_DESCRIPTOR_HEAP_DESC* desc,REFIID iid,void** out){
    const HRESULT hr=createHeap(d,desc,iid,out);
    if(SUCCEEDED(hr)&&out&&*out&&PTSceneLive::owner.Active()){
        ComPtr<ID3D12DescriptorHeap> heap;
        if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&heap))))PTSceneLive::owner.Heap(heap.Get());
    }
    return hr;
}
static void STDMETHODCALLTYPE CopyAllHook(ID3D12Device* d,UINT dstCount,const D3D12_CPU_DESCRIPTOR_HANDLE* dst,const UINT* dstSizes,
    UINT srcCount,const D3D12_CPU_DESCRIPTOR_HANDLE* src,const UINT* srcSizes,D3D12_DESCRIPTOR_HEAP_TYPE type){
    copyAll(d,dstCount,dst,dstSizes,srcCount,src,srcSizes,type);
    if(!PTSceneLive::owner.Active()||type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)return;
    if((dstCount&&!dst)||(srcCount&&!src)||dstCount>8192||srcCount>8192){PTSceneLive::owner.InvalidateDescriptors();return;}
    std::vector<PTSceneDescriptors::Range> destinations,sources;
    for(UINT i=0;i<dstCount;++i){const UINT n=dstSizes?dstSizes[i]:1;if(n)destinations.push_back({dst[i].ptr,n});}
    for(UINT i=0;i<srcCount;++i){const UINT n=srcSizes?srcSizes[i]:1;if(n)sources.push_back({src[i].ptr,n});}
    if(!destinations.empty()||!sources.empty())PTSceneLive::owner.Copy(destinations,sources);
}
static HRESULT STDMETHODCALLTYPE StateHook(ID3D12Device5* d,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out){
    if(!desc||desc->Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE||!enabled)return createState(d,desc,iid,out);
    bool target=false;
    for(UINT i=0;i<desc->NumSubobjects;++i)if(desc->pSubobjects[i].Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY){
        const auto* lib=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);char sha[65]{};
        if(lib&&lib->DXILLibrary.BytecodeLength==29328&&PTReflectionSha256(static_cast<const unsigned char*>(lib->DXILLibrary.pShaderBytecode),29328,sha)&&strcmp(sha,"66ec0c29541de1f4aa59636ce6e4415de440a12315b983839d0de0c0a2bab9a8")==0)target=true;
    }
    if(!target)return createState(d,desc,iid,out);
    std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);objects.reserve(desc->NumSubobjects+1);
    std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
    std::vector<std::vector<LPCWSTR>> exportNames(desc->NumSubobjects);
    for(UINT i=0;i<desc->NumSubobjects;++i)if(objects[i].Type==D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION){
        associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
        const auto n=associations[i].pSubobjectToAssociate-desc->pSubobjects;
        if(n<0||static_cast<UINT>(n)>=desc->NumSubobjects)return createState(d,desc,iid,out);
        const auto type=objects[static_cast<size_t>(n)].Type;
        bool primary=false;
        for(UINT j=0;j<associations[i].NumExports;++j)primary=primary||wcscmp(associations[i].pExports[j],L"transparentRayGeneration")==0;
        if(primary&&(type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE||type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG)){
            exportNames[i].assign(associations[i].pExports,associations[i].pExports+associations[i].NumExports);
            exportNames[i].push_back(L"ptGlassRayGeneration");associations[i].NumExports=static_cast<UINT>(exportNames[i].size());associations[i].pExports=exportNames[i].data();
        }
        associations[i].pSubobjectToAssociate=&objects[static_cast<size_t>(n)];objects[i].pDesc=&associations[i];
    }
    D3D12_EXPORT_DESC exportDesc{L"ptGlassRayGeneration",nullptr,D3D12_EXPORT_FLAG_NONE};
    D3D12_DXIL_LIBRARY_DESC extra{{rayBytes.data(),rayBytes.size()},1,&exportDesc};
    objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&extra});
    D3D12_STATE_OBJECT_DESC custom=*desc;custom.NumSubobjects=static_cast<UINT>(objects.size());custom.pSubobjects=objects.data();
    HRESULT hr=createState(d,&custom,iid,out);
    if(FAILED(hr)){Log("PT_GLASS_STATE_FALLBACK hr=0x%08lX",static_cast<unsigned long>(hr));return createState(d,desc,iid,out);}
    ComPtr<ID3D12StateObject> object;ComPtr<ID3D12StateObjectProperties> props;
    if(out&&*out&&SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&object)))&&SUCCEEDED(object.As(&props))){
        const void* identifier=props->GetShaderIdentifier(L"ptGlassRayGeneration");
        if(identifier){Pipeline p;p.owner=object;memcpy(p.raygen,identifier,32);std::lock_guard<std::mutex> guard(lock);pipelines.emplace(object.Get(),p);if(iid==__uuidof(ID3D12StateObject))pipelines.emplace(static_cast<ID3D12StateObject*>(*out),p);Log("PT_GLASS_STATE_READY native_exports_retained=1 extra_raygen=1 recursion_unchanged=1");}
    }
    return hr;
}
static void STDMETHODCALLTYPE ExecuteHook(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists){
    execute(q,count,lists);std::lock_guard<std::mutex> guard(lock);
    if(q!=queue.Get())return;
    for(auto it=pending.begin();it!=pending.end();){bool submitted=false;for(UINT i=0;i<count;++i)submitted=submitted||lists[i]==it->list;
        if(submitted)it->hold.Submitted();
        if(it->hold.Ready()){if(FAILED(resources.Submitted(it->serial)))enabled=false;it=pending.erase(it);}else ++it;}
}
static HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso){
    const HRESULT hr=reset(list,allocator,pso);if(SUCCEEDED(hr)){std::lock_guard<std::mutex> guard(lock);
        for(auto it=pending.begin();it!=pending.end();)if(it->list==list&&!it->hold.submitted){
            if(it->hold.anySubmitted){if(FAILED(resources.Submitted(it->serial)))enabled=false;}
            else resources.Discarded(it->serial);it=pending.erase(it);
        }else ++it;}
    return hr;
}
static void STDMETHODCALLTYPE SetRayPsoHook(ID3D12GraphicsCommandList4* list,ID3D12StateObject* pso){currentRayPso=pso;setRayPso(list,pso);}
static void STDMETHODCALLTYPE RaysHook(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc){
    if(PTSceneLive::owner.Active()){
        auto& capture=PTSceneLive::owner.Scenes();
        if(!PTSceneQueues::Start(&capture)||!PTSceneQueues::WatchList(list))capture.Invalidate();
    }
    PTNativeJobRuntime::Recorded(list,desc,currentRayPso);
    if(!enabled||!rayScope||!desc||desc->Depth!=1||pass.mode==0||(ptReflectionUserState.load()&1u)==0){if(enabled&&rayScope)Skip("dispatch_disabled_or_shape");dispatchRays(list,desc);return;}
    if(!pass.ray){Skip("ray_descriptor_snapshot");dispatchRays(list,desc);return;}
    Context context{};unsigned char identifier[32]{};
    {std::lock_guard<std::mutex> guard(lock);auto it=pipelines.find(currentRayPso);if(it==pipelines.end()){Skip("ray_pipeline");dispatchRays(list,desc);return;}memcpy(identifier,it->second.raygen,32);}
    if(!ReadContext(&context)||context.rays!=list||context.manager!=pass.rayContext.manager||context.gpu.ptr!=pass.rayContext.gpu.ptr){Skip("dispatch_context");dispatchRays(list,desc);return;}
    if(!queueReady){std::lock_guard<std::mutex> guard(lock);
        if(!queueReady){
            if(!Patch(context.queue,10,reinterpret_cast<void*>(&ExecuteHook),reinterpret_cast<void**>(&execute))){Skip("queue_submit_hook");dispatchRays(list,desc);return;}
            // Control submits through the Streamline queue proxy, whose GetDevice
            // returns the proxy device. Resources belong to the native device.
            // Keep intercepting the actual submitted queue; fence its owned native
            // counterpart only after that submission returns.
            void* raw=nullptr;
            const bool resolved=slGetNativeInterfaceApi&&slGetNativeInterfaceApi(context.queue,&raw)==sl::Result::eOk;
            ComPtr<ID3D12CommandQueue> nativeQueue;
            if(resolved&&raw)static_cast<IUnknown*>(raw)->QueryInterface(IID_PPV_ARGS(&nativeQueue));
            if(raw)static_cast<IUnknown*>(raw)->Release();
            if(!nativeQueue){Skip("queue_native_interface");dispatchRays(list,desc);return;}
            const HRESULT hr=resources.Initialize(device.Get(),nativeQueue.Get(),2ull*1024*1024*1024);
            if(FAILED(hr)){Skip("queue_resource_init",0,hr);dispatchRays(list,desc);return;}
            queue=context.queue;queueReady=true;
            Log("PT_GLASS_QUEUE_READY submitted_queue=%p native_queue=%p native_device=%p",queue.Get(),nativeQueue.Get(),device.Get());
        }
    }
    if(queue.Get()!=context.queue){Skip("queue_changed");dispatchRays(list,desc);return;}
    if(pass.serial>UINT32_MAX){Skip("recording_serial");dispatchRays(list,desc);return;}
    const HRESULT acquireHr=resources.Acquire(pass.serial,desc->Width,desc->Height,&pass.slot);
    if(FAILED(acquireHr)){Skip("resource_acquire",(UINT64(desc->Width)<<32)|desc->Height,acquireHr);dispatchRays(list,desc);return;}
    auto* slot=pass.slot;void* mapped=nullptr;
    if(FAILED(slot->dispatch->Map(0,nullptr,&mapped))){Skip("dispatch_upload_map");resources.Discarded(pass.serial);pass.slot=nullptr;dispatchRays(list,desc);return;}
    memset(mapped,0,512);UINT constants[4]{static_cast<UINT>(pass.serial),65534,desc->Width,desc->Height};memcpy(mapped,constants,16);memcpy(static_cast<unsigned char*>(mapped)+256,identifier,32);slot->dispatch->Unmap(0,nullptr);
    mirrorGuard=true;
    const bool ready=pass.ray->Copy(pass.serial,2,{slot->descriptors.Get(),4,1})&&pass.ray->Copy(pass.serial,45,{slot->descriptors.Get(),0,1});
    PTGlass::DescriptorRange fresh;D3D12_GPU_DESCRIPTOR_HANDLE table{};
    constexpr UINT64 required=(7ull<<5)|(15ull<<41)|(1ull<<2)|(1ull<<45);
    const bool published=ready&&Fresh(context,&fresh)&&pass.ray->PublishChecked(pass.serial,fresh,required,&table);mirrorGuard=false;
    if(!published){Skip("ray_table",pass.ray->Observed());resources.Discarded(pass.serial);pass.slot=nullptr;dispatchRays(list,desc);return;}
    // Slot resources finish each successful lighting pass in UAV state.
    D3D12_DISPATCH_RAYS_DESC custom=*desc;custom.RayGenerationShaderRecord={slot->dispatch->GetGPUVirtualAddress()+256,32};
    {std::lock_guard<std::mutex> guard(lock);pending.push_back({context.list,pass.serial});}
    list->SetComputeRootDescriptorTable(0,table);dispatchRays(list,&custom);list->SetComputeRootDescriptorTable(0,context.gpu);
    pass.traced=true;const auto n=++dispatchCount;if(n<=4||n%240==0)Log("PT_GLASS_RAYS dispatch=%llu extent=%ux%u native_hits_restored=1 additional_opaque_reflection=1",n,desc->Width,desc->Height);
}
using UseFn=void(*)(void*,void*);using RayFn=void(*)(int,int);using BindFn=void(*)(void*);using LightFn=void(*)(void*,const void*);
static UseFn useNative=nullptr;static RayFn rayNative=nullptr;static BindFn bindNative=nullptr;static LightFn lightNative=nullptr;
static RRIndirectCallPatch usePatch,rayPatch,lightPatch;static RRAlbedoCallPatch bindPatch{};
static void UseHook(void* table,void* state){
    pass=Pass{};pass.serial=sequence.fetch_add(1);pass.mode=ptGlassUserMode.load(std::memory_order_acquire);captureKind=enabled?1u:0u;
    useNative(table,state);captureKind=0;
    if(enabled&&!pass.ray)Skip("native_constant_use");
}
static void RayHook(int a,int b){
    Context c{};bool ready=false;
    if(enabled&&ReadContext(&c)&&c.rays){
        ready=Patch(c.list,10,reinterpret_cast<void*>(&ResetHook),reinterpret_cast<void**>(&reset));
        if(!ready)Skip("list_reset_hook");
    }else if(enabled)Skip("ray_context");
    rayScope=enabled&&ready;static std::atomic<UINT64> entries{0};const auto n=++entries;
    if(n<=4)Log("PT_GLASS_NATIVE_ENTER call=%llu enabled=%u scope=%u snapshot=%u",n,unsigned(enabled.load()),unsigned(rayScope),unsigned(pass.ray!=nullptr));
    rayNative(a,b);rayScope=false;
}
static void BindHook(void* shader){captureKind=enabled&&pass.traced?2u:0u;bindNative(shader);captureKind=0;}
static void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
}
static bool LightingStateMatches(void* state) noexcept {
    __try {
        auto* s=static_cast<unsigned char*>(state);
        auto* shader=*reinterpret_cast<unsigned char**>(s+0x1068);
        return *reinterpret_cast<ID3D12RootSignature**>(s+0x878)==lightingRoot.Get()&&shader&&
            *reinterpret_cast<ID3D12PipelineState**>(shader+0x78)==nativeLighting.Get();
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// Finalize only after deciding whether a lighting consumer was recorded.
// Early returns retain the ray submission until a real queue fence retires it.
struct FinishLighting {
    UINT64 serial;ID3D12CommandList* consumer=nullptr;
    ~FinishLighting(){std::lock_guard<std::mutex> guard(lock);
        for(auto it=pending.begin();it!=pending.end();++it)if(it->serial==serial){
            const bool different=consumer&&consumer!=it->list;
            if(!it->hold.Finish(different)){enabled=false;Skip("consumer_submission_order");return;}
            if(consumer)it->list=consumer;
            if(it->hold.Ready()){if(FAILED(resources.Submitted(serial)))enabled=false;pending.erase(it);}
            return;
        }
    }
};
static bool ConsumerOrdered(UINT64 serial,ID3D12CommandList* consumer){
    std::lock_guard<std::mutex> guard(lock);
    for(const auto& p:pending)if(p.serial==serial)return p.list==consumer||p.hold.submitted;
    return false;
}
static void LightHook(void* state,const void* dimensions){
    FinishLighting finish{pass.traced?pass.serial:0};
    lightNative(state,dimensions);
    if(!enabled||!pass.traced)return;
    if(!pass.slot||!pass.light||!secondaryLighting||!compositeLighting||!nativeLighting||!LightingStateMatches(state)){Skip("lighting_state_or_snapshot");return;}
    constexpr UINT64 fullNative=3ull|(((1ull<<28)-1)<<5)|(1ull<<41);
    if(!pass.light->Has(pass.serial,fullNative)){Skip("lighting_inputs",pass.light->Observed());return;}
    Context context{};if(!ReadContext(&context)){Skip("lighting_context_read");return;}
    if(context.list!=pass.lightingContext.list||context.manager!=pass.lightingContext.manager||
       context.queue!=pass.rayContext.queue||context.gpu.ptr!=pass.lightingContext.gpu.ptr){
        const UINT64 mismatch=(context.list!=pass.lightingContext.list?1ull:0ull)|
            (context.manager!=pass.lightingContext.manager?2ull:0ull)|
            (context.queue!=pass.rayContext.queue?4ull:0ull)|
            (context.gpu.ptr!=pass.lightingContext.gpu.ptr?8ull:0ull);
        Skip("lighting_context",mismatch);return;
    }
    if(!ConsumerOrdered(pass.serial,context.list)){Skip("lighting_producer_not_submitted");return;}
    if(!Patch(context.list,10,reinterpret_cast<void*>(&ResetHook),reinterpret_cast<void**>(&reset))){Skip("lighting_reset_hook");return;}
    const auto* dims=static_cast<const UINT*>(dimensions);if(!dims||dims[2]!=1){Skip("lighting_dimensions");return;}
    auto* slot=pass.slot;PTGlass::LightingTableShadow composite;
    mirrorGuard=true;
    bool good=SUCCEEDED(composite.Initialize(device.Get()))&&composite.Begin(pass.serial,context.table)&&composite.Copy(pass.serial,0,pass.light->Snapshot());
    D3D12_CONSTANT_BUFFER_VIEW_DESC cb{slot->dispatch->GetGPUVirtualAddress(),256};
    good=good&&pass.light->ConfigureSecondary(pass.serial,cb,{slot->descriptors.Get(),1,1},{slot->descriptors.Get(),2,1});
    good=good&&composite.Copy(pass.serial,2,{slot->descriptors.Get(),4,1})&&composite.Copy(pass.serial,33,{slot->descriptors.Get(),1,1})&&composite.Copy(pass.serial,34,{slot->descriptors.Get(),3,1});
    PTGlass::DescriptorRange secondaryRange{},compositeRange{};D3D12_GPU_DESCRIPTOR_HANDLE secondaryTable{},compositeTable{};
    good=good&&Fresh(context,&secondaryRange)&&pass.light->Publish(pass.serial,secondaryRange,&secondaryTable)&&Fresh(context,&compositeRange)&&composite.PublishChecked(pass.serial,compositeRange,PTGlass::LightingTableLayout::NativeRequired|(1ull<<2)|(1ull<<33)|(1ull<<34)|(1ull<<41),&compositeTable);
    mirrorGuard=false;if(!good){Skip("lighting_tables");return;}
    auto* list=context.list;D3D12_RESOURCE_BARRIER uav{};uav.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&uav);
    Barrier(list,slot->hits.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    {PTGlass::ScopedLightingBinding binding(list,nativeLighting.Get(),context.gpu,secondaryLighting.Get(),secondaryTable);list->Dispatch(dims[0],dims[1],dims[2]);}
    Barrier(list,slot->radiance.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    {PTGlass::ScopedLightingBinding binding(list,nativeLighting.Get(),context.gpu,compositeLighting.Get(),compositeTable);list->Dispatch(dims[0],dims[1],dims[2]);}
    list->ResourceBarrier(1,&uav);
    Barrier(list,slot->hits.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Barrier(list,slot->radiance.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    finish.consumer=context.list;
    const auto n=dispatchCount.load();if(n<=4||n%240==0)Log("PT_GLASS_LIGHTING dispatch=%llu secondary=1 composite=1 native_diffuse_direct_emission_retained=1",n);
}
static void ObserveCompute(ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,void* result){
    if(!enabled||!desc||!result||desc->CS.BytecodeLength!=39720)return;char sha[65]{};
    if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||strcmp(sha,"b19de5e0b8a216076b6800790b100b91caa83814abdad3ad276736ba72b5e762"))return;
    std::lock_guard<std::mutex> guard(lock);if(nativeLighting)return;
    static_cast<IUnknown*>(result)->QueryInterface(IID_PPV_ARGS(&nativeLighting));lightingRoot=desc->pRootSignature;
    D3D12_COMPUTE_PIPELINE_STATE_DESC p=*desc;p.CachedPSO={};p.CS={secondaryBytes.data(),secondaryBytes.size()};
    const HRESULT a=ptReflectionOriginalCreateComputePipelineState(d,&p,IID_PPV_ARGS(&secondaryLighting));p.CS={compositeBytes.data(),compositeBytes.size()};
    const HRESULT b=ptReflectionOriginalCreateComputePipelineState(d,&p,IID_PPV_ARGS(&compositeLighting));
    Log("PT_GLASS_LIGHTING_PSO secondary_hr=0x%08lX composite_hr=0x%08lX",static_cast<unsigned long>(a),static_cast<unsigned long>(b));
}
static void InstallDevice(ID3D12Device* d){
    if(device||!enabled)return;device=d;ComPtr<ID3D12Device5> rayDevice;
    if(FAILED(d->QueryInterface(IID_PPV_ARGS(&rayDevice)))||
       !Patch(d,24,reinterpret_cast<void*>(&CopyHook),reinterpret_cast<void**>(&copySimple))||
       !Patch(d,17,reinterpret_cast<void*>(&CbvHook),reinterpret_cast<void**>(&createCbv))||
       !Patch(d,18,reinterpret_cast<void*>(&SrvHook),reinterpret_cast<void**>(&createSrv))||
       !Patch(d,19,reinterpret_cast<void*>(&UavHook),reinterpret_cast<void**>(&createUav))||
       !Patch(d,14,reinterpret_cast<void*>(&HeapHook),reinterpret_cast<void**>(&createHeap))||
       !Patch(d,23,reinterpret_cast<void*>(&CopyAllHook),reinterpret_cast<void**>(&copyAll))||
       !Patch(rayDevice.Get(),62,reinterpret_cast<void*>(&StateHook),reinterpret_cast<void**>(&createState)))enabled=false;
    Log("PT_GLASS_DEVICE_HOOKS ready=%u",unsigned(enabled.load()));
}
struct NativeCallPatch {
    unsigned char* site=nullptr;unsigned char original[6]{},replacement[6]{};bool installed=false;
    bool Prepare(unsigned char* at,unsigned slot,void* hook){
        site=at;memcpy(original,at,6);void* relay=AllocateExecutableRelayNear(at);if(!relay)return false;
        if(!PTGlass::EncodeVtableCall6(original,reinterpret_cast<UINT_PTR>(at),slot,reinterpret_cast<UINT_PTR>(relay),replacement))return false;
        unsigned char stub[14]{0xff,0x25,0,0,0,0};memcpy(stub+6,&hook,8);memcpy(relay,stub,14);
        DWORD old=0;return VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),relay,14);
    }
    bool Exchange(bool install){
        if(!site||(!install&&!installed))return !install;
        if(memcmp(site,install?original:replacement,6))return false;
        DWORD old=0;if(!VirtualProtect(site,6,PAGE_EXECUTE_READWRITE,&old))return false;
        memcpy(site,install?replacement:original,6);installed=install;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,6)!=FALSE;
        DWORD ignored=0;return VirtualProtect(site,6,old,&ignored)!=FALSE&&flushed;
    }
};
static NativeCallPatch nativeSetCall,nativeDispatchCall;
static bool Install(HMODULE renderer,HMODULE d3d){
    PTNativeJobRuntime::dispatchHookReady=false;
    if(!HashMatches(renderer,kRendererHash)||!HashMatches(d3d,kD3dHash)){
        Log("PT_GLASS_INSTALL ready=0 build=G3 reason=unsupported_module_hash");return false;
    }
    char sha[65]{};bool loaded=false;
    if(!PTReflectionLoadSidecar(L"pt_glass_raygen.dxil","GLASS_RAY",rayBytes,sha,loaded)||
       !PTReflectionLoadSidecar(L"pt_glass_secondary.dxbc","GLASS_LIGHT",secondaryBytes,sha,loaded)||
       !PTReflectionLoadSidecar(L"pt_glass_composite.dxbc","GLASS_COMPOSITE",compositeBytes,sha,loaded))return false;
    auto* b=reinterpret_cast<unsigned char*>(renderer);
    useNative=reinterpret_cast<UseFn>(*reinterpret_cast<void**>(b+0x5dfb18));rayNative=reinterpret_cast<RayFn>(*reinterpret_cast<void**>(b+0x5df4c8));
    bindNative=reinterpret_cast<BindFn>(b+0x1db750);lightNative=reinterpret_cast<LightFn>(*reinterpret_cast<void**>(b+0x5dfcc0));
    auto* db=reinterpret_cast<unsigned char*>(d3d);
    bool ready=nativeSetCall.Prepare(db+0x354a5,75,reinterpret_cast<void*>(&SetRayPsoHook))&&
       nativeDispatchCall.Prepare(db+0x358e4,76,reinterpret_cast<void*>(&RaysHook))&&usePatch.Prepare(b+0xa5440,reinterpret_cast<void**>(b+0x5dfb18),reinterpret_cast<void*>(useNative),reinterpret_cast<void*>(&UseHook))&&
       rayPatch.Prepare(b+0xa557e,reinterpret_cast<void**>(b+0x5df4c8),reinterpret_cast<void*>(rayNative),reinterpret_cast<void*>(&RayHook))&&
       lightPatch.Prepare(b+0xa5682,reinterpret_cast<void**>(b+0x5dfcc0),reinterpret_cast<void*>(lightNative),reinterpret_cast<void*>(&LightHook))&&
       RRAlbedoPrepareCallPatch(b+0xa562e,reinterpret_cast<void*>(bindNative),reinterpret_cast<void*>(&BindHook),&bindPatch);
    if(ready)ready=nativeSetCall.Exchange(true)&&nativeDispatchCall.Exchange(true)&&usePatch.Exchange(true)&&rayPatch.Exchange(true)&&lightPatch.Exchange(true)&&RRAlbedoExchangeCall(&bindPatch,true);
    if(!ready){nativeSetCall.Exchange(false);nativeDispatchCall.Exchange(false);usePatch.Exchange(false);rayPatch.Exchange(false);lightPatch.Exchange(false);RRAlbedoExchangeCall(&bindPatch,false);}
    enabled=ready;PTNativeJobRuntime::dispatchHookReady=ready;Log("PT_GLASS_INSTALL ready=%u build=G3 glossy_only=1 diffuse_removed=1 gpu_diagnostics=0 smoothness_min=0.8",unsigned(ready));
    if(ready){ID3D12Device* existing=nullptr;memcpy(&existing,reinterpret_cast<unsigned char*>(d3d)+kControlGlobalD3D12DevicePointerRva,sizeof(existing));if(existing)InstallDevice(existing);}
    return ready;
}
}
