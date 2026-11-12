#pragma once
#include "gi_native_render_scope.h"
// Exact std::function<void(OrderSolverPassData*)> prepare invoker. Its body
// starts before the native graphics fence signal and material/scene jobs.
// Hooks only identify this boundary. Host exclusion is NOT installed here.
namespace gi_native_prepare_scope {
using U64=gi_native_render_scope::U64;
using Native=gi_native_render_scope::Native;
using Scope=gi_native_render_scope::Scope;
inline Native original=nullptr;
inline unsigned char* d3d=nullptr;
inline std::atomic<void(*)(const Scope&)noexcept> entering{nullptr};
inline std::atomic<void(*)(const Scope&,bool)noexcept> leaving{nullptr};
inline bool Read(void* closure,void* argument,Scope* out)noexcept {
 if(!closure||!argument||!out||!d3d)return false;
 __try {
  // Body r14=closure+8, owner loaded from [r14+20h]. The thunk leaves
  // the original second argument intact (unlike the render thunk).
  out->owner=*reinterpret_cast<void**>(static_cast<unsigned char*>(closure)+0x28);
  out->pass=*static_cast<void**>(argument);
  out->frame=*reinterpret_cast<U64*>(d3d+0x136c68);
  return out->owner&&out->pass&&out->frame;
 }__except(EXCEPTION_EXECUTE_HANDLER){*out={};return false;}
}
inline void Invoke(void* closure,void* argument){
 Scope scope{};if(Read(closure,argument,&scope))scope.token=gi_native_render_scope::Next();
 // Notify even if metadata is unavailable. A future global scene gate must
 // never permit a writer merely because its frame/owner pointer was unreadable.
 if(const auto callback=entering.load(std::memory_order_acquire))callback(scope);
 bool returned=false;
 __try {original(closure,argument);returned=true;}
 __finally {if(const auto callback=leaving.load(std::memory_order_acquire))callback(scope,returned);}
}
inline bool Install(HMODULE renderer,HMODULE module)noexcept {
 if(!renderer||!module||original||renderer!=verifiedRenderer||module!=verifiedD3d||
    !HashMatches(renderer,kRendererHash)||!HashMatches(module,kD3dHash))return false;
 auto* r=reinterpret_cast<unsigned char*>(renderer);
 const unsigned char thunk[]={0x48,0x83,0xc1,0x08,0xe9,0xf7,0x5e,0xfe,0xff};
 if(std::memcmp(r+0x13de60,thunk,sizeof(thunk)))return false;
 auto** slot=reinterpret_cast<void**>(r+0x632270);auto* expected=r+0x13de60;
 if(*slot!=expected)return false;
 DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 original=reinterpret_cast<Native>(expected);d3d=reinterpret_cast<unsigned char*>(module);
 const auto before=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&Invoke),expected);
 DWORD ignored=0;const bool protectedAgain=VirtualProtect(slot,sizeof(void*),old,&ignored)!=FALSE;
 if(before!=expected){original=nullptr;d3d=nullptr;return false;}
 if(protectedAgain)return true;
 InterlockedCompareExchangePointer(slot,expected,reinterpret_cast<void*>(&Invoke));
 VirtualProtect(slot,sizeof(void*),old,&ignored);return false;
}
}
