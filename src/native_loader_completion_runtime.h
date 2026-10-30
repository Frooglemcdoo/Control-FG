#pragma once
// Audited call-site observers only; native completion limits and callbacks stay native.
namespace native_loader_completion {
using U64=loader_trace::U64;
using Complete=unsigned(*)(void*,unsigned);
using Release=void(*)(void*,void*);
inline Complete complete=nullptr;
inline Release release=nullptr;
inline native_loader_stages::Metric runtimeTime,rendererTime,releaseTime;
inline std::atomic<U64> waitSamples{0},waitingCompletions{0},waitingHeap{0},completionMax{0},heapMax{0},nested{0};
inline thread_local unsigned depth=0;
inline NativeCpuCallPatch calls[7]{};
inline RRIndirectCallPatch imports[4]{};
inline bool installed=false;
inline unsigned CompleteCall(void* self,unsigned limit,native_loader_stages::Metric& metric){
    const DWORD incoming=GetLastError();const auto begin=native_loader_stages::Tick();
    const bool outer=depth++==0;if(!outer)++nested;
    SetLastError(incoming);const unsigned remaining=complete(self,limit);const DWORD error=GetLastError();
    --depth;if(outer)metric.Add(native_loader_stages::Tick()-begin);
    SetLastError(error);return remaining;
}
inline unsigned Runtime(void* self,unsigned limit){return CompleteCall(self,limit,runtimeTime);}
inline unsigned Renderer(void* self,unsigned limit){return CompleteCall(self,limit,rendererTime);}
inline void ReleaseHook(void* self,void* loaded){
    const DWORD incoming=GetLastError();const auto begin=native_loader_stages::Tick();
    SetLastError(incoming);release(self,loaded);const DWORD error=GetLastError();
    releaseTime.Add(native_loader_stages::Tick()-begin);SetLastError(error);
}
inline void ObserveWait(U64 owner){
    if(!owner)return;
    // Aligned atomic snapshots; other workers may change these immediately.
    auto* self=reinterpret_cast<unsigned char*>(owner);
    const auto completions=static_cast<unsigned>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(self+0x818),0,0));
    const auto heap=static_cast<unsigned>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(self+0x54c),0,0));
    ++waitSamples;waitingCompletions.fetch_add(completions,std::memory_order_relaxed);waitingHeap.fetch_add(heap,std::memory_order_relaxed);
    loader_trace::Tracker::Max(completionMax,completions);loader_trace::Tracker::Max(heapMax,heap);
}
inline bool Install(){
    auto runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll"),renderer=GetModuleHandleW(L"renderer_rmdwin10_f.dll");
    if(!runtime||!renderer||!HashMatches(runtime,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")||
       !HashMatches(renderer,"EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433")){
        Log("LOADER_R238_COMPLETION disabled=module_hash");return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(runtime);auto* renderBase=reinterpret_cast<unsigned char*>(renderer);
    complete=reinterpret_cast<Complete>(base+0xfcfb0);release=reinterpret_cast<Release>(base+0xf9fe0);
    const unsigned sites[]={0xfcf82,0x130e63,0x131abe,0x1346fd,0x13509e,0x15c1cb,0xfd1ba};
    for(unsigned i=0;i<7;++i)if(!NativeCpuPrepareCallPatch(base+sites[i],i==6?reinterpret_cast<void*>(release):reinterpret_cast<void*>(complete),
        i==6?reinterpret_cast<void*>(&ReleaseHook):reinterpret_cast<void*>(&Runtime),&calls[i])){Log("LOADER_R238_COMPLETION disabled=prepare index=%u",i);return false;}
    const unsigned renderSites[]={0x10c90b,0x10ca06,0x10d38a,0x168771};
    for(unsigned i=0;i<4;++i)if(!imports[i].Prepare(renderBase+renderSites[i],reinterpret_cast<void**>(renderBase+0x5e07c0),reinterpret_cast<void*>(complete),reinterpret_cast<void*>(&Renderer))){Log("LOADER_R238_COMPLETION disabled=import_prepare index=%u",i);return false;}
    unsigned applied=0,imported=0;
    for(;applied<7;){const auto i=applied;if(!NativeCpuExchangeCall(&calls[i],true))break;++applied;if(!calls[i].writeHealthy)break;}
    if(applied==7&&calls[6].writeHealthy)for(;imported<4;){const auto i=imported;if(!imports[i].Exchange(true))break;++imported;if(!imports[i].healthy)break;}
    if(applied!=7||!calls[6].writeHealthy||imported!=4||!imports[3].healthy){
        while(imported){--imported;if(!imports[imported].Exchange(false))Log("LOADER_R238_COMPLETION import_rollback_failed=%u",imported);}
        while(applied){--applied;if(!NativeCpuExchangeCall(&calls[applied],false))Log("LOADER_R238_COMPLETION rollback_failed=%u",applied);}return false;
    }
    native_loader_stages::observeWait=&ObserveWait;installed=true;
    Log("LOADER_R238_COMPLETION installed=1 runtime_sites=6 renderer_sites=4 release_sites=1 outermost_only=1");return true;
}
inline void Poll(U64 present,double scale){
    if(!installed)return;
    const char* names[]={"complete_runtime","complete_renderer","release_buffers"};
    native_loader_stages::Metric* metrics[]={&runtimeTime,&rendererTime,&releaseTime};
    for(unsigned i=0;i<3;++i){auto& m=*metrics[i];Log("LOADER_R238_STAGE present=%llu stage=%s count=%llu total_ms=%.3f max_ms=%.3f",present,names[i],m.count.load(),m.ticks.load()*scale,m.maximum.load()*scale);}
    Log("LOADER_R238_WAIT present=%llu samples=%llu completions_sum=%llu completions_max=%llu heap_sum=%llu heap_max=%llu nested_complete=%llu",present,waitSamples.load(),waitingCompletions.load(),completionMax.load(),waitingHeap.load(),heapMax.load(),nested.load());
}
}
