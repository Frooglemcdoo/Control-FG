#pragma once
#include <cstdint>
#include <cstring>
namespace native_texture_upgrade {
// Exact Steam selector branch, aligned eight-byte exchange at RVA 15AA90.
// CMP EAX,[priority minimum]; JNE skip -> TEST EAX,EAX; NOP*4; JNS skip.
// Only negative deltas (higher detail) proceed. Surrounding direction, pending
// bytes, level-state and timed request gates remain native.
inline constexpr unsigned char Original[8]={0x3b,0x05,0x72,0x81,0x6a,0x00,0x75,0x2f};
inline constexpr unsigned char Replacement[8]={0x85,0xc0,0x90,0x90,0x90,0x90,0x79,0x2f};
struct Patch {
 unsigned char* site=nullptr;bool changed=false,healthy=false;
 bool Prepare(unsigned char* p){if(!p||(reinterpret_cast<std::uintptr_t>(p)&7)||memcmp(p,Original,8))return false;site=p;return true;}
 bool Exchange(bool install){
  if(!site)return false;
  long long expected=0,desired=0;memcpy(&expected,install?Original:Replacement,8);memcpy(&desired,install?Replacement:Original,8);
  DWORD old=0;if(!VirtualProtect(site,8,PAGE_EXECUTE_READWRITE,&old))return false;
  const auto found=InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(site),desired,expected);
  const bool exchanged=found==expected;if(exchanged)changed=install;
  const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,8)!=FALSE;
  DWORD ignored=0;healthy=VirtualProtect(site,8,old,&ignored)!=FALSE&&flushed;
  return exchanged;
 }
};
}
