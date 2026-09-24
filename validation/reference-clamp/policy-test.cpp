#include "../../diagnostic_addon/reference_clamp_contract.h"
#include <cassert>
#include <cstdio>
using namespace controlfg_clamp_ref_contract;
int main() {
    const std::uint32_t good = ReadyMask;
    assert(Ready(true,good,1,1,TargetShaderCRC,0x12345678u));
    for (unsigned bit=0; bit<5; ++bit) assert(!Ready(true,good & ~(1u<<bit),1,1,TargetShaderCRC,1));
    assert(!Ready(true,good | TargetInitializedUnreplaced,1,1,TargetShaderCRC,1));
    assert(!Ready(false,good,1,1,TargetShaderCRC,1));
    assert(!Ready(true,good,0,1,TargetShaderCRC,1));
    assert(!Ready(true,good,1,0,TargetShaderCRC,1));
    assert(!Ready(true,good,1,1,0xDEADBEEFu,1));
    assert(!Ready(true,good,1,1,TargetShaderCRC,0));
    assert(Ready(true,good | (1u<<12),9,4,TargetShaderCRC,0x89ABCDEFu));
    std::puts("PASS: r21w reference-clamp readiness is fail-closed for missing confirmation, wrong identity, zero counts and any unreplaced target PSO");
    return 0;
}
