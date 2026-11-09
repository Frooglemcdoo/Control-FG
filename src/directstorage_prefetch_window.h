#pragma once
#include <cstdint>
namespace control_storage {
constexpr unsigned PrefetchGranularity=64*1024;
struct PrefetchWindow {std::uint64_t offset=0;unsigned size=0;};
inline PrefetchWindow RequestWindow(std::uint64_t offset,unsigned count,std::uint64_t fileSize,unsigned maximum) noexcept {
    if(!count||count>maximum||offset>fileSize||count>fileSize-offset)return {};
    auto start=offset-offset%PrefetchGranularity;
    const auto span=offset-start+count;
    auto size=(span+PrefetchGranularity-1)/PrefetchGranularity*PrefetchGranularity;
    // A maximum-size unaligned demand still fits one allocation exactly.
    if(size>maximum){start=offset;size=count;}
    if(size>fileSize-start)size=fileSize-start;
    return {start,static_cast<unsigned>(size)};
}
}
