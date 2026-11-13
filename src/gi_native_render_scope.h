#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
// Exact renderer ebfb5b47... std::function<void(OrderSolverPassData*)> callback.
// Covers native128D20..12DA38, including join, GI ray and GI6. This identifies
// the native scope; it does NOT itself exclude scene writers or own GPU memory.
namespace gi_native_render_scope {
using U64=std::uint64_t;
using Native=void(*)(void*,void*);
struct Scope {U64 token=0,frame=0;void* owner=nullptr;void* pass=nullptr;};
inline thread_local Scope current{};
inline std::atomic<U64> serial{0};
inline Native original=nullptr;
inline unsigned char* d3d=nullptr;
inline std::atomic<void(*)(const Scope&,bool)noexcept> leaving{nullptr};
inline bool Read(void* closure,void* argument,Scope* out)noexcept {
 if(!closure||!argument||!out||!d3d)return false;
 __try {
  out->owner=*reinterpret_cast<void**>(static_cast<unsigned char*>(closure)+8);
  out->pass=*static_cast<void**>(argument);
  out->frame=*reinterpret_cast<U64*>(d3d+0x136c68);
  return out->owner&&out->pass&&out->frame;
 }__except(EXCEPTION_EXECUTE_HANDLER){*out={};return false;}
}
inline U64 Next()noexcept {
 auto n=serial.load(std::memory_order_relaxed);
 while(n!=UINT64_MAX){if(serial.compare_exchange_weak(n,n+1,std::memory_order_relaxed))return n+1;}
 return 0;
}
inline void Invoke(void* closure,void* argument){
 const Scope previous=current;Scope next{};
 if(Read(closure,argument,&next))next.token=Next();
 current=next;
 bool returned=false;
 __try {original(closure,argument);returned=true;}
 __finally {
  if(next.token)if(const auto callback=leaving.load(std::memory_order_acquire))callback(next,returned);
  current=previous;
 }
}
// Atomically replaces only the verified vtable slot; leaves the native thunk
// and function prologue intact. Call during startup, before render workers.
inline bool Install(HMODULE renderer,HMODULE module)noexcept {
 if(!renderer||!module||original||renderer!=verifiedRenderer||module!=verifiedD3d||
    !HashMatches(renderer,kRendererHash)||!HashMatches(module,kD3dHash))return false;
 auto* r=reinterpret_cast<unsigned char*>(renderer);
 const unsigned char thunk[]={0x48,0x8b,0x12,0x48,0x83,0xc1,0x08,0xe9,0xb4,0xa4,0xfe,0xff};
 if(std::memcmp(r+0x13e860,thunk,sizeof(thunk)))return false;
 auto** slot=reinterpret_cast<void**>(r+0x631da0);auto* expected=r+0x13e860;
 if(*slot!=expected)return false;
 DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 original=reinterpret_cast<Native>(expected);d3d=reinterpret_cast<unsigned char*>(module);
 const auto before=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&Invoke),expected);
 DWORD ignored=0;const bool protectedAgain=VirtualProtect(slot,sizeof(void*),old,&ignored)!=FALSE;
 if(before!=expected){original=nullptr;d3d=nullptr;return false;}
 if(protectedAgain)return true;
 // Failed protection restoration is not a successful install. Restore the
 // slot while writable, keeping the forwarding target available if raced.
 InterlockedCompareExchangePointer(slot,expected,reinterpret_cast<void*>(&Invoke));
 VirtualProtect(slot,sizeof(void*),old,&ignored);return false;
}
}
