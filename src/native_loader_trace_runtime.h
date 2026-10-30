#pragma once
#include "native_loader_trace_metrics.h"
// Included after native_cpu_runtime.h: startup call patch helpers already exist.
namespace native_loader_trace {
using Callback=void(*)(void*,void*);
inline Callback construct=nullptr,worker=nullptr,loaderWorker=nullptr;
using Request=std::int64_t(*)(void*,void*);
inline Request request=nullptr;
inline thread_local std::uintptr_t requestOwner=0;
inline NativeCpuCallPatch patches[5]{};
inline std::int64_t RequestHook(void* self,void* arguments){
    struct Scope {std::uintptr_t previous;~Scope(){requestOwner=previous;}} scope{requestOwner};
    requestOwner=reinterpret_cast<std::uintptr_t>(self);return request(self,arguments);
}
inline loader_trace::Tracker tracker;
inline std::atomic<loader_trace::U64> workerCount{0},workerTicks{0},workerMax{0},inlineCount{0},inlineTicks{0},inlineMax{0};
inline bool installed=false;
inline loader_trace::U64 Tick(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return static_cast<loader_trace::U64>(value.QuadPart);}
inline loader_trace::U64 Field(void* request,unsigned offset){loader_trace::U64 value=0;memcpy(&value,static_cast<unsigned char*>(request)+offset,sizeof(value));return value;}
inline void Construct(void* arguments,void* request){
    construct(arguments,request);const DWORD error=GetLastError();
    if(requestOwner)tracker.Enqueue(Field(request,0x178),Tick(),Field(request,0x150),requestOwner);SetLastError(error);
}
inline void Execute(void* self,void* request,Callback original,bool onLoader){
    const DWORD incoming=GetLastError();const auto begin=Tick();
    const bool matched=tracker.Begin(Field(request,0x178),begin,reinterpret_cast<std::uintptr_t>(self));
    SetLastError(incoming);original(self,request);const DWORD error=GetLastError();
    if(matched){const auto elapsed=Tick()-begin;
        auto& count=onLoader?inlineCount:workerCount;auto& ticks=onLoader?inlineTicks:workerTicks;auto& maximum=onLoader?inlineMax:workerMax;
        count.fetch_add(1,std::memory_order_relaxed);ticks.fetch_add(elapsed,std::memory_order_relaxed);loader_trace::Tracker::Max(maximum,elapsed);
    }
    SetLastError(error);
}
inline void Worker(void* self,void* request){Execute(self,request,worker,false);}
inline void LoaderWorker(void* self,void* request){Execute(self,request,loaderWorker,true);}
inline bool Install(){
    char enabled[8]{};if(GetEnvironmentVariableA("CONTROLFG_LOADER_TRACE",enabled,sizeof(enabled))!=1||enabled[0]!='1')return false;
    auto runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll");
    if(!runtime||!HashMatches(runtime,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")){Log("LOADER_R235 disabled=runtime_hash");return false;}
    auto* base=reinterpret_cast<unsigned char*>(runtime);
    construct=reinterpret_cast<Callback>(base+0xfa320);worker=reinterpret_cast<Callback>(base+0xfb440);loaderWorker=reinterpret_cast<Callback>(base+0xfbae0);
    request=reinterpret_cast<Request>(base+0xff080);
    const unsigned sites[]={0xff15c,0xff280,0xfcae3,0xfcaea,0xfa30f};
    void* originals[]={reinterpret_cast<void*>(construct),reinterpret_cast<void*>(construct),reinterpret_cast<void*>(worker),reinterpret_cast<void*>(loaderWorker),reinterpret_cast<void*>(request)};
    void* hooks[]={reinterpret_cast<void*>(&Construct),reinterpret_cast<void*>(&Construct),reinterpret_cast<void*>(&Worker),reinterpret_cast<void*>(&LoaderWorker),reinterpret_cast<void*>(&RequestHook)};
    for(unsigned i=0;i<5;++i)if(!NativeCpuPrepareCallPatch(base+sites[i],originals[i],hooks[i],&patches[i])){Log("LOADER_R235 disabled=prepare index=%u",i);return false;}
    unsigned applied=0;
    for(;applied<5;){const auto i=applied;if(!NativeCpuExchangeCall(&patches[i],true))break;++applied;if(!patches[i].writeHealthy)break;}
    if(applied!=5||!patches[4].writeHealthy){while(applied){--applied;if(!NativeCpuExchangeCall(&patches[applied],false))Log("LOADER_R235 rollback_failed=%u",applied);}return false;}
    installed=true;Log("LOADER_R235 installed=1 scope=partial_requests request_id=0x178 retained_native_pointers=0 cache_required=0");return true;
}
inline void Poll(unsigned long long present){
    static ULONGLONG next=0;if(!installed||present%60)return;const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);const double scale=frequency.QuadPart?1000.0/frequency.QuadPart:0;
    Log("LOADER_R235_QUEUE present=%llu queued=%llu matched=%llu unmatched=%llu dropped=%llu replaced=%llu matched_bytes=%llu queue_ms=%.3f queue_max_ms=%.3f worker_count=%llu worker_ms=%.3f worker_max_ms=%.3f loader_count=%llu loader_ms=%.3f loader_max_ms=%.3f",
        present,tracker.queued.load(),tracker.matched.load(),tracker.unmatched.load(),tracker.dropped.load(),tracker.replaced.load(),tracker.bytes.load(),tracker.waitTicks.load()*scale,tracker.waitMax.load()*scale,
        workerCount.load(),workerTicks.load()*scale,workerMax.load()*scale,inlineCount.load(),inlineTicks.load()*scale,inlineMax.load()*scale);
}
}
