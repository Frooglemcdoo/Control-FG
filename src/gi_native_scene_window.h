#pragma once
#include "gi_native_prepare_scope.h"
#include "gi_reuse/native_scene_window.h"
#include "native_cpu_call_patch.h"
// Startup-only exact-build adapter. A reader can be acquired only in the full
// GI render invocation after its actual native scene join. The host must still
// retain all GPU bindings and release only at real consumer submit/cancellation.
namespace gi_native_scene_window {
using U64=gi_native_render_scope::U64;
using Scope=gi_native_render_scope::Scope;
using JoinFunction=void(*)(void*);
inline gi_reuse::NativeSceneWindow window;
inline NativeCpuCallPatch patch{};
inline JoinFunction originalJoin=nullptr;
inline std::atomic<bool> ready{false};
struct Proof {U64 token=0,frame=0,epoch=0;void* scene=nullptr;};
inline thread_local Proof joined{};
inline bool Scene(const Scope& scope,void** out)noexcept {
 if(!scope.owner||!out)return false;*out=nullptr;
 __try {*out=*reinterpret_cast<void**>(static_cast<unsigned char*>(scope.owner)+0x220);return *out!=nullptr;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void EnterPreparation(const Scope&)noexcept{window.EnterPreparation();}
inline void LeavePreparation(const Scope&,bool)noexcept{window.LeavePreparation();}
inline void Join(void* scene){
 joined={};const auto scope=gi_native_render_scope::current;
 const auto epoch=ready.load(std::memory_order_acquire)?window.BeforeJoin():0;
 originalJoin(scene); // Includes native CPU job join and async->graphics wait.
 void* expected=nullptr;
 if(epoch&&scope.token&&scope.frame&&Scene(scope,&expected)&&expected==scene&&
    gi_native_render_scope::current.token==scope.token)
  joined={scope.token,scope.frame,epoch,scene};
}
inline bool TryAcquire()noexcept {
 if(!ready.load(std::memory_order_acquire))return false;
 const auto& scope=gi_native_render_scope::current;void* expected=nullptr;
 return scope.token&&joined.token==scope.token&&joined.frame==scope.frame&&
  Scene(scope,&expected)&&expected==joined.scene&&window.TryAcquire(scope.token,joined.epoch);
}
// Not called from render-scope exit unconditionally. An already recorded but
// unsubmitted consumer still references the scene; it must execute or be sealed
// by successful Reset before preparation may proceed.
inline bool ReleaseAfterConsumerOrCancellation(U64 token)noexcept{return window.Release(token);}
inline bool Install(HMODULE renderer,HMODULE d3d)noexcept {
 if(ready.load()||!renderer||!d3d||renderer!=verifiedRenderer||d3d!=verifiedD3d||
    !HashMatches(renderer,kRendererHash)||!HashMatches(d3d,kD3dHash))return false;
 auto* r=reinterpret_cast<unsigned char*>(renderer);
 if(gi_native_render_scope::original!=reinterpret_cast<gi_native_render_scope::Native>(r+0x13e860)||
    gi_native_prepare_scope::original!=reinterpret_cast<gi_native_prepare_scope::Native>(r+0x13de60)||
    gi_native_prepare_scope::entering.load()||gi_native_prepare_scope::leaving.load())return false;
 auto target=reinterpret_cast<JoinFunction>(r+0xa8bf0);
 if(!NativeCpuPrepareCallPatch(r+0x128f3b,reinterpret_cast<void*>(target),reinterpret_cast<void*>(&Join),&patch))return false;
 originalJoin=target;
 if(!NativeCpuExchangeCall(&patch,true))return false;
 if(!patch.writeHealthy){NativeCpuExchangeCall(&patch,false);return false;}
 gi_native_prepare_scope::entering.store(&EnterPreparation,std::memory_order_release);
 gi_native_prepare_scope::leaving.store(&LeavePreparation,std::memory_order_release);
 ready.store(true,std::memory_order_release);return true;
}
}
