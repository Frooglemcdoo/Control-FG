#pragma once
#include <cstdint>
#include <cstring>
#include <climits>
namespace control_rr {
struct IndirectCall6 {unsigned char bytes[6]{};};
inline bool DecodeIndirectCall(const IndirectCall6& instruction,std::uintptr_t site,std::uintptr_t& slot) noexcept {
 slot=0;if(instruction.bytes[0]!=0xff||instruction.bytes[1]!=0x15||site>UINTPTR_MAX-6)return false;
 std::int32_t offset=0;std::memcpy(&offset,instruction.bytes+2,4);
 const auto next=site+6;
 if(offset>=0){if(next>UINTPTR_MAX-static_cast<unsigned>(offset))return false;slot=next+offset;}
 else {const auto magnitude=static_cast<std::uint64_t>(-static_cast<std::int64_t>(offset));if(next<magnitude)return false;slot=next-static_cast<std::uintptr_t>(magnitude);}
 return slot!=0;
}
inline bool ReplaceIndirectCall(const IndirectCall6& original,std::uintptr_t site,std::uintptr_t expectedSlot,
 std::uintptr_t relay,IndirectCall6& replacement) noexcept {
 replacement={};std::uintptr_t slot=0;
 if(!DecodeIndirectCall(original,site,slot)||slot!=expectedSlot||!relay||site>UINTPTR_MAX-5)return false;
 const auto next=site+5;
 std::int64_t delta=0;
 if(relay>=next){if(relay-next>INT32_MAX)return false;delta=static_cast<std::int64_t>(relay-next);}
 else {if(next-relay>std::uint64_t(INT32_MAX)+1)return false;delta=-static_cast<std::int64_t>(next-relay);}
 // The extra NOP preserves the original continuation address and stack shape.
 replacement.bytes[0]=0xe8;const auto relative=static_cast<std::int32_t>(delta);
 std::memcpy(replacement.bytes+1,&relative,4);replacement.bytes[5]=0x90;return true;
}
}
