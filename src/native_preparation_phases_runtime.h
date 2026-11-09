#pragma once
#include "native_preparation_phases.h"
#include "native_cpu_metrics.h"
#include "rr_indirect_call_windows.h"
namespace native_prep {
inline bool installed=false;
inline native_cpu::Cell cells[native_cpu::kThreads][kPhases];
inline std::atomic<Tick> rejected{0},withReadback{0};
struct Frame;
inline thread_local Frame* current=nullptr;
inline Tick Now() noexcept {
    const DWORD saved=GetLastError();LARGE_INTEGER now{};
    const bool ok=QueryPerformanceCounter(&now)!=FALSE;SetLastError(saved);
    return ok?static_cast<Tick>(now.QuadPart):0;
}
struct Frame {
    Frame* previous=current;Timeline time{};unsigned slot;bool active;
    Frame(bool sampled,unsigned threadSlot) noexcept:slot(threadSlot),active(installed&&sampled&&threadSlot<native_cpu::kThreads){
        current=this;if(active)time.begin=Now();
    }
    ~Frame() noexcept {current=previous;}
    void Finish() noexcept {
        if(!active)return;
        time.end=Now();Tick durations[kPhases]{};
        if(!time.begin||!time.Split(durations)){rejected.fetch_add(1,std::memory_order_relaxed);return;}
        for(unsigned i=0;i<kPhases;++i)cells[slot][i].Add(durations[i],0,false);
        if(time.readback)withReadback.fetch_add(1,std::memory_order_relaxed);
    }
};
using Map=void*(*)(void*);
using Unmap=void(*)(void*);
// Verified native return record: 24 bytes, Windows x64 hidden return pointer.
struct AccelerationInfo {std::uint64_t bytes[3];};
using Estimate=AccelerationInfo(*)(int,int);
inline Map mapOriginal=nullptr;inline Unmap unmapOriginal=nullptr;inline Estimate estimateOriginal=nullptr;
inline void* MapHook(void* buffer){
    auto* f=current;if(f&&f->active){f->time.readback=true;f->time.map=Now();}
    return mapOriginal(buffer);
}
inline void UnmapHook(void* buffer){
    unmapOriginal(buffer);
    auto* f=current;if(f&&f->active){f->time.unmapped=true;f->time.unmap=Now();}
}
inline AccelerationInfo EstimateHook(int flags,int count){
    auto* f=current;if(f&&f->active){f->time.estimated=true;f->time.estimateBegin=Now();}
    auto result=estimateOriginal(flags,count);
    if(f&&f->active){f->time.estimateComplete=true;f->time.estimateEnd=Now();}
    return result;
}
inline RRIndirectCallPatch patches[3];
inline bool Install(HMODULE renderer,HMODULE d3d) noexcept {
    char setting[8]{};
    if(GetEnvironmentVariableA("CONTROLFG_CPU_PREPARATION_PHASES",setting,sizeof(setting))!=1||setting[0]!='1')return false;
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d)return false;
    const char* names[]={
        "?mapBufferForReadback@NativeBufferUtilDX12@d3d@@SAPEAXPEAVNativeBuffer@2@@Z",
        "?unmapBufferForReadback@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z",
        "?estimateTopLevelAccelerationStructureSize@DeviceUtilRaytracing@d3d@@SA?AURaytracingAccelerationInfo@2@W4RaytracingAccelerationBuildFlags@2@H@Z"};
    constexpr std::size_t rvas[]={0xa840b,0xa8448,0xa8810};
    constexpr unsigned char expected[3][6]={{0xff,0x15,0x5f,0x75,0x53,0},{0xff,0x15,0x2a,0x75,0x53,0},{0xff,0x15,0xea,0x6c,0x53,0}};
    void* hooks[]={reinterpret_cast<void*>(&MapHook),reinterpret_cast<void*>(&UnmapHook),reinterpret_cast<void*>(&EstimateHook)};
    void* originals[3]{};auto* base=reinterpret_cast<unsigned char*>(renderer);
    for(unsigned i=0;i<3;++i){
        originals[i]=reinterpret_cast<void*>(GetProcAddress(d3d,names[i]));
        if(memcmp(base+rvas[i],expected[i],6)||!patches[i].Prepare(base+rvas[i],FindImport(renderer,"d3d_rmdwin10_f.dll",names[i]),originals[i],hooks[i]))return false;
    }
    mapOriginal=reinterpret_cast<Map>(originals[0]);unmapOriginal=reinterpret_cast<Unmap>(originals[1]);estimateOriginal=reinterpret_cast<Estimate>(originals[2]);
    for(unsigned i=0;i<3;++i){
        if(!patches[i].Exchange(true)||!patches[i].healthy){
            for(unsigned j=0;j<3;++j)if(patches[j].changed&&!patches[j].Exchange(false))Log("NATIVE_CPU_R227 rollback_failed=%u",j);
            return false;
        }
    }
    installed=true;Log("NATIVE_CPU_R227 phases=1 sample_interval=64 cache=none native_calls=forwarded");return true;
}
inline void Poll(Tick present,double frequency) noexcept {
    if(!installed||frequency<=0)return;
    for(unsigned p=0;p<kPhases;++p){
        Tick samples=0,ticks=0,maximum=0;unsigned skipped=0;
        for(unsigned t=0;t<native_cpu::kThreads;++t){
            native_cpu::Snapshot s{};if(!cells[t][p].Read(s)){++skipped;continue;}
            samples+=s.samples;ticks+=s.ticks;if(s.maximum>maximum)maximum=s.maximum;
        }
        Log("NATIVE_CPU_R227_PHASE present=%llu phase=%s samples=%llu sampled_ms=%.6f maximum_ms=%.6f skipped_cells=%u readback_jobs=%llu rejected=%llu",
            present,Name(p),samples,double(ticks)*1000.0/frequency,double(maximum)*1000.0/frequency,skipped,withReadback.load(),rejected.load());
    }
}
}
