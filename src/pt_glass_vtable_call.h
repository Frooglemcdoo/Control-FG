#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace PTGlass {
// Replace exactly FF 90 <slot*8> with E8 <relay> 90. The trailing NOP
// preserves the native continuation at site+6, including its unwind region.
inline bool EncodeVtableCall6(const unsigned char* original,std::uintptr_t site,
                             unsigned slot,std::uintptr_t relay,unsigned char* output) noexcept {
    std::uint32_t displacement=0;
    std::memcpy(&displacement,original+2,4);
    if(original[0]!=0xff||original[1]!=0x90||displacement!=slot*8u)return false;
    const auto delta=static_cast<std::int64_t>(relay)-static_cast<std::int64_t>(site+5);
    if(delta<std::numeric_limits<std::int32_t>::min()||delta>std::numeric_limits<std::int32_t>::max())return false;
    const auto rel=static_cast<std::int32_t>(delta);output[0]=0xe8;
    std::memcpy(output+1,&rel,4);output[5]=0x90;return true;
}
}
