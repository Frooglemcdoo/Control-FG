#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <climits>
namespace control_storage {
// Exact PackFile::readFile tail dispatch, not a general function detour.
struct PackTailPatch {
    unsigned char* site=nullptr;
    unsigned char original[7]{0x48,0xff,0xa0,0x90,0,0,0},replacement[7]{};
    bool changed=false,healthy=false;
    bool Prepare(unsigned char* address,void* hook,void* relay) noexcept {
        if(!address||!hook||!relay||memcmp(address,original,7))return false;
        const auto delta=reinterpret_cast<std::intptr_t>(relay)-reinterpret_cast<std::intptr_t>(address+5);
        if(delta<INT32_MIN||delta>INT32_MAX)return false;
        site=address;replacement[0]=0xe9;const auto relative=static_cast<std::int32_t>(delta);
        memcpy(replacement+1,&relative,4);replacement[5]=replacement[6]=0x90;
        unsigned char stub[14]{0xff,0x25,0,0,0,0};memcpy(stub+6,&hook,8);memcpy(relay,stub,14);
        DWORD old=0;return VirtualProtect(relay,14,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),relay,14);
    }
    bool Exchange(bool install) noexcept {
        if(!site||memcmp(site,install?original:replacement,7))return false;
        DWORD old=0;if(!VirtualProtect(site,7,PAGE_EXECUTE_READWRITE,&old))return false;
        memcpy(site,install?replacement:original,7);changed=install;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,7)!=FALSE;
        DWORD ignored=0;healthy=VirtualProtect(site,7,old,&ignored)&&flushed;return healthy;
    }
};
}
