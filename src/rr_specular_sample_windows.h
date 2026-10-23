#pragma once
#include "rr_specular_sample_patch.h"

// Installed once during verified renderer initialization, never on UI clicks.
// Keep code RX and observations RW in separate pages, as with the jitter mirror.
struct RRSpecularSamplePatch {
    unsigned char* site=nullptr;
    std::array<unsigned char,7> original{},replacement{};
    control_rr::SpecularSampleObservation* observation=nullptr;
    void* relay=nullptr;
    bool changed=false,healthy=false;
    bool Prepare(unsigned char* address,unsigned char* nativeOption) {
        if(!address||!nativeOption)return false;
        site=address;std::memcpy(original.data(),site,original.size());
        relay=AllocateExecutableRelayNear(site);
        observation=static_cast<control_rr::SpecularSampleObservation*>(AllocateExecutableRelayNear(site));
        if(!relay||!observation)return false;
        *observation={};control_rr::SpecularSampleCode code;
        if(!control_rr::BuildSpecularSampleCode(original.data(),reinterpret_cast<std::uintptr_t>(site),
            reinterpret_cast<std::uintptr_t>(nativeOption),reinterpret_cast<std::uintptr_t>(relay),
            reinterpret_cast<std::uintptr_t>(observation),code))return false;
        replacement=code.replacement;std::memcpy(relay,code.relay.data(),code.relay.size());
        DWORD previous=0;
        return VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&previous)&&
            FlushInstructionCache(GetCurrentProcess(),relay,code.relay.size());
    }
    void BeginFrame(unsigned request) noexcept {
        if(!observation)return;
        observation->seen=0;
        observation->requested=request<=16?request:0;
        MemoryBarrier();
    }
    bool Exchange(bool install) noexcept {
        if(!site||!relay||!observation)return false;
        const auto& expected=install?original:replacement;
        const auto& next=install?replacement:original;
        if(std::memcmp(site,expected.data(),expected.size()))return false;
        DWORD previous=0;if(!VirtualProtect(site,next.size(),PAGE_EXECUTE_READWRITE,&previous))return false;
        std::memcpy(site,next.data(),next.size());changed=install;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,next.size())!=FALSE;
        DWORD ignored=0;const bool protectedAgain=VirtualProtect(site,next.size(),previous,&ignored)!=FALSE;
        healthy=flushed&&protectedAgain;return true;
    }
};
