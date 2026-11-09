#pragma once
#include "native_material_diff.h"
#include "native_cpu_workload.h"
#include "native_cpu_call_patch.h"
namespace native_material_diff {
using Update=void(*)(void*,void*,void*,void*);
inline Update original=nullptr;
inline NativeCpuCallPatch patch{};
inline bool installed=false;
inline Queue queue;
inline std::atomic<unsigned long long> changed{0},invalid{0},reported{0};
inline thread_local unsigned long long ordinal=0;
inline void Hook(void* manager,void* handle,void* source,void* textures){
    using native_workload::Load;
    if(!manager||!handle||!source){original(manager,handle,source,textures);return;}
    const auto id=Load<std::uint32_t>(handle,0);
    const auto count=Load<std::uint32_t>(manager,0x80);
    const auto* hashes=Load<const unsigned char*>(manager,0x98);
    const auto* records=Load<const unsigned char*>(manager,0x88);
    const auto* keys=Load<const unsigned char*>(manager,0x78);
    const auto* incoming=Load<const unsigned char*>(source,8);
    if(id>=count||!hashes||!records||!keys||!incoming){invalid.fetch_add(1,std::memory_order_relaxed);original(manager,handle,source,textures);return;}
    const auto oldHash=Load<std::uint32_t>(hashes+static_cast<size_t>(id)*4,0);
    const auto oldKey=Load<std::uint64_t>(keys+static_cast<size_t>(id)*8,0);
    const auto derived=Load<std::uint32_t>(incoming,0x148);
    std::uint32_t before[Words],after[Words];
    memcpy(before,records+static_cast<size_t>(id)*0x150,sizeof(before));
    original(manager,handle,source,textures);
    // This native routine updates existing storage; it does not resize manager arrays.
    const auto newHash=Load<std::uint32_t>(hashes+static_cast<size_t>(id)*4,0);
    if(newHash==oldHash)return;
    changed.fetch_add(1,std::memory_order_relaxed);
    if((ordinal++%8)!=0)return;
    memcpy(after,records+static_cast<size_t>(id)*0x150,sizeof(after));
    const auto newKey=Load<std::uint64_t>(keys+static_cast<size_t>(id)*8,0);
    queue.Push(Compare(reinterpret_cast<std::uintptr_t>(manager),id,oldHash,newHash,oldKey,newKey,derived,before,after));
}
inline bool Install(HMODULE renderer) noexcept {
    char value[8]{};GetEnvironmentVariableA("CONTROLFG_CPU_MATERIAL_DIFF",value,sizeof(value));
    if(value[0]=='0'){Log("NATIVE_CPU_R224 enabled=0 reason=environment");return false;}
    if(!renderer||renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    constexpr unsigned char call[]={0xe8,0x94,0,0,0};
    constexpr unsigned char entry[]={0x48,0x85,0xd2,0x0f,0x84,0x84,0x01,0,0};
    if(memcmp(base+0xc7987,call,sizeof(call))||memcmp(base+0xc7a20,entry,sizeof(entry)))return false;
    original=reinterpret_cast<Update>(base+0xc7a20);
    if(!NativeCpuPrepareCallPatch(base+0xc7987,reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Hook),&patch))return false;
    const bool applied=NativeCpuExchangeCall(&patch,true);
    if(!applied||!patch.writeHealthy){if(applied&&!NativeCpuExchangeCall(&patch,false))Log("NATIVE_CPU_R224 rollback_failed=update");return false;}
    installed=true;Log("NATIVE_CPU_R224 enabled=1 changed_sample_interval=8 record_bytes=336 logging=present original_update=preserved");return true;
}
inline void Poll(unsigned long long present) noexcept {
    if(!installed)return;
    Event e;unsigned drained=0;
    while(drained<Capacity&&queue.Pop(e)){
        ++drained;char diff[256]{};size_t used=0;
        for(unsigned i=0;i<e.changes&&i<Examples;++i){
            const int n=_snprintf_s(diff+used,sizeof(diff)-used,_TRUNCATE,"%s%x:%08x>%08x",i?",":"",e.offset[i],e.before[i],e.after[i]);
            if(n<=0)break;used+=static_cast<size_t>(n);
        }
        Log("NATIVE_CPU_R224_DIFF present=%llu manager=0x%llx entry=%u old_hash=%08x new_hash=%08x old_key=%llx new_key=%llx source_derived=%08x committed_derived=%08x changed_words=%u mask0=%llx mask1=%llx words=%s",
            present,static_cast<unsigned long long>(e.manager),e.entry,e.oldHash,e.newHash,
            static_cast<unsigned long long>(e.oldKey),static_cast<unsigned long long>(e.newKey),e.sourceDerived,e.committedDerived,e.changes,
            static_cast<unsigned long long>(e.mask[0]),static_cast<unsigned long long>(e.mask[1]),diff);
    }
    reported.fetch_add(drained,std::memory_order_relaxed);
    Log("NATIVE_CPU_R224_STATUS present=%llu changed=%llu reported=%llu dropped=%llu invalid=%llu",present,changed.load(),reported.load(),queue.dropped.load(),invalid.load());
}
}
