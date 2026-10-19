#pragma once
#include <cstdint>
#include <cstddef>
namespace PTCharacterCapture {
inline constexpr std::size_t MaxRecordBytes=4u*1024u*1024u;
inline constexpr std::size_t MaxCaptureBytes=32u*1024u*1024u;
// NativeBufferUtil::unlock clamps end-start to the mapped length. Refuse
// contradictory ranges instead of silently truncating diagnostic evidence.
inline bool MappedBytes(std::uintptr_t start,std::uint64_t length,
                        std::uintptr_t end,std::size_t& bytes) noexcept {
    bytes=0;
    if(start<0x10000||!length||length>MaxRecordBytes||
       length>UINTPTR_MAX-start)return false;
    if(end&&(end<start||end>start+length))return false;
    const auto used=end?end-start:length;
    if(!used)return false;
    bytes=static_cast<std::size_t>(used);return true;
}
// DynamicBuffer rounds allocations up to 64 bytes. Rig records are 144 bytes,
// so padding is less than one record; never copy that unwritten padding.
inline std::size_t RigBytes(std::uint32_t allocated) noexcept {
    if(!allocated||allocated>MaxRecordBytes||allocated%64)return 0;
    const auto used=std::size_t(allocated/144)*144;
    return used&&((used+63)&~std::size_t(63))==allocated?used:0;
}
}
