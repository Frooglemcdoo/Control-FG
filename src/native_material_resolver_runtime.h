#pragma once
#include "native_material_resolver.h"
#include "native_material_loop_bytes.h"
namespace native_material_resolver {
inline const void* descriptorImport=nullptr;
inline bool installed=false;
inline unsigned char* site=nullptr;
inline unsigned char* relay=nullptr;
inline unsigned char replacement[15]{};
inline RUNTIME_FUNCTION function{};
__declspec(noinline) inline bool Hook(void* manager,std::uint64_t entry,std::uint64_t outputBase) noexcept {
    return Resolve(manager,entry,outputBase,descriptorImport);
}
inline bool Prepare(unsigned char* target) noexcept {
    if(memcmp(target,kNativeMaterialLoop,sizeof(kNativeMaterialLoop)))return false;
    site=target;relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(target));
    if(!relay||!Rel32Fits(site+5,relay))return false;
    // Native RSP is 16-byte aligned before the injected CALL. Supply ABI shadow space.
    unsigned char code[]={
        0x48,0x83,0xec,0x28,             // sub rsp,40
        0x4c,0x89,0xe1,                  // rcx=r12 manager
        0x4c,0x89,0xea,                  // rdx=r13 entry
        0x4c,0x8b,0x85,0x60,1,0,0,      // r8=[rbp+160] output base
        0x48,0xb8,0,0,0,0,0,0,0,0,      // rax=Hook
        0xff,0xd0,                       // call rax
        0x48,0x8b,0xbd,0x60,1,0,0,      // rdi=output base
        0x84,0xc0,0x48,0x0f,0x45,0xf7,  // if nonnull slots: rsi=rdi
        0x45,0x33,0xed,                  // r13=0
        0xbb,0x23,0,0,0,                 // rbx=35
        0x48,0x83,0xc4,0x28,0xc3         // add rsp,40; ret
    };
    auto* hook=reinterpret_cast<void*>(&Hook);memcpy(code+19,&hook,8);memcpy(relay,code,sizeof(code));
    const unsigned char unwind[]={1,4,1,0,4,0x42,0,0};memcpy(relay+0x100,unwind,sizeof(unwind));
    function.BeginAddress=0;function.EndAddress=sizeof(code);function.UnwindData=0x100;
    DWORD old=0;if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay,sizeof(code)))return false;
    if(!RtlAddFunctionTable(&function,1,reinterpret_cast<DWORD64>(relay)))return false;
    memset(replacement,0x90,sizeof(replacement));replacement[0]=0xe8;
    const auto call=static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(relay)-reinterpret_cast<std::intptr_t>(site+5));memcpy(replacement+1,&call,4);
    replacement[5]=0xe9;const std::int32_t jump=sizeof(kNativeMaterialLoop)-10;memcpy(replacement+6,&jump,4);
    return true;
}
inline bool Exchange(bool enable) noexcept {
    const auto* expected=enable?kNativeMaterialLoop:replacement;
    if(memcmp(site,expected,sizeof(replacement)))return false;
    DWORD old=0;if(!VirtualProtect(site,sizeof(replacement),PAGE_EXECUTE_READWRITE,&old))return false;
    memcpy(site,enable?replacement:kNativeMaterialLoop,sizeof(replacement));installed=enable;
    const bool flush=FlushInstructionCache(GetCurrentProcess(),site,sizeof(replacement))!=FALSE;
    DWORD ignored=0;const bool protect=VirtualProtect(site,sizeof(replacement),old,&ignored)!=FALSE;
    return flush&&protect;
}
inline bool Install(HMODULE renderer,HMODULE d3d) noexcept {
    char value[8]{};const DWORD n=GetEnvironmentVariableA("CONTROLFG_CPU_MATERIAL_RESOLVER",value,sizeof(value));
    if(n!=1||value[0]!='1'){Log("NATIVE_CPU_R226 installed=0 native_loop=1 reason=not_opted_in");return false;}
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);auto* db=reinterpret_cast<unsigned char*>(d3d);
    struct Check {unsigned rva;unsigned size;unsigned char bytes[16];};
    const Check checks[]={
        {0xea80,5,{0x48,0x8b,0x41,8,0xc3}},
        {0x1a9a0,14,{0x48,0x8b,0x81,0x80,0,0,0,0x48,0x89,2,0x48,0x8b,0xc2,0xc3}},
        {0x1a9c0,13,{0x4c,0x89,0x84,0xd1,0xf8,1,0,0,0x48,0xff,0x41,8,0xc3}},
        {0x416a0,8,{0x4c,0x89,0x41,8,0x85,0xd2,0x74,0x42}}
    };
    for(unsigned i=0;i<4;++i)if(memcmp(db+checks[i].rva,checks[i].bytes,checks[i].size))return false;
    constexpr unsigned char type0[]={0x4d,0x85,0xc0,0x74,0x0e,0x49,0x8b,0x80,0x90,0,0,0,0x48,0x89,1,0x48,0x8b,0xc1,0xc3};
    if(memcmp(db+0x416ea,type0,sizeof(type0)))return false;
    const unsigned iat[]={0x5dfd30,0x5dfc80,0x5df640,0x5df630};const unsigned funcs[]={0xea80,0x416a0,0x1a9a0,0x1a9c0};
    for(unsigned i=0;i<4;++i)if(Read<void*>(base+iat[i])!=db+funcs[i])return false;
    auto* global=GetProcAddress(d3d,"?g_pGlobalDescriptorManager@d3d@@3PEAVGlobalDescriptorManager@1@EA");
    if(!global||Read<void*>(base+0x5df638)!=reinterpret_cast<void*>(global))return false;
    descriptorImport=base+0x5df638;
    if(!Prepare(base+0xc80fc))return false;
    if(!Exchange(true)){
        if(installed&&!Exchange(false))Log("NATIVE_CPU_R226 rollback_failed=resolver");return false;
    }
    Log("NATIVE_CPU_R226 installed=1 slots=35 cache=none native_order=preserved version_increments=35");return true;
}
// Relay/function metadata remain allocated for process lifetime, including rollback failures.
}
