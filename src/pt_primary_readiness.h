#pragma once
#include <cstdint>
namespace PTPrimaryReadiness {
enum class Result {noSamples,healthy,unhealthy};
// Unsupported BRDFs retain native output per pixel. Their screen coverage must
// not demote the whole frame when aiming brings a character into the foreground.
// All other transport faults and failed output commits still count as failures.
inline Result Check(std::uint32_t integrated,std::uint32_t covered,std::uint32_t faults,std::uint32_t unsupported){
    if(covered>integrated||faults>integrated-covered||unsupported>faults)return Result::unhealthy;
    const auto supported=integrated-unsupported;
    if(!supported)return Result::noSamples;
    return covered>=supported-supported/20?Result::healthy:Result::unhealthy;
}
}
