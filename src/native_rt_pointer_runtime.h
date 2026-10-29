#pragma once
#include "native_rt_append.h"
#include "native_cpu_call_patch.h"
#include <atomic>
namespace native_rt_pointer {
using Append=void*(*)(native_rt_append::Array*,const void*);
inline Append original=nullptr;
inline bool installed=false,enabled=false;
struct Counts {unsigned long long fast=0,native=0;};
inline thread_local Counts counts;
inline std::atomic<unsigned long long> completedFast{0},completedNative{0};
inline void Publish(Counts before) noexcept {
    completedFast.fetch_add(counts.fast-before.fast,std::memory_order_relaxed);
    completedNative.fetch_add(counts.native-before.native,std::memory_order_relaxed);
}
inline void* Hook(native_rt_append::Array* array,const void* source) {
    if(enabled){if(auto* out=native_rt_append::AppendPointerIfRoom(array,source)){++counts.fast;return out;}}
    ++counts.native;return original(array,source);
}
inline NativeCpuCallPatch patch{};
inline bool Install(HMODULE renderer) noexcept {
    if(!renderer||renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    original=reinterpret_cast<Append>(base+0x22730);
    constexpr unsigned char entry[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x20,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x20};
    constexpr unsigned char room[]={0x8b,0x71,0x0c,0x48,0x8b,0xf9,0x8b,0x49,0x08};
    constexpr unsigned char copy[]={0x8d,0x41,0x01,0x89,0x47,0x08,0x48,0x8b,0x02,0x49,0x89,0x07};
    if(memcmp(base+0x22730,entry,sizeof(entry))||memcmp(base+0x22746,room,sizeof(room))||memcmp(base+0x22837,copy,sizeof(copy)))return false;
    if(!NativeCpuPrepareCallPatch(base+0xA8DED,reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Hook),&patch))return false;
    const bool changed=NativeCpuExchangeCall(&patch,true);
    if(!changed||!patch.writeHealthy){
        if(changed&&!NativeCpuExchangeCall(&patch,false))Log("NATIVE_RT_POINTER_R220 rollback_failed=1");
        // A surviving wrapper still delegates because enabled remains false.
        return false;
    }
    char value[8]{};const DWORD length=GetEnvironmentVariableA("CONTROLFG_RT_POINTER_FAST",value,sizeof(value));
    enabled=!(length>0&&length<sizeof(value)&&value[0]=='0');installed=true;
    Log("NATIVE_RT_POINTER_R220 installed=1 enabled=%u record_bytes=8 sites=1 allocation=native references=unchanged",unsigned(enabled));
    return true;
}
}
