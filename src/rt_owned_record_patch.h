#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace rt_owned_record {
inline bool TailPatch(std::uintptr_t site,std::uintptr_t relay,std::uintptr_t hook,unsigned char (&jump)[5],unsigned char (&stub)[14])noexcept{
 const auto delta=static_cast<std::intptr_t>(relay)-static_cast<std::intptr_t>(site+5);
 if(delta<(std::numeric_limits<std::int32_t>::min)()||delta>(std::numeric_limits<std::int32_t>::max)())return false;
 jump[0]=0xe9;const auto relative=static_cast<std::int32_t>(delta);std::memcpy(jump+1,&relative,4);
 const unsigned char head[]={0xff,0x25,0,0,0,0};std::memcpy(stub,head,6);std::memcpy(stub+6,&hook,8);return true;
}
}
