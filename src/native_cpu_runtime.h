#pragma once
#include "native_cpu_metrics.h"
#include "native_rt_append_runtime.h"
#include "native_cpu_call_patch.h"
#include "rr_indirect_call_windows.h"
// Included after the existing startup-only call/slot patch helpers.
namespace native_cpu {
inline bool installed=false;
inline LARGE_INTEGER clockFrequency{};
inline ULONGLONG nextReport=0;
using Callback=void(*)(void*);
using Wait=void(*)(void*,void*);
inline Callback material=nullptr,preparation=nullptr,recording=nullptr;
inline Wait wait=nullptr;
inline NativeCpuCallPatch materialPatch{};
inline RRIndirectCallPatch joinPatch{};
struct Local {unsigned slot=kThreads;bool registered=false;U64 ordinal[kStages]{};};
inline thread_local Local local;
struct Scope {
    Cell* cell=nullptr;LARGE_INTEGER start{};ULONG64 firstCycles=0;bool cyclesValid=false;
    explicit Scope(Stage stage) noexcept {
        const DWORD error=GetLastError();
        if(!local.registered){local.slot=Reserve(GetCurrentThreadId());local.registered=true;}
        if(local.slot<kThreads){
            auto& c=threads[local.slot].cells[stage];const U64 ordinal=++local.ordinal[stage];
            c.calls.store(ordinal,std::memory_order_relaxed);
            if(Sample(ordinal)){
                cyclesValid=QueryThreadCycleTime(GetCurrentThread(),&firstCycles)!=FALSE;
                if(QueryPerformanceCounter(&start))cell=&c;
            }
        }
        SetLastError(error);
    }
    ~Scope() noexcept {
        if(!cell)return;
        const DWORD error=GetLastError();LARGE_INTEGER end{};ULONG64 lastCycles=0;
        const bool timed=QueryPerformanceCounter(&end)!=FALSE;
        const bool valid=QueryThreadCycleTime(GetCurrentThread(),&lastCycles)!=FALSE&&cyclesValid&&lastCycles>=firstCycles;
        if(timed&&end.QuadPart>=start.QuadPart)cell->Add(static_cast<U64>(end.QuadPart-start.QuadPart),valid?lastCycles-firstCycles:0,valid);
        SetLastError(error);
    }
};
inline void MaterialHook(void* self){Scope scope(Material);material(self);}
inline void PreparationHook(void* self){
    const auto before=native_rt_append::counts;
    {Scope scope(Preparation);preparation(self);}
    native_rt_append::Publish(before);
}
inline void RecordingHook(void* self){Scope scope(Recording);recording(self);}
inline void JoinHook(void* self,void* handle){Scope scope(Join);wait(self,handle);}
inline bool Install(HMODULE renderer) noexcept {
    char value[8]{};GetEnvironmentVariableA("CONTROLFG_NATIVE_CPU_TIMING",value,sizeof(value));
    if(value[0]=='0'){Log("NATIVE_CPU_R218 enabled=0 reason=environment");return false;}
    if(!renderer||renderer!=verifiedRenderer||!QueryPerformanceFrequency(&clockFrequency)||clockFrequency.QuadPart<=0)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    // Exact renderer image hash is verified by Configure before reaching here.
    material=reinterpret_cast<Callback>(base+0xC7BC0);
    preparation=reinterpret_cast<Callback>(base+0xA8F80);
    recording=reinterpret_cast<Callback>(base+0xA8F70);
    constexpr unsigned char materialBytes[]={0xe8,0x9e,0x32,0xfa,0xff};
    constexpr unsigned char joinBytes[]={0xff,0x15,0x15,0xc4,0x4b,0x00};
    if(memcmp(base+0x12491D,materialBytes,sizeof(materialBytes))||memcmp(base+0x12404D,joinBytes,sizeof(joinBytes)))return false;
    auto* slot=FindImport(renderer,"rl_rmdwin10_f.dll","?waitJob@JobSystem@experimental@r@@QEAAXAEAVJobHandle@3@@Z");
    auto runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll");
    wait=runtime?reinterpret_cast<Wait>(GetProcAddress(runtime,"?waitJob@JobSystem@experimental@r@@QEAAXAEAVJobHandle@3@@Z")):nullptr;
    Patch slots[]={
        {reinterpret_cast<void**>(base+0x6281F0),reinterpret_cast<void*>(preparation),reinterpret_cast<void*>(&PreparationHook),"CPU_RT_PREPARE"},
        {reinterpret_cast<void**>(base+0x6281D8),reinterpret_cast<void*>(recording),reinterpret_cast<void*>(&RecordingHook),"CPU_RT_RECORD"}
    };
    for(const auto& p:slots)if(*p.slot!=p.original)return false;
    if(!NativeCpuPrepareCallPatch(base+0x12491D,reinterpret_cast<void*>(material),reinterpret_cast<void*>(&MaterialHook),&materialPatch)||
       !joinPatch.Prepare(base+0x12404D,slot,reinterpret_cast<void*>(wait),reinterpret_cast<void*>(&JoinHook)))return false;
    bool materialChanged=NativeCpuExchangeCall(&materialPatch,true);
    bool healthy=materialChanged&&materialPatch.writeHealthy;
    if(healthy)healthy=joinPatch.Exchange(true)&&joinPatch.healthy;
    unsigned applied=0;
    if(healthy)for(;applied<2;++applied)if(!Exchange(slots[applied],true))break;
    if(!healthy||applied!=2){
        while(applied){--applied;if(!Exchange(slots[applied],false))Log("NATIVE_CPU_R218 rollback_failed=%s",slots[applied].label);}
        if(joinPatch.changed&&!joinPatch.Exchange(false))Log("NATIVE_CPU_R218 rollback_failed=join");
        if(materialChanged&&!NativeCpuExchangeCall(&materialPatch,false))Log("NATIVE_CPU_R218 rollback_failed=material");
        return false;
    }
    installed=true;nextReport=GetTickCount64()+5000;
    Log("NATIVE_CPU_R218 enabled=1 sample_interval=%u max_threads=%u qpc_frequency=%lld cumulative=1 join_includes_helping=1 graphics=R217",kInterval,kThreads,clockFrequency.QuadPart);
    return true;
}
inline void Poll(U64 present) noexcept {
    if(!installed||present%60)return;
    const auto now=GetTickCount64();if(now<nextReport)return;nextReport=now+5000;
    Log("NATIVE_RT_APPEND_R219_STATUS present=%llu installed=%u enabled=%u completed_job_fast_appends=%llu completed_job_native_appends=%llu",
        present,unsigned(native_rt_append::installed),unsigned(native_rt_append::enabled),native_rt_append::completedFast.load(),native_rt_append::completedNative.load());
    // Called on Present, never under a native material lock or from a worker.
    // Cumulative snapshots remain usable if a report races an in-flight sample.
    for(unsigned stage=0;stage<kStages;++stage){
        Snapshot total{};U64 calls=0;unsigned observed=0,skipped=0;char distribution[1100]{};size_t used=0;
        for(unsigned t=0;t<kThreads;++t){
            const auto tid=threads[t].tid.load(std::memory_order_acquire);if(!tid)continue;
            auto& c=threads[t].cells[stage];const auto n=c.calls.load(std::memory_order_relaxed);if(!n)continue;
            Snapshot s{};if(!c.Read(s)){++skipped;continue;}++observed;calls+=n;
            total.samples+=s.samples;total.ticks+=s.ticks;total.cycles+=s.cycles;total.cycleSamples+=s.cycleSamples;
            if(s.maximum>total.maximum)total.maximum=s.maximum;
            if(used<sizeof(distribution)-1){
                const int wrote=_snprintf_s(distribution+used,sizeof(distribution)-used,_TRUNCATE,"%s%lu:%llu",used?",":"",tid,n);
                if(wrote>0)used+=static_cast<size_t>(wrote);else used=sizeof(distribution)-1;
            }
        }
        const double ms=1000.0/static_cast<double>(clockFrequency.QuadPart);
        Log("NATIVE_CPU_R218_STAGE present=%llu stage=%s calls=%llu samples=%llu sampled_ms=%.3f mean_ms=%.4f sampled_max_ms=%.3f cycles=%llu cycle_samples=%llu threads=%u skipped_cells=%u overflow_threads=%llu tid_calls=%s",
            present,Name(stage),calls,total.samples,total.ticks*ms,total.samples?total.ticks*ms/total.samples:0.0,total.maximum*ms,total.cycles,total.cycleSamples,observed,skipped,overflowThreads.load(),distribution);
    }
}
}
