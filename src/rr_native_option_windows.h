#pragma once
#include "rr_native_option_policy.h"

// Initialization-only instruction redirect for Control's exact
//   cmp byte ptr [rip+disp32], bl
// at renderer+0x11DE2E. The replacement byte lives in a private RW page near
// the instruction and is never the game's registered option storage.
struct RRNativeJitterOptionPatch {
    unsigned char* site=nullptr;
    control_rr::RipByteCompare6 original{},replacement{};
    volatile char* mirror=nullptr;
    bool changed=false,healthy=false;

    bool Prepare(unsigned char* address,unsigned char* expectedTarget) noexcept {
        if(!address||!expectedTarget)return false;
        site=address;std::memcpy(original.data(),address,original.size());
        void* storage=AllocateExecutableRelayNear(address); // returns RW; this page is intentionally never executable.
        if(!storage)return false;
        mirror=reinterpret_cast<volatile char*>(storage);*mirror=0;
        if(!control_rr::RedirectRipByteCompare(original,reinterpret_cast<std::uintptr_t>(address),
            reinterpret_cast<std::uintptr_t>(expectedTarget),reinterpret_cast<std::uintptr_t>(storage),replacement))return false;
        return true;
    }
    bool Set(bool enabled) noexcept {
        if(!mirror)return false;
        InterlockedExchange8(mirror,enabled?1:0);
        return *mirror==(enabled?1:0);
    }
    bool Exchange(bool install) noexcept {
        if(!site||!mirror)return false;
        const auto& expected=install?original:replacement;const auto& next=install?replacement:original;
        if(std::memcmp(site,expected.data(),expected.size()))return false;
        DWORD previous=0;if(!VirtualProtect(site,next.size(),PAGE_EXECUTE_READWRITE,&previous))return false;
        std::memcpy(site,next.data(),next.size());changed=install;
        const bool flush=FlushInstructionCache(GetCurrentProcess(),site,next.size())!=FALSE;
        DWORD ignored=0;const bool protect=VirtualProtect(site,next.size(),previous,&ignored)!=FALSE;
        healthy=flush&&protect;return true;
    }
};
