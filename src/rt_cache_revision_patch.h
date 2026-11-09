#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace rt_cache_revision {
inline bool Jump5(std::uintptr_t source,std::uintptr_t target,unsigned char* out)noexcept{
 if(target>=source+5){if(target-(source+5)>0x7fffffffull)return false;}
 else if(source+5-target>0x80000000ull)return false;
 const auto displacement=static_cast<std::int32_t>(target-(source+5));out[0]=0xe9;std::memcpy(out+1,&displacement,4);return true;
}
inline void AbsoluteJump(unsigned char* out,std::uintptr_t target)noexcept{
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(out,jump,6);std::memcpy(out+6,&target,8);
}
// JMP r11 is not a Windows epilogue form. An indirect RIP-memory JMP after a
// copied stack prologue can be misclassified as an already-unwound epilogue.
inline void ResumeJump(unsigned char* out,std::uintptr_t target)noexcept{
 out[0]=0x49;out[1]=0xbb;std::memcpy(out+2,&target,8);out[10]=0x41;out[11]=0xff;out[12]=0xe3;
}
inline constexpr unsigned char ConstructorEntry[]={0x44,0x89,0x4c,0x24,0x20};
inline constexpr unsigned char DestructorEntry[]={0x40,0x57,0x48,0x83,0xec,0x40};
inline constexpr unsigned char MaterialEntry[]={0x40,0x55,0x56,0x57,0x48,0x83,0xec,0x30};
inline constexpr unsigned char ConstructorUnwind[]={1,0,0,0};
inline constexpr unsigned char DestructorUnwind[]={1,6,2,0,6,0x72,2,0x70};
inline constexpr unsigned char MaterialUnwind[]={1,8,4,0,8,0x52,4,0x70,3,0x60,2,0x50};
}
