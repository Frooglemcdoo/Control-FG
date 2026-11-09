#pragma once
#include "native_rt_append.h"
#include <atomic>
#include "native_cpu_call_patch.h"
namespace native_rt_append {
using Append=void*(*)(Array*,const void*);
inline Append original=nullptr;
inline bool installed=false,enabled=false;
struct Counts {unsigned long long fast=0,native=0;};
inline thread_local Counts counts;
inline std::atomic<unsigned long long> completedFast{0},completedNative{0};
inline void Publish(Counts before) noexcept {
    completedFast.fetch_add(counts.fast-before.fast,std::memory_order_relaxed);
    completedNative.fetch_add(counts.native-before.native,std::memory_order_relaxed);
}
inline void* Hook(Array* array,const void* source) {
    if(enabled){if(auto* out=AppendIfRoom(array,source)){++counts.fast;return out;}}
    ++counts.native;return original(array,source);
}
inline NativeCpuCallPatch patches[2]{};
inline bool Install(HMODULE renderer) noexcept {
    if(!renderer||renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    original=reinterpret_cast<Append>(base+0xA6D20);
    // Additional exact native routine guards beyond Configure's image hash.
    constexpr unsigned char entry[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x20,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x20};
    constexpr unsigned char room[]={0x8b,0x79,0x0c,0x48,0x8b,0xd9,0x8b,0x49,0x08};
    if(memcmp(base+0xA6D20,entry,sizeof(entry))||memcmp(base+0xA6D36,room,sizeof(room)))return false;
    constexpr std::size_t sites[]={0xA8D93,0xA8EFC};
    for(unsigned i=0;i<2;++i)if(!NativeCpuPrepareCallPatch(base+sites[i],reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Hook),&patches[i]))return false;
    unsigned applied=0;bool healthy=true;
    for(;applied<2;){
        if(!NativeCpuExchangeCall(&patches[applied],true)){healthy=false;break;}
        healthy=patches[applied].writeHealthy;++applied;if(!healthy)break;
    }
    if(applied!=2||!healthy){
        // enabled remains false: any unremovable hook forwards to the native
        // function, with its relay and original target retained for lifetime.
        while(applied){--applied;if(!NativeCpuExchangeCall(&patches[applied],false))Log("NATIVE_RT_APPEND_R219 rollback_failed=%u",applied);}
        return false;
    }
    char value[8]{};const DWORD length=GetEnvironmentVariableA("CONTROLFG_RT_APPEND_FAST",value,sizeof(value));
    enabled=!(length>0&&length<sizeof(value)&&value[0]=='0');installed=true;
    Log("NATIVE_RT_APPEND_R219 installed=1 enabled=%u record_bytes=64 sites=2 allocation=native ownership=native publication=native",unsigned(enabled));
    return true;
}
}
