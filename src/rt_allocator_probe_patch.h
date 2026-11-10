#pragma once
#include <cstdint>
#include <cstring>
namespace rt_allocator_probe {
inline bool Call6(std::uintptr_t site,std::uintptr_t relay,unsigned char (&output)[6]){
 if(site>UINTPTR_MAX-5)return false;
 if(relay>=site+5){if(relay-(site+5)>0x7fffffffull)return false;}else if(site+5-relay>0x80000000ull)return false;
 const auto delta=static_cast<std::int32_t>(relay-(site+5));output[0]=0xe8;std::memcpy(output+1,&delta,4);output[5]=0x90;return true;
}
}
