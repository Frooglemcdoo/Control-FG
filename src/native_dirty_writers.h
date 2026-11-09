#pragma once
#include <cstdint>
#include <cstring>
// Startup-only probes of exact native dirty stores. No C++ calls on the hit path.
namespace native_dirty {
struct Site {
    unsigned rva;const char* name;unsigned size;unsigned char bytes[8];
    unsigned char* address=nullptr;unsigned char replacement[8]{};
    unsigned char* code=nullptr;volatile LONG64* count=nullptr;
    RUNTIME_FUNCTION unwind{};bool registered=false,changed=false,healthy=false;
};
inline Site sites[]={
    {0xc481c,"manager_init",8,{0x41,0xc6,0x86,0x18,1,0,0,1}},
    {0xc4da5,"entry_insert",7,{0xc6,0x86,0x18,1,0,0,1}},
    {0xc4f8f,"entry_remove",7,{0xc6,0x86,0x18,1,0,0,1}},
    {0xc7b96,"material_hash_changed",8,{0x41,0xc6,0x87,0x18,1,0,0,1}},
    {0xc7bb0,"texture_callback",7,{0xc6,0x81,0x18,1,0,0,1}},
    {0xc99fe,"texture_unbind",7,{0xc6,0x80,0x18,1,0,0,1}},
    {0xc9a7d,"texture_bind",7,{0xc6,0x80,0x18,1,0,0,1}},
    {0xc9a99,"texture_listener_invalidate",7,{0xc6,0x80,0x18,1,0,0,1}}
};
inline bool installed=false;
inline bool Prepare(Site& s,unsigned char* address) noexcept {
    if(!address||(s.size!=7&&s.size!=8)||memcmp(address,s.bytes,s.size))return false;
    s.address=address;s.code=static_cast<unsigned char*>(AllocateExecutableRelayNear(address));
    if(!s.code)return false;
    s.count=static_cast<volatile LONG64*>(AllocateExecutableRelayNear(s.code));
    if(!s.count||!Rel32Fits(address+5,s.code))return false;
    // pushfq; original dirty store; lock inc qword [RIP+counter]; popfq; ret.
    // Counter data lives on a separate RW page. Stub and unwind metadata become RX.
    unsigned n=0;s.code[n++]=0x9c;memcpy(s.code+n,s.bytes,s.size);n+=s.size;
    s.code[n++]=0xf0;s.code[n++]=0x48;s.code[n++]=0xff;s.code[n++]=0x05;
    const auto delta=reinterpret_cast<std::intptr_t>(s.count)-reinterpret_cast<std::intptr_t>(s.code+n+4);
    if(delta<INT32_MIN||delta>INT32_MAX)return false;
    const auto displacement=static_cast<std::int32_t>(delta);memcpy(s.code+n,&displacement,4);n+=4;
    s.code[n++]=0x9d;s.code[n++]=0xc3;
    // pushfq allocates eight stack bytes; no register is saved for exception unwind.
    const unsigned char info[]={1,1,1,0,1,2,0,0};memcpy(s.code+0x100,info,sizeof(info));
    s.unwind.BeginAddress=0;s.unwind.EndAddress=n;s.unwind.UnwindData=0x100;
    DWORD old=0;
    if(!VirtualProtect(s.code,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),s.code,n))return false;
    if(!RtlAddFunctionTable(&s.unwind,1,reinterpret_cast<DWORD64>(s.code)))return false;
    s.registered=true;
    memset(s.replacement,0x90,s.size);s.replacement[0]=0xe8;
    const auto rel=static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(s.code)-reinterpret_cast<std::intptr_t>(address+5));
    memcpy(s.replacement+1,&rel,4);return true;
}
inline bool Exchange(Site& s,bool install) noexcept {
    const auto* expected=install?s.bytes:s.replacement;
    if(memcmp(s.address,expected,s.size))return false;
    DWORD old=0;if(!VirtualProtect(s.address,s.size,PAGE_EXECUTE_READWRITE,&old))return false;
    memcpy(s.address,install?s.replacement:s.bytes,s.size);s.changed=install;
    const bool flush=FlushInstructionCache(GetCurrentProcess(),s.address,s.size)!=FALSE;
    DWORD ignored=0;const bool protect=VirtualProtect(s.address,s.size,old,&ignored)!=FALSE;
    s.healthy=flush&&protect;return true;
}
inline bool Install(HMODULE renderer) noexcept {
    char value[8]{};GetEnvironmentVariableA("CONTROLFG_CPU_DIRTY_WRITERS",value,sizeof(value));
    if(value[0]=='0'){Log("NATIVE_CPU_R223 enabled=0 reason=environment");return false;}
    if(!renderer||renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    for(auto& s:sites)if(memcmp(base+s.rva,s.bytes,s.size))return false;
    for(auto& s:sites)if(!Prepare(s,base+s.rva))return false;
    for(auto& s:sites){
        if(!Exchange(s,true)||!s.healthy){
            for(auto& undo:sites)if(undo.changed&&(!Exchange(undo,false)||!undo.healthy))Log("NATIVE_CPU_R223 rollback_failed=%s",undo.name);
            return false;
        }
    }
    installed=true;
    Log("NATIVE_CPU_R223 enabled=1 sites=8 original_stores=preserved flags=preserved registers=preserved unwind=registered counters=global");
    return true;
}
inline void Poll(unsigned long long present) noexcept {
    if(!installed)return;
    for(auto& s:sites)Log("NATIVE_CPU_R223_WRITER present=%llu site=0x%x reason=%s hits=%llu",present,s.rva,s.name,
        static_cast<unsigned long long>(InterlockedCompareExchange64(s.count,0,0)));
}
// Stubs, counter pages and function tables intentionally live until process exit,
// including failed preparation/rollback, so no reachable code or unwind data is freed.
}
