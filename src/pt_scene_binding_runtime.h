#pragma once
#include "pt_scene_observer.h"
#include "pt_native_binding.h"
// M7R1 diagnostics are never used as rendering bindings.
namespace PTSceneBindingRuntime {
using PTSceneObserver::Event;
constexpr unsigned Capacity=131072;
static Event* events=nullptr;
static unsigned stored=0,flushed=0,geometryRows=0,bindingRows=0;
static std::atomic<UINT64> bindingCalls{0},fillCalls[2]{},invalidCounts{0};
static ULONGLONG lastTableTick=0;static unsigned tables=0;
static SRWLOCK eventLock=SRWLOCK_INIT;
static std::atomic<UINT64> calls{0},dropped{0};
static std::atomic<bool> recording{false};
static HANDLE output=INVALID_HANDLE_VALUE;
static PTGlassRuntime::NativeCallPatch buildCall[2];
static void Sink(const Event& e) noexcept {
    AcquireSRWLockExclusive(&eventLock);
    if(recording.load(std::memory_order_relaxed)){
        unsigned& quota=e.kind>=5?bindingRows:geometryRows;
        if(stored<Capacity&&quota<65536){events[stored++]=e;++quota;}else ++dropped;
    }
    ReleaseSRWLockExclusive(&eventLock);
}
static void Hook(UINT site,ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,UINT count,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* info){
    const DWORD error=GetLastError();
    if(recording.load(std::memory_order_relaxed))PTSceneObserver::Observe(list,desc,site,++calls,&Sink);
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
    while(GetTickCount64()-start<180000){Sleep(250);if(!(good=Flush()))break;}
    AcquireSRWLockExclusive(&eventLock);recording=false;ReleaseSRWLockExclusive(&eventLock);
    good=Flush()&&good;
    char end[512];const int n=_snprintf_s(end,sizeof(end),_TRUNCATE,"{\"kind\":\"end\",\"inline_calls\":%llu,\"worker_calls\":%llu,\"invalid_counts\":%llu,\"calls\":%llu,\"records\":%u,\"dropped\":%llu,\"write_ok\":%s,\"scope\":\"recording_only_not_gpu_completion\"}\n",fillCalls[0].load(),fillCalls[1].load(),invalidCounts.load(),calls.load(),flushed,dropped.load(),good?"true":"false");
    if(n>0)good=Write(end,static_cast<DWORD>(n))&&good;
    FlushFileBuffers(output);CloseHandle(output);output=INVALID_HANDLE_VALUE;
    Log("PT_SCENE_M7R1_FINISHED calls=%llu records=%u dropped=%llu write_ok=%u recording_only=1 inline_calls=%llu worker_calls=%llu invalid_counts=%llu",calls.load(),flushed,dropped.load(),unsigned(good),fillCalls[0].load(),fillCalls[1].load(),invalidCounts.load());return 0;
}
// Exact internal writer call at d3d+357c6, seven native arguments.
using Fill=PTNativeBinding::Fill;
static Fill nativeFill=nullptr;static RRAlbedoCallPatch fillPatch[2]{};
static void ObserveBindings(UINT site,void* buffer,void* state,int bytes,int rays,int count,const void* bindings) noexcept {
    if(!recording)return;
    if(count<=0){++invalidCounts;return;}
    const auto now=GetTickCount64();bool capture=false;
    AcquireSRWLockExclusive(&eventLock);
    if(tables<8&&(tables==0||now-lastTableTick>=15000)){lastTableTick=now;++tables;capture=true;}
    ReleaseSRWLockExclusive(&eventLock);if(!capture)return;
    Event h{};h.kind=5;h.sequence=++bindingCalls;h.site=site;h.list=reinterpret_cast<UINT_PTR>(buffer);
    h.dest=reinterpret_cast<UINT_PTR>(state);h.count=static_cast<UINT>(count);h.stride=static_cast<UINT>(bytes);h.layout=static_cast<UINT>(rays);
    h.vertexCount=(std::min)(h.count,8191u);Sink(h);
    for(UINT i=0;i<h.vertexCount;++i){
        PTNativeBinding::Binding b{};Event e=h;e.kind=6;e.ordinal=i;
        if(!PTNativeBinding::Read(bindings,i,b)){e.kind=7;Sink(e);break;}
        e.geometryType=b.type;e.geometryFlags=b.tag;e.index=b.indices;e.attributes=b.attributes;
        std::memcpy(e.constants,b.constants,sizeof(b.constants));Sink(e);
    }
}
static void FillHook(unsigned route,int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    const DWORD error=GetLastError();
    if(recording.load(std::memory_order_relaxed)){
        ++fillCalls[route];ObserveBindings(route==0?0x357c6u:0x374a1u,buffer,state,bytes,rays,count,bindings);
    }
    SetLastError(error);
    PTNativeBinding::Forward(nativeFill,mode,buffer,state,bytes,rays,count,bindings);
}
static void FillInline(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    FillHook(0,mode,buffer,state,bytes,rays,count,bindings);
}
static void FillWorker(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings){
    FillHook(1,mode,buffer,state,bytes,rays,count,bindings);
}
static void RestoreCalls(){
    for(auto& p:fillPatch)if(p.site)RRAlbedoExchangeCall(&p,false);
    buildCall[1].Exchange(false);buildCall[0].Exchange(false);
}
static bool Install(HMODULE d3d){
    if(!HashMatches(d3d,kD3dHash)){Log("PT_SCENE_M7R1_INSTALL ready=0 reason=unsupported_module_hash");return false;}
    wchar_t dir[32768]{};const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",dir,_countof(dir));if(!len||len>=_countof(dir))return false;
    wchar_t name[160]{};swprintf_s(name,L"\\ControlFGProbe\\pt-scene-m7r1-%llu-%lu.jsonl",GetTickCount64(),GetCurrentProcessId());
    output=CreateFileW((std::wstring(dir)+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(output==INVALID_HANDLE_VALUE)return false;
    events=new(std::nothrow) Event[Capacity];if(!events){CloseHandle(output);output=INVALID_HANDLE_VALUE;return false;}
    const char header[]="{\"kind\":\"header\",\"schema\":1,\"build\":\"M7R1\",\"limit\":131072,\"geometry_limit\":65536,\"binding_limit\":65536,\"seconds\":180,\"scope\":\"recording_only_not_gpu_completion\"}\n";
    bool ready=Write(header,static_cast<DWORD>(sizeof(header)-1));auto* b=reinterpret_cast<unsigned char*>(d3d);
    nativeFill=reinterpret_cast<Fill>(b+0x33a40);
    ready=ready&&RRAlbedoPrepareCallPatch(b+0x357c6,reinterpret_cast<void*>(nativeFill),reinterpret_cast<void*>(&FillInline),&fillPatch[0]);
    ready=ready&&RRAlbedoPrepareCallPatch(b+0x374a1,reinterpret_cast<void*>(nativeFill),reinterpret_cast<void*>(&FillWorker),&fillPatch[1]);
    ready=ready&&buildCall[0].Prepare(b+0x33593,72,reinterpret_cast<void*>(&Hook0))&&buildCall[1].Prepare(b+0x337cc,72,reinterpret_cast<void*>(&Hook1));
    if(ready)ready=buildCall[0].Exchange(true)&&buildCall[1].Exchange(true)&&RRAlbedoExchangeCall(&fillPatch[0],true)&&RRAlbedoExchangeCall(&fillPatch[1],true);
    if(!ready){RestoreCalls();CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M7R1_INSTALL ready=0 reason=call_patch");return false;}
    recording=true;HANDLE thread=CreateThread(nullptr,0,&Worker,nullptr,0,nullptr);
    if(!thread){recording=false;RestoreCalls();CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M7R1_INSTALL ready=0 reason=writer_thread");return false;}
    CloseHandle(thread);Log("PT_SCENE_M7R1_INSTALL ready=1 sites=33593,337cc,357c6,374a1 gpu_changes=0 duration_seconds=180 record_limit=%u",Capacity);return true;
}
}
