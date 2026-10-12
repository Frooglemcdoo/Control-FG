#pragma once
#include "pt_scene_observer.h"
#include "pt_native_binding.h"
#include "pt_native_buffer_observer.h"
#include "pt_owned_scene_runtime.h"
#include "pt_scene_topology_runtime.h"
// M11 diagnostics are never used as rendering bindings.
namespace PTSceneBindingRuntime {
using PTSceneObserver::Event;
static PTOwnedScene::Runtime ownedScene;
static PTSceneTopology::Runtime topology;
constexpr unsigned Capacity=196608;
static Event* events=nullptr;
static unsigned stored=0,flushed=0,geometryRows=0,bindingRows=0,resourceRows=0;
static std::atomic<UINT64> bindingCalls{0},fillCalls[2]{},invalidCounts{0};
static ULONGLONG lastTableTick=0;static unsigned tables=0;
static SRWLOCK eventLock=SRWLOCK_INIT;
static std::atomic<UINT64> calls{0},dropped{0},resourceCalls{0};
static std::atomic<bool> recording{false};
static HANDLE output=INVALID_HANDLE_VALUE;
static PTGlassRuntime::NativeCallPatch buildCall[2];
static void Sink(const Event& e) noexcept {
    if(e.kind==3&&recording.load(std::memory_order_relaxed))ownedScene.Geometry(e);
    AcquireSRWLockExclusive(&eventLock);
    if(recording.load(std::memory_order_relaxed)){
        unsigned& quota=e.kind>=8?resourceRows:(e.kind>=5?bindingRows:geometryRows);
        if(stored<Capacity&&quota<65536){events[stored++]=e;++quota;}else ++dropped;
    }
    ReleaseSRWLockExclusive(&eventLock);
}
static void Hook(UINT site,ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,UINT count,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* info){
    const DWORD error=GetLastError();
    if(recording.load(std::memory_order_relaxed)){topology.Build(desc);PTSceneObserver::Observe(list,desc,site,++calls,&Sink);}
    SetLastError(error);PTSceneObserver::Forward(list,desc,count,info);
}
static void STDMETHODCALLTYPE Hook0(ID3D12GraphicsCommandList4* l,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT n,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p){Hook(0x33593,l,d,n,p);}
static void STDMETHODCALLTYPE Hook1(ID3D12GraphicsCommandList4* l,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT n,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p){Hook(0x337cc,l,d,n,p);}
static bool Write(const char* text,DWORD size) noexcept {DWORD written=0;return WriteFile(output,text,size,&written,nullptr)&&written==size;}
static bool Flush() noexcept {
    unsigned available=0;AcquireSRWLockShared(&eventLock);available=stored;ReleaseSRWLockShared(&eventLock);
    for(;flushed<available;++flushed){
        const auto& e=events[flushed];char line[1536];
        const int n=_snprintf_s(line,sizeof(line),_TRUNCATE,
            "{\"kind\":%u,\"sequence\":%llu,\"site\":%u,\"list\":\"%llx\",\"dest\":\"%llx\",\"source\":\"%llx\",\"flags\":%u,\"count\":%u,\"layout\":%u,\"ordinal\":%u,\"geometry_type\":%u,\"geometry_flags\":%u,\"index\":\"%llx\",\"position\":\"%llx\",\"transform\":\"%llx\",\"stride\":%llu,\"index_format\":%u,\"vertex_format\":%u,\"index_count\":%u,\"vertex_count\":%u,\"attributes\":\"%llx\",\"constants\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}\n",
            e.kind,e.sequence,e.site,e.list,e.dest,e.source,e.buildFlags,e.count,e.layout,e.ordinal,e.geometryType,e.geometryFlags,e.index,e.position,e.transform,e.stride,e.indexFormat,e.vertexFormat,e.indexCount,e.vertexCount,e.attributes,e.constants[0],e.constants[1],e.constants[2],e.constants[3],e.constants[4],e.constants[5],e.constants[6],e.constants[7],e.constants[8],e.constants[9]);
        if(n<=0||!Write(line,static_cast<DWORD>(n)))return false;
    }
    return true;
}
static DWORD WINAPI Worker(void*) {
    const ULONGLONG start=GetTickCount64();bool good=true;
    while(GetTickCount64()-start<120000){Sleep(250);if(!(good=Flush()))break;}
    AcquireSRWLockExclusive(&eventLock);recording=false;ReleaseSRWLockExclusive(&eventLock);
    const auto stats=ownedScene.Stop();
    const auto links=topology.Stop();
    good=Flush()&&good;
    char registry[1024];const int rn=_snprintf_s(registry,sizeof(registry),_TRUNCATE,
        "{\"kind\":\"registry\",\"retained\":%llu,\"rejected\":%llu,\"budget\":%llu,\"observed\":%llu,\"unowned_geometry\":%llu,\"validated_release\":%llu,\"mismatched_release\":%llu,\"peak_bytes\":%llu,\"candidate\":%llu,\"missing_allocation\":%llu,\"missing_geometry\":%llu,\"ambiguous_geometry\":%llu,\"invalid_binding\":%llu,\"invalid_geometry\":%llu,\"gpu_work\":false,\"renderer_ready\":false}\n",
        stats.retained,stats.rejected,stats.budget,stats.observed,stats.unownedGeometry,stats.validatedRelease,stats.mismatchedRelease,stats.peakBytes,
        stats.resolved[0],stats.resolved[1],stats.resolved[2],stats.resolved[3],stats.resolved[4],stats.resolved[5]);
    if(rn<=0||!Write(registry,static_cast<DWORD>(rn)))good=false;
    Log("PT_SCENE_M11_REGISTRY retained=%llu candidates=%llu missing_allocation=%llu ambiguous=%llu peak_bytes=%llu renderer_ready=0",stats.retained,stats.resolved[0],stats.resolved[1],stats.resolved[3],stats.peakBytes);
    char relation[1024];const int tn=_snprintf_s(relation,sizeof(relation),_TRUNCATE,
        "{\"kind\":\"topology\",\"uploads\":%llu,\"read_errors\":%llu,\"blas_builds\":%llu,\"compactions\":%llu,\"missing_compactions\":%llu,\"tlas_builds\":%llu,\"missing_upload\":%llu,\"tables\":%llu,\"instances\":%llu,\"matched\":%llu,\"missing_blas\":%llu,\"out_of_range\":%llu,\"mismatch\":%llu,\"conflict\":%llu,\"masked\":%llu,\"scope\":\"preceding_cpu_snapshots_not_gpu_order\",\"renderer_ready\":false}\n",
        links.uploads,links.readErrors,links.builds,links.compactions,links.missingCompactions,links.tlas,links.missingUpload,links.tables,
        links.relation.instances,links.relation.matched,links.relation.missingBlas,links.relation.outOfRange,links.relation.mismatch,links.relation.conflict,links.relation.masked);
    if(tn<=0||!Write(relation,static_cast<DWORD>(tn)))good=false;
    char end[512];const int n=_snprintf_s(end,sizeof(end),_TRUNCATE,"{\"kind\":\"end\",\"inline_calls\":%llu,\"worker_calls\":%llu,\"invalid_counts\":%llu,\"calls\":%llu,\"records\":%u,\"dropped\":%llu,\"write_ok\":%s,\"scope\":\"recording_only_not_gpu_completion\"}\n",fillCalls[0].load(),fillCalls[1].load(),invalidCounts.load(),calls.load(),flushed,dropped.load(),good?"true":"false");
    if(n>0)good=Write(end,static_cast<DWORD>(n))&&good;
    FlushFileBuffers(output);CloseHandle(output);output=INVALID_HANDLE_VALUE;
    Log("PT_SCENE_M11_FINISHED calls=%llu records=%u dropped=%llu write_ok=%u recording_only=1 inline_calls=%llu worker_calls=%llu invalid_counts=%llu",calls.load(),flushed,dropped.load(),unsigned(good),fillCalls[0].load(),fillCalls[1].load(),invalidCounts.load());return 0;
}
// Exact internal writer call at d3d+357c6, seven native arguments.
using Fill=PTNativeBinding::Fill;
static Fill nativeFill=nullptr;static RRAlbedoCallPatch fillPatch[2]{};
static void ObserveBindings(UINT site,void* buffer,void* state,int bytes,int rays,int count,const void* bindings) noexcept {
    if(!recording)return;
    if(count<=0){++invalidCounts;return;}
    const auto now=GetTickCount64();bool capture=false;
    AcquireSRWLockExclusive(&eventLock);
    if(tables<8&&(tables==0||now-lastTableTick>=10000)){lastTableTick=now;++tables;capture=true;}
    ReleaseSRWLockExclusive(&eventLock);if(!capture)return;
    Event h{};h.kind=5;h.sequence=++bindingCalls;h.site=site;h.list=reinterpret_cast<UINT_PTR>(buffer);
    h.dest=reinterpret_cast<UINT_PTR>(state);h.count=static_cast<UINT>(count);h.stride=static_cast<UINT>(bytes);h.layout=static_cast<UINT>(rays);
    h.vertexCount=(std::min)(h.count,8191u);Sink(h);
    std::vector<PTNativeBinding::Binding> snapshot;snapshot.reserve(h.vertexCount);
    for(UINT i=0;i<h.vertexCount;++i){
        PTNativeBinding::Binding b{};Event e=h;e.kind=6;e.ordinal=i;
        if(!PTNativeBinding::Read(bindings,i,b)){e.kind=7;Sink(e);break;}
        snapshot.push_back(b);e.geometryType=b.type;e.geometryFlags=b.tag;e.index=b.indices;e.attributes=b.attributes;
        std::memcpy(e.constants,b.constants,sizeof(b.constants));Sink(e);
        PTSceneAdmission::Record candidate{};const auto status=ownedScene.Resolve(b,candidate);
        (void)status; // Aggregate resolution counts are written independently of event quotas.
    }
    if(snapshot.size()==h.count)topology.Table(snapshot,static_cast<unsigned>(rays));
}
static void FillHook(unsigned route,int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    const auto ticket=PTNativeJobRuntime::Begin(mode,{reinterpret_cast<UINT_PTR>(buffer),reinterpret_cast<UINT_PTR>(state),reinterpret_cast<UINT_PTR>(bindings),bytes,rays,count});
    const DWORD error=GetLastError();
    if(recording.load(std::memory_order_relaxed)){
        ++fillCalls[route];ObserveBindings(route==0?0x357c6u:0x374a1u,buffer,state,bytes,rays,count,bindings);
    }
    SetLastError(error);
    PTNativeBinding::Forward(nativeFill,mode,buffer,state,bytes,rays,count,bindings);
    PTNativeJobRuntime::Finish(mode,ticket);
}
static void FillInline(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    FillHook(0,mode,buffer,state,bytes,rays,count,bindings);
}
static void FillWorker(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    FillHook(1,mode,buffer,state,bytes,rays,count,bindings);
}
static RRAlbedoCallPatch resourcePatch[27]{};
static PTNativeBufferObserver::CreateGeneral createGeneral=nullptr;
static PTNativeBufferObserver::CreateRay createRay=nullptr;
static PTNativeBufferObserver::Release releaseBuffer=nullptr;
static void ObserveResource(void* buffer,UINT kind,UINT site) noexcept {
    if(!recording.load(std::memory_order_relaxed))return;
    Event e{};e.kind=kind;e.site=site;e.sequence=++resourceCalls;e.list=reinterpret_cast<UINT_PTR>(buffer);
    PTNativeBufferObserver::Snapshot b{};
    if(!PTNativeBufferObserver::Read(buffer,b))e.kind=10;
    else {e.dest=b.resource;e.index=b.address;e.stride=b.bytes;
        if(kind==8){topology.Invalidate(b.address,b.bytes);const auto status=ownedScene.Created(reinterpret_cast<UINT_PTR>(buffer),reinterpret_cast<IUnknown*>(b.resource),b.address,b.bytes);
            Event owned=e;owned.kind=12;owned.geometryFlags=status;Sink(owned);}
    }
    Sink(e);
}
static void CreateGeneralHook(std::uint64_t bytes,std::uint64_t stride,unsigned flags,unsigned format,std::uint64_t unused0,std::uint64_t unused1,void* buffer){
    PTNativeBufferObserver::ForwardGeneral(createGeneral,bytes,stride,flags,format,unused0,unused1,buffer);
    const DWORD error=GetLastError();ObserveResource(buffer,8,0x26030);SetLastError(error);
}
static void CreateRayHook(int bytes,int type,void* buffer,int flags){
    PTNativeBufferObserver::ForwardRay(createRay,bytes,type,buffer,flags);
    const DWORD error=GetLastError();ObserveResource(buffer,8,0x26360);SetLastError(error);
}
static void ReleaseHook(void* buffer){
    const DWORD error=GetLastError();ObserveResource(buffer,9,0x25f00);
    auto lease=ownedScene.BeforeRelease(reinterpret_cast<UINT_PTR>(buffer));
    if(lease.resource)topology.Invalidate(lease.address,lease.bytes);
    SetLastError(error);releaseBuffer(buffer);
    const DWORD afterError=GetLastError();const auto status=ownedScene.AfterRelease(lease);
    if(status){
        Event e{};e.kind=11;e.site=0x25f00;e.sequence=++resourceCalls;e.list=reinterpret_cast<UINT_PTR>(buffer);
        e.index=lease.address;e.stride=lease.bytes;e.geometryFlags=status;Sink(e);
    }
    SetLastError(afterError);
}
// Capture Control's exact instance upload source before its native memcpy.
// No Map call, GPU readback, or read from write-combined upload memory.
using LockInstances=void(*)(void*,UINT64,UINT64,unsigned,void**);
using CopyInstances=void*(*)(void*,const void*,std::size_t);
static LockInstances lockInstances=nullptr;static CopyInstances copyInstances=nullptr;
static RRIndirectCallPatch instanceLockPatch;static RRAlbedoCallPatch instanceCopyPatch{};
static PTGlassRuntime::NativeCallPatch compactPatch;
using RunJob=void(*)(void*,void*);
static RunJob runJob=nullptr;static RRIndirectCallPatch runJobPatch;
static void RunJobHook(void* system,void* handle){PTNativeJobRuntime::Scheduling(handle);runJob(system,handle);}
struct LockedInstances {void* mapped=nullptr;UINT64 address=0,bytes=0;};
static thread_local LockedInstances instanceUpload;
static bool ReadMapped(void** mapped,void*& out)noexcept{
    __try{if(!mapped)return false;out=*mapped;return out!=nullptr;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void LockInstancesHook(void* buffer,UINT64 offset,UINT64 bytes,unsigned mode,void** mapped){
    lockInstances(buffer,offset,bytes,mode,mapped);const DWORD error=GetLastError();instanceUpload={};
    PTNativeBufferObserver::Snapshot b{};void* pointer=nullptr;
    if(recording&&offset==0&&bytes&&bytes<=2ull*1024*1024&&bytes%64==0&&
       PTNativeBufferObserver::Read(buffer,b)&&bytes<=b.bytes&&ReadMapped(mapped,pointer))instanceUpload={pointer,b.address,bytes};
    SetLastError(error);
}
static void* CopyInstancesHook(void* dst,const void* src,std::size_t bytes){
    const DWORD error=GetLastError();
    if(recording&&dst==instanceUpload.mapped&&bytes==instanceUpload.bytes)topology.UploadInstances(instanceUpload.address,src,bytes);
    instanceUpload={};SetLastError(error);return copyInstances(dst,src,bytes);
}
using Compact=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,UINT64,UINT64,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE);
static void STDMETHODCALLTYPE CompactHook(ID3D12GraphicsCommandList4* list,UINT64 dst,UINT64 src,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode){
    const DWORD error=GetLastError();if(recording&&mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)topology.Compact(dst,src);
    SetLastError(error);reinterpret_cast<Compact>((*reinterpret_cast<void***>(list))[74])(list,dst,src,mode);
}
static bool PrepareTopology(unsigned char* d3d){
    runJob=reinterpret_cast<RunJob>(*reinterpret_cast<void**>(d3d+0x5d650));
    auto* renderer=reinterpret_cast<unsigned char*>(verifiedRenderer);
    lockInstances=reinterpret_cast<LockInstances>(*reinterpret_cast<void**>(renderer+0x5dfe70));
    copyInstances=reinterpret_cast<CopyInstances>(renderer+0x588e0e);
    return instanceLockPatch.Prepare(renderer+0xa8972,reinterpret_cast<void**>(renderer+0x5dfe70),reinterpret_cast<void*>(lockInstances),reinterpret_cast<void*>(&LockInstancesHook))&&
        RRAlbedoPrepareCallPatch(renderer+0xa8987,reinterpret_cast<void*>(copyInstances),reinterpret_cast<void*>(&CopyInstancesHook),&instanceCopyPatch)&&
        compactPatch.Prepare(d3d+0x3369d,74,reinterpret_cast<void*>(&CompactHook))&&
        runJobPatch.Prepare(d3d+0x359d9,reinterpret_cast<void**>(d3d+0x5d650),reinterpret_cast<void*>(runJob),reinterpret_cast<void*>(&RunJobHook));
}
static bool PrepareResources(unsigned char* b){
    createGeneral=reinterpret_cast<PTNativeBufferObserver::CreateGeneral>(b+0x26030);
    createRay=reinterpret_cast<PTNativeBufferObserver::CreateRay>(b+0x26360);
    releaseBuffer=reinterpret_cast<PTNativeBufferObserver::Release>(b+0x25f00);
    unsigned i=0;
    for(unsigned rva:{0x25217u,0x2527fu,0x252f1u,0x25367u,0x253d1u,0x2544bu,0x254bbu,0x2551fu})
        if(!RRAlbedoPrepareCallPatch(b+rva,b+0x26030,reinterpret_cast<void*>(&CreateGeneralHook),&resourcePatch[i++]))return false;
    for(unsigned rva:{0x2824fu,0x2ae34u,0x35763u})
        if(!RRAlbedoPrepareCallPatch(b+rva,b+0x26360,reinterpret_cast<void*>(&CreateRayHook),&resourcePatch[i++]))return false;
    for(unsigned rva:{0x17c25u,0x17c67u,0x17d6eu,0x17f0bu,0x25200u,0x25268u,0x252d8u,0x25350u,0x253b8u,0x25430u,0x254a0u,0x25508u,0x2565du,0x28233u,0x2ae20u,0x35752u})
        if(!RRAlbedoPrepareCallPatch(b+rva,b+0x25f00,reinterpret_cast<void*>(&ReleaseHook),&resourcePatch[i++]))return false;
    return i==27;
}
static bool InstallResources(){for(auto& p:resourcePatch)if(!RRAlbedoExchangeCall(&p,true))return false;return true;}
static void RestoreCalls(){
    PTNativeJobRuntime::workerHooksReady=false;
    if(runJobPatch.changed)runJobPatch.Exchange(false);
    if(instanceLockPatch.changed)instanceLockPatch.Exchange(false);
    if(instanceCopyPatch.site)RRAlbedoExchangeCall(&instanceCopyPatch,false);compactPatch.Exchange(false);
    for(auto& p:resourcePatch)if(p.site)RRAlbedoExchangeCall(&p,false);
    for(auto& p:fillPatch)if(p.site)RRAlbedoExchangeCall(&p,false);
    buildCall[1].Exchange(false);buildCall[0].Exchange(false);
}
static bool Install(HMODULE d3d){
    if(!HashMatches(d3d,kD3dHash)||!HashMatches(verifiedRenderer,kRendererHash)){Log("PT_SCENE_M11_INSTALL ready=0 reason=unsupported_module_hash");return false;}
    wchar_t dir[32768]{};const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",dir,_countof(dir));if(!len||len>=_countof(dir))return false;
    wchar_t name[160]{};swprintf_s(name,L"\\ControlFGProbe\\pt-scene-m11-%llu-%lu.jsonl",GetTickCount64(),GetCurrentProcessId());
    output=CreateFileW((std::wstring(dir)+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(output==INVALID_HANDLE_VALUE)return false;
    events=new(std::nothrow) Event[Capacity];if(!events){CloseHandle(output);output=INVALID_HANDLE_VALUE;return false;}
    const char header[]="{\"kind\":\"header\",\"schema\":1,\"build\":\"M11\",\"limit\":196608,\"geometry_limit\":65536,\"binding_limit\":65536,\"resource_limit\":65536,\"seconds\":120,\"scope\":\"recording_only_not_gpu_completion\"}\n";
    bool ready=Write(header,static_cast<DWORD>(sizeof(header)-1));auto* b=reinterpret_cast<unsigned char*>(d3d);
    nativeFill=reinterpret_cast<Fill>(b+0x33a40);
    ready=ready&&RRAlbedoPrepareCallPatch(b+0x357c6,reinterpret_cast<void*>(nativeFill),reinterpret_cast<void*>(&FillInline),&fillPatch[0]);
    ready=ready&&RRAlbedoPrepareCallPatch(b+0x374a1,reinterpret_cast<void*>(nativeFill),reinterpret_cast<void*>(&FillWorker),&fillPatch[1]);
    ready=ready&&buildCall[0].Prepare(b+0x33593,72,reinterpret_cast<void*>(&Hook0))&&buildCall[1].Prepare(b+0x337cc,72,reinterpret_cast<void*>(&Hook1));
    ready=ready&&PrepareResources(b)&&PrepareTopology(b);
    if(ready)ready=runJobPatch.Exchange(true)&&instanceLockPatch.Exchange(true)&&RRAlbedoExchangeCall(&instanceCopyPatch,true)&&compactPatch.Exchange(true)&&InstallResources()&&buildCall[0].Exchange(true)&&buildCall[1].Exchange(true)&&RRAlbedoExchangeCall(&fillPatch[0],true)&&RRAlbedoExchangeCall(&fillPatch[1],true);
    if(!ready){RestoreCalls();CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M11_INSTALL ready=0 reason=call_patch");return false;}
    recording=true;HANDLE thread=CreateThread(nullptr,0,&Worker,nullptr,0,nullptr);
    if(!thread){recording=false;RestoreCalls();CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M11_INSTALL ready=0 reason=writer_thread");return false;}
    PTNativeJobRuntime::workerHooksReady=true;
    CloseHandle(thread);Log("PT_SCENE_M11_INSTALL ready=1 sites=33593,337cc,357c6,374a1 resource_sites=27 gpu_changes=0 duration_seconds=120 record_limit=%u",Capacity);return true;
}
}
