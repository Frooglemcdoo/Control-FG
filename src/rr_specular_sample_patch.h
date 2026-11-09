#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <limits>

namespace control_rr {
// Private render-thread state. UI never writes this storage. The relay records
// the value actually read by the producer, not merely the requested option.
struct SpecularSampleObservation {
    std::uint32_t requested=0, nativeCount=0, effectiveCount=0, seen=0;
};
struct SpecularSampleCode {
    std::array<unsigned char,7> replacement{};
    std::vector<unsigned char> relay;
};
// Replace only the audited mov r9d,[rip+disp32]. R9 flows to BOTH the hit-array
// allocation and the sample-count provider via the untouched [rbp+100h] local.
// No calls, SIMD changes, option writes, or shader patches.
// RAX and flags are saved/restored; all other registers are untouched.
inline bool BuildSpecularSampleCode(const unsigned char* original,std::uintptr_t site,
    std::uintptr_t nativeOption,std::uintptr_t relayAddress,std::uintptr_t state,
    SpecularSampleCode& out) {
    if(!original||original[0]!=0x44||original[1]!=0x8b||original[2]!=0x0d)return false;
    std::int32_t displacement=0;std::memcpy(&displacement,original+3,4);
    if(static_cast<std::uintptr_t>(static_cast<std::intptr_t>(site+7)+displacement)!=nativeOption)return false;
    auto putRelative=[](unsigned char* target,std::uintptr_t after,std::uintptr_t destination){
        const auto delta=static_cast<std::int64_t>(destination)-static_cast<std::int64_t>(after);
        if(delta<(std::numeric_limits<std::int32_t>::min)()||delta>(std::numeric_limits<std::int32_t>::max)())return false;
        const auto value=static_cast<std::int32_t>(delta);std::memcpy(target,&value,4);return true;
    };
    out.replacement={0xe9,0,0,0,0,0x90,0x90};out.relay.clear();
    if(!putRelative(out.replacement.data()+1,site+5,relayAddress))return false;
    // Native option/state may be outside RIP-relative range even though the
    // relay is close enough to the patched instruction. Use absolute addresses
    // through saved RAX; only the return jump needs a signed rel32 displacement.
    auto absolute=[&](std::uintptr_t address){
        out.relay.insert(out.relay.end(),{0x48,0xb8}); // mov rax,imm64
        const auto offset=out.relay.size();out.relay.resize(offset+8);
        const auto value=static_cast<std::uint64_t>(address);
        std::memcpy(out.relay.data()+offset,&value,8);
    };
    out.relay.push_back(0x50);                     // push rax
    absolute(nativeOption);
    out.relay.insert(out.relay.end(),{0x44,0x8b,0x08}); // mov r9d,[rax]
    absolute(state);
    out.relay.insert(out.relay.end(),{0x44,0x89,0x48,4}); // record native count
    out.relay.push_back(0x9c);                     // pushfq
    std::vector<std::size_t> skipBranches;
    auto skip=[&](unsigned char opcode){out.relay.push_back(opcode);skipBranches.push_back(out.relay.size());out.relay.push_back(0);};
    out.relay.insert(out.relay.end(),{0x41,0x83,0xf9,1});skip(0x72);
    out.relay.insert(out.relay.end(),{0x41,0x83,0xf9,16});skip(0x77);
    out.relay.insert(out.relay.end(),{0x83,0x38,1});skip(0x72); // request <1
    out.relay.insert(out.relay.end(),{0x83,0x38,16});skip(0x77);// request >16
    out.relay.insert(out.relay.end(),{0x44,0x8b,0x08}); // mov r9d,[request]
    for(auto offset:skipBranches)out.relay[offset]=static_cast<unsigned char>(out.relay.size()-offset-1);
    out.relay.push_back(0x9d);                     // popfq
    out.relay.insert(out.relay.end(),{0x44,0x89,0x48,8}); // effective count
    out.relay.insert(out.relay.end(),{0xc7,0x40,12,1,0,0,0}); // seen=1
    out.relay.push_back(0x58);                     // pop rax
    out.relay.push_back(0xe9);const auto back=out.relay.size();out.relay.resize(back+4);
    return putRelative(out.relay.data()+back,relayAddress+out.relay.size(),site+7);
}
}
