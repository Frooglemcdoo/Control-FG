#pragma once
// Startup-only replacement of the two verified native scan-partition stores.
// The leaf relay executes MOV [RIP+disp32], imm32; RET: no register/flag changes,
// no API calls, allocations, locks or logging while native settings are applied.
namespace native_streaming_policy {
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
        relay=AllocateExecutableRelayNear(site);if(!relay)return false;
        auto* code=static_cast<unsigned char*>(relay);
        if(!Rel32Fits(site+5,relay)||!Rel32Fits(code+10,destination))return false;
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
inline bool Install(HMODULE renderer) noexcept {
    char enabled[8]{},slices[8]{};
    if(GetEnvironmentVariableA("CONTROLFG_STREAMING_PIN_SLICES",enabled,sizeof(enabled))!=1||enabled[0]!='1')return false;
    const DWORD n=GetEnvironmentVariableA("CONTROLFG_STREAMING_SLICES",slices,sizeof(slices));
    const int value=n==1&&slices[0]=='8'?8:n==2&&slices[0]=='1'&&slices[1]=='6'?16:0;
    if(!value||!renderer||renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    constexpr unsigned char first[]={0x44,0x89,0x25,0x67,0x18,0x7e,0};
    constexpr unsigned char second[]={0xc7,0x05,0xe8,0x17,0x7e,0,8,0,0,0};
    if(memcmp(base+0x1377a2,first,sizeof(first))||memcmp(base+0x13781e,second,sizeof(second)))return false;
    auto* destination=reinterpret_cast<int*>(base+0x919010);
    if(!writers[0].Prepare(base+0x1377a2,destination,value)||!writers[1].Prepare(base+0x13781e,destination,value))return false;
    for(auto& writer:writers){
        if(!writer.Exchange(true)||!writer.healthy){
            for(auto& rollback:writers)if(rollback.changed&&!rollback.Exchange(false))Log("STREAMING_R228 rollback_failed=1");
            return false;
        }
    }
    Log("STREAMING_R228 pin_slices=%d writers=2 phase_hooks=independent scope=process_only",value);
    return true;
}
}
