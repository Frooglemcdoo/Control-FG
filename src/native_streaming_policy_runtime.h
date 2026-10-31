#pragma once
#include "storage_startup_policy.h"
// Startup-only replacement of the two verified native scan-partition stores.
// The leaf relay executes MOV [RIP+disp32], imm32; RET: no register/flag changes,
// no API calls, allocations, locks or logging while native settings are applied.
namespace native_streaming_policy {
// Both the CALL and the relay's RIP-relative store must be in range. The
// generic allocator only constrains the CALL and starts near its lower limit.
inline bool RelayRange(std::uintptr_t site,std::uintptr_t target,std::uintptr_t minimum,
    std::uintptr_t maximum,std::uintptr_t& low,std::uintptr_t& high) noexcept {
    constexpr std::uintptr_t span=0x7fff0000ull;
    low=minimum;high=maximum;
    for(auto center:{site,target}){
        const auto lo=center>span?center-span:minimum;
        const auto hi=center>maximum||maximum-center<span?maximum:center+span;
        if(lo>low)low=lo;if(hi<high)high=hi;
    }
    return low<high&&high-low>=4096;
}
inline void* AllocateWriterRelay(const void* site,const void* target) noexcept {
    SYSTEM_INFO si{};GetSystemInfo(&si);
    std::uintptr_t low=0,high=0;
    if(!RelayRange(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(target),
        reinterpret_cast<std::uintptr_t>(si.lpMinimumApplicationAddress),
        reinterpret_cast<std::uintptr_t>(si.lpMaximumApplicationAddress),low,high))return nullptr;
    const std::uintptr_t gran=si.dwAllocationGranularity?si.dwAllocationGranularity:0x10000u;
    for(auto cursor=low;cursor<high;){
        MEMORY_BASIC_INFORMATION mbi{};
        if(!VirtualQuery(reinterpret_cast<void*>(cursor),&mbi,sizeof(mbi)))break;
        const auto start=reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const auto end=start+mbi.RegionSize;if(end<=cursor)break;
        if(mbi.State==MEM_FREE){
            const auto first=start>low?start:low;
            const auto candidate=(first+gran-1)&~(gran-1);
            if(candidate>=first&&candidate<=high-4096&&candidate<=end&&end-candidate>=4096){
                if(void* memory=VirtualAlloc(reinterpret_cast<void*>(candidate),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return memory;
            }
        }
        cursor=end;
    }
    return nullptr;
}
struct Writer {
    unsigned char* site=nullptr;
    unsigned char original[10]{},replacement[10]{};
    unsigned size=0;
    void* relay=nullptr;
    bool changed=false,healthy=false;
    bool Prepare(unsigned char* address,int* destination,int partitions) noexcept {
        if(!address||!destination||(partitions!=8&&partitions!=16))return false;
        unsigned displacementOffset=0;
        if(address[0]==0x44&&address[1]==0x89&&address[2]==0x25){size=7;displacementOffset=3;}
        else if(address[0]==0xc7&&address[1]==0x05){
            int immediate=0;memcpy(&immediate,address+6,4);
            if(immediate!=8)return false;
            size=10;displacementOffset=2;
        }else return false;
        int displacement=0;memcpy(&displacement,address+displacementOffset,4);
        if(address+size+displacement!=reinterpret_cast<unsigned char*>(destination))return false;
        site=address;memcpy(original,site,size);
        relay=AllocateWriterRelay(site,destination);
        if(!relay){Log("STREAMING_R229 prepare_failed=allocation");return false;}
        auto* code=static_cast<unsigned char*>(relay);
        if(!Rel32Fits(site+5,relay)||!Rel32Fits(code+10,destination)){
            Log("STREAMING_R229 prepare_failed=relative_range");VirtualFree(relay,0,MEM_RELEASE);relay=nullptr;return false;
        }
        const int storeDelta=static_cast<int>(reinterpret_cast<std::intptr_t>(destination)-reinterpret_cast<std::intptr_t>(code+10));
        const int callDelta=static_cast<int>(reinterpret_cast<std::intptr_t>(relay)-reinterpret_cast<std::intptr_t>(site+5));
        code[0]=0xc7;code[1]=0x05;memcpy(code+2,&storeDelta,4);memcpy(code+6,&partitions,4);code[10]=0xc3;
        memset(replacement,0x90,size);replacement[0]=0xe8;memcpy(replacement+1,&callDelta,4);
        DWORD old=0;
        return VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),relay,11);
    }
    bool Exchange(bool install) noexcept {
        if(!site||!relay||memcmp(site,install?original:replacement,size))return false;
        DWORD old=0;if(!VirtualProtect(site,size,PAGE_EXECUTE_READWRITE,&old))return false;
        memcpy(site,install?replacement:original,size);changed=install;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,size)!=FALSE;
        DWORD ignored=0;healthy=VirtualProtect(site,size,old,&ignored)!=FALSE&&flushed;
        return true;
    }
};
inline Writer writers[2];
inline unsigned char* observedRenderer=nullptr;
inline bool installed=false;
inline void Report(unsigned long long present) noexcept {
    if(!observedRenderer)return;
    float budget=0;int slices=0;
    memcpy(&budget,observedRenderer+0x9186a0,4);memcpy(&slices,observedRenderer+0x919010,4);
    Log("STREAMING_R229_STATE present=%llu pin_installed=%u budget_ms=%.3f slices=%d",present,unsigned(installed),double(budget),slices);
}
inline bool Install(HMODULE renderer,bool defaultEnabled=false) noexcept {
    if(!renderer||renderer!=verifiedRenderer){Log("STREAMING_R229 disabled=unverified_renderer");return false;}
    observedRenderer=reinterpret_cast<unsigned char*>(renderer);
    char slices[8]{};
    if(!storage_startup_policy::EnvironmentFlag("CONTROLFG_STREAMING_PIN_SLICES",defaultEnabled)){Log("STREAMING_R229 disabled=environment");return false;}
    const DWORD n=GetEnvironmentVariableA("CONTROLFG_STREAMING_SLICES",slices,sizeof(slices));
    const int value=!n&&defaultEnabled?16:n==1&&slices[0]=='8'?8:n==2&&slices[0]=='1'&&slices[1]=='6'?16:0;
    if(!value){Log("STREAMING_R229 disabled=invalid_slices");return false;}
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    constexpr unsigned char first[]={0x44,0x89,0x25,0x67,0x18,0x7e,0};
    constexpr unsigned char second[]={0xc7,0x05,0xe8,0x17,0x7e,0,8,0,0,0};
    if(memcmp(base+0x1377a2,first,sizeof(first))||memcmp(base+0x13781e,second,sizeof(second))){Log("STREAMING_R229 disabled=writer_mismatch");return false;}
    auto* destination=reinterpret_cast<int*>(base+0x919010);
    if(!writers[0].Prepare(base+0x1377a2,destination,value)||!writers[1].Prepare(base+0x13781e,destination,value)){Log("STREAMING_R229 disabled=prepare_failed");return false;}
    for(auto& writer:writers){
        if(!writer.Exchange(true)||!writer.healthy){
            for(auto& rollback:writers)if(rollback.changed&&!rollback.Exchange(false))Log("STREAMING_R229 rollback_failed=1");
            Log("STREAMING_R229 disabled=exchange_failed");
            return false;
        }
    }
    installed=true;
    storage_startup_policy::pinnedSlices=value;
    Log("STREAMING_R229 pin_slices=%d writers=2 phase_hooks=independent scope=process_only",value);
    return true;
}
}
