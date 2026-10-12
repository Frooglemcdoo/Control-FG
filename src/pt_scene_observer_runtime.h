#pragma once
#include "pt_scene_observer.h"
// M6 diagnostics are never used as rendering bindings.
namespace PTSceneObserverRuntime {
using PTSceneObserver::Event;
constexpr unsigned Capacity=65536;
static Event* events=nullptr;
static unsigned stored=0,flushed=0;
static SRWLOCK eventLock=SRWLOCK_INIT;
static std::atomic<UINT64> calls{0},dropped{0};
static std::atomic<bool> recording{false};
static HANDLE output=INVALID_HANDLE_VALUE;
static PTGlassRuntime::NativeCallPatch buildCall[2];
static void Sink(const Event& e) noexcept {
    AcquireSRWLockExclusive(&eventLock);
    if(recording.load(std::memory_order_relaxed)){
        if(stored<Capacity)events[stored++]=e;else ++dropped;
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
        const auto& e=events[flushed];char line[1024];
        const int n=_snprintf_s(line,sizeof(line),_TRUNCATE,
            "{\"kind\":%u,\"sequence\":%llu,\"site\":%u,\"list\":\"%llx\",\"dest\":\"%llx\",\"source\":\"%llx\",\"flags\":%u,\"count\":%u,\"layout\":%u,\"ordinal\":%u,\"geometry_type\":%u,\"geometry_flags\":%u,\"index\":\"%llx\",\"position\":\"%llx\",\"transform\":\"%llx\",\"stride\":%llu,\"index_format\":%u,\"vertex_format\":%u,\"index_count\":%u,\"vertex_count\":%u}\n",
            e.kind,e.sequence,e.site,e.list,e.dest,e.source,e.buildFlags,e.count,e.layout,e.ordinal,e.geometryType,e.geometryFlags,e.index,e.position,e.transform,e.stride,e.indexFormat,e.vertexFormat,e.indexCount,e.vertexCount);
        if(n<=0||!Write(line,static_cast<DWORD>(n)))return false;
    }
    return true;
}
static DWORD WINAPI Worker(void*) {
    const ULONGLONG start=GetTickCount64();bool good=true;
    while(GetTickCount64()-start<180000){Sleep(250);if(!(good=Flush()))break;if(flushed==Capacity)break;}
    AcquireSRWLockExclusive(&eventLock);recording=false;ReleaseSRWLockExclusive(&eventLock);
    good=Flush()&&good;
    char end[256];const int n=_snprintf_s(end,sizeof(end),_TRUNCATE,"{\"kind\":\"end\",\"calls\":%llu,\"records\":%u,\"dropped\":%llu,\"write_ok\":%s,\"scope\":\"recording_only_not_gpu_completion\"}\n",calls.load(),flushed,dropped.load(),good?"true":"false");
    if(n>0)good=Write(end,static_cast<DWORD>(n))&&good;
    FlushFileBuffers(output);CloseHandle(output);output=INVALID_HANDLE_VALUE;
    Log("PT_SCENE_M6_FINISHED calls=%llu records=%u dropped=%llu write_ok=%u recording_only=1",calls.load(),flushed,dropped.load(),unsigned(good));return 0;
}
static bool Install(HMODULE d3d){
    if(!HashMatches(d3d,kD3dHash)){Log("PT_SCENE_M6_INSTALL ready=0 reason=unsupported_module_hash");return false;}
    wchar_t dir[32768]{};const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",dir,_countof(dir));if(!len||len>=_countof(dir))return false;
    wchar_t name[160]{};swprintf_s(name,L"\\ControlFGProbe\\pt-scene-m6-%llu-%lu.jsonl",GetTickCount64(),GetCurrentProcessId());
    output=CreateFileW((std::wstring(dir)+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(output==INVALID_HANDLE_VALUE)return false;
    events=new(std::nothrow) Event[Capacity];if(!events){CloseHandle(output);output=INVALID_HANDLE_VALUE;return false;}
    const char header[]="{\"kind\":\"header\",\"schema\":1,\"build\":\"M6\",\"limit\":65536,\"seconds\":180,\"scope\":\"recording_only_not_gpu_completion\"}\n";
    bool ready=Write(header,static_cast<DWORD>(sizeof(header)-1));auto* b=reinterpret_cast<unsigned char*>(d3d);
    ready=ready&&buildCall[0].Prepare(b+0x33593,72,reinterpret_cast<void*>(&Hook0))&&buildCall[1].Prepare(b+0x337cc,72,reinterpret_cast<void*>(&Hook1));
    if(ready)ready=buildCall[0].Exchange(true)&&buildCall[1].Exchange(true);
    if(!ready){buildCall[1].Exchange(false);buildCall[0].Exchange(false);CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M6_INSTALL ready=0 reason=call_patch");return false;}
    recording=true;HANDLE thread=CreateThread(nullptr,0,&Worker,nullptr,0,nullptr);
    if(!thread){recording=false;CloseHandle(output);output=INVALID_HANDLE_VALUE;Log("PT_SCENE_M6_INSTALL ready=0 reason=writer_thread");return false;}
    CloseHandle(thread);Log("PT_SCENE_M6_INSTALL ready=1 sites=33593,337cc gpu_changes=0 duration_seconds=180 record_limit=%u",Capacity);return true;
}
}
