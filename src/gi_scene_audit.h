#pragma once
#include "gi_phase_audit.h"
#include "native_cpu_call_patch.h"
#include <intrin.h>
#include <cstring>
// Observational, startup-only, build-locked hooks. No scene lease is granted.
namespace gi_scene_audit {
using U64=gi_phase_audit::U64;
using Native=void(*)(void*);
inline Native original[4]{};
inline NativeCpuCallPatch patches[4]{};
inline unsigned char *renderer=nullptr,*d3d=nullptr;
inline std::atomic<bool> installed{false};
inline std::atomic<U64> serial{0},active[3]{};
inline thread_local void* scene=nullptr;
inline thread_local U64 sceneFrame=0;
inline thread_local bool joined=false;
inline bool Requested() noexcept {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_SCENE_CHECK",v,4)==1&&v[0]=='1';}
inline bool Tracking() noexcept {return installed.load(std::memory_order_relaxed)&&!gi_phase_audit::stopped.load(std::memory_order_relaxed);}
struct Writer {
 unsigned kind;void* argument;bool tracking;
 Writer(unsigned k,void* p) noexcept:kind(k),argument(p),tracking(Tracking()) {
  if(!tracking)return;
  ++active[k];const auto s=++serial;
  if(gi_phase_audit::Window())gi_phase_audit::Emit(20,{gi_phase_audit::Frame(),k,1,s,reinterpret_cast<U64>(p),GetCurrentThreadId()});
 }
 ~Writer(){if(!tracking)return;
  const auto s=++serial;
  if(gi_phase_audit::Window())gi_phase_audit::Emit(20,{gi_phase_audit::Frame(),kind,0,s,reinterpret_cast<U64>(argument),GetCurrentThreadId()});
  --active[kind];
 }
 Writer(const Writer&)=delete;Writer& operator=(const Writer&)=delete;
};
inline void Material(void* p){Writer scope(0,p);original[0](p);}
inline void Prepare(void* p){Writer scope(1,p);original[1](p);}
inline void Record(void* p){Writer scope(2,p);original[2](p);}
inline void Join(void* p){
 if(!Tracking()){original[3](p);return;}
 scene=p;sceneFrame=gi_phase_audit::Frame();joined=false;
 if(gi_phase_audit::Window())gi_phase_audit::Emit(21,{sceneFrame,reinterpret_cast<U64>(p),0,GetCurrentThreadId()});
 original[3](p);joined=true;
 if(gi_phase_audit::Window())gi_phase_audit::Emit(21,{sceneFrame,reinterpret_cast<U64>(p),1,GetCurrentThreadId()});
}
struct Snapshot {
 U64 valid=0,fault=0,scenePtr=0,sceneFence=0,sceneValue=0,sceneCom=0,job0=0,job1=0;
 U64 deviceFence=0,deviceValue=0,deviceCom=0,managerGeneration=0;
 U64 state=0,heap[2]{},heapCom[2]{},epoch[2]{},generation[2]{},cursor[2]{};
};
// Trivial POD only inside SEH. Native pointer samples are not retained or owned.
inline void Read(Snapshot* s,U64 frame) noexcept {
 __try {
  if(scene&&sceneFrame==frame){s->scenePtr=reinterpret_cast<U64>(scene);
   auto* f=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(scene)+0xd0);
   if(f){s->sceneFence=reinterpret_cast<U64>(f);s->sceneValue=*reinterpret_cast<U64*>(f);s->sceneCom=*reinterpret_cast<U64*>(f+8);s->valid|=1;}}
  s->job0=*reinterpret_cast<U64*>(renderer+0x90e9e8);s->job1=*reinterpret_cast<U64*>(renderer+0x90e9f0);s->valid|=2;
  auto* device=*reinterpret_cast<unsigned char**>(d3d+0x136d28);
  if(device){auto* f=*reinterpret_cast<unsigned char**>(device+0x88);
   if(f){s->deviceFence=reinterpret_cast<U64>(f);s->deviceValue=*reinterpret_cast<U64*>(f);s->deviceCom=*reinterpret_cast<U64*>(f+8);s->valid|=4;}}
  auto* manager=*reinterpret_cast<unsigned char**>(d3d+0x111bb8);
  if(manager){s->managerGeneration=*reinterpret_cast<U64*>(manager+8);s->valid|=8;}
  auto** slots=reinterpret_cast<void**>(__readgsqword(0x58));
  auto* tls=slots?static_cast<unsigned char*>(slots[*reinterpret_cast<unsigned*>(d3d+0x1115fc)]):nullptr;
  auto* state=tls?*reinterpret_cast<unsigned char**>(tls+0x20):nullptr;
  if(!state)state=*reinterpret_cast<unsigned char**>(d3d+0x111c10);
  if(state){s->state=reinterpret_cast<U64>(state);
   for(unsigned i=0;i<2;++i){auto* h=*reinterpret_cast<unsigned char**>(state+0x50+i*8);
    if(!h)continue;s->heap[i]=reinterpret_cast<U64>(h);s->heapCom[i]=*reinterpret_cast<U64*>(h);
    s->epoch[i]=*reinterpret_cast<U64*>(h+0x20);s->generation[i]=*reinterpret_cast<U64*>(h+0x30);
    s->cursor[i]=*reinterpret_cast<U64*>(h+0x18);s->valid|=U64(16)<<i;}}
 } __except(EXCEPTION_EXECUTE_HANDLER){s->fault=GetExceptionCode();}
}
inline void Observe(U64 parent,U64 frame,unsigned point) noexcept {
 if(!Tracking()||!gi_phase_audit::Window())return;
 const auto before=serial.load();const U64 a=active[0].load(),b=active[1].load(),c=active[2].load();
 Snapshot s{};Read(&s,frame);const auto after=serial.load();
 const auto id=gi_phase_audit::Emit(22,{parent,frame,point,before,after,a,b,c,s.valid,s.fault,GetCurrentThreadId(),U64(joined&&sceneFrame==frame)});
 gi_phase_audit::Emit(23,{id,s.scenePtr,s.sceneFence,s.sceneValue,s.sceneCom,s.job0,s.job1,s.deviceFence,s.deviceValue,s.deviceCom,s.managerGeneration,s.state});
 for(unsigned i=0;i<2;++i)gi_phase_audit::Emit(24,{id,i,s.heap[i],s.heapCom[i],s.epoch[i],s.generation[i],s.cursor[i]});
}
inline bool Install(HMODULE r,HMODULE d) noexcept {
 if(!Requested())return false;
 if(!gi_phase_audit::Enabled()||r!=verifiedRenderer||d!=verifiedD3d||!r||!d||
    !HashMatches(r,kRendererHash)||!HashMatches(d,kD3dHash)){
  gi_phase_audit::Emit(25,{427,0,1});return false;}
 renderer=reinterpret_cast<unsigned char*>(r);d3d=reinterpret_cast<unsigned char*>(d);
 constexpr unsigned sites[]={0x12491d,0xa8f84,0xa8f74,0x128f3b};
 constexpr unsigned targets[]={0xc7bc0,0xa81b0,0xa89c0,0xa8bf0};
 void* hooks[]={reinterpret_cast<void*>(&Material),reinterpret_cast<void*>(&Prepare),reinterpret_cast<void*>(&Record),reinterpret_cast<void*>(&Join)};
 for(unsigned i=0;i<4;++i){original[i]=reinterpret_cast<Native>(renderer+targets[i]);
  if(!NativeCpuPrepareCallPatch(renderer+sites[i],renderer+targets[i],hooks[i],&patches[i],i==1||i==2?0xe9:0xe8)){
   gi_phase_audit::Emit(25,{427,0,2,i});return false;}}
 unsigned changed=0;
 for(;changed<4;){auto& p=patches[changed];if(!NativeCpuExchangeCall(&p,true))break;++changed;if(!p.writeHealthy)break;}
 if(changed!=4||!patches[3].writeHealthy){bool rollback=true;
  while(changed){--changed;if(!NativeCpuExchangeCall(&patches[changed],false)||!patches[changed].writeHealthy)rollback=false;}
  gi_phase_audit::Emit(25,{427,0,3,U64(rollback)});return false;}
 installed=true;gi_phase_audit::observer.store(Observe);
 gi_phase_audit::Emit(25,{427,1,0,4});return true;
}
}
