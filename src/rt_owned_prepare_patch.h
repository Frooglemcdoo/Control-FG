#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace rt_owned_prepare {
inline constexpr unsigned char kLoopEntry[]={0x48,0x8b,0x71,0x10,0x8b,0x41,0x18,0x0f,0xb6,0x15,0x19,0x60,0x86,0x00};
inline bool BuildLoopPatch(std::uintptr_t site,std::uintptr_t relay,std::uintptr_t hook,unsigned char (&code)[27],unsigned char (&patch)[14]){
 const auto delta=static_cast<std::intptr_t>(relay)-static_cast<std::intptr_t>(site+5);
 if(delta<(std::numeric_limits<std::int32_t>::min)()||delta>(std::numeric_limits<std::int32_t>::max)())return false;
 const unsigned char stub[]={0x48,0x83,0xec,0x28,0x4c,0x89,0xf9,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xd0,0x49,0x8b,0x0f,0x48,0x83,0xc4,0x28,0xc3};
 std::memcpy(code,stub,sizeof(stub));std::memcpy(code+9,&hook,8);
 std::memset(patch,0x90,sizeof(patch));patch[0]=0xe8;patch[5]=0xe9;const auto relative=static_cast<std::int32_t>(delta);std::memcpy(patch+1,&relative,4);
 const std::int32_t skip=0xa8800-(0xa8459+10);std::memcpy(patch+6,&skip,4);return true;
}
}
