#pragma once
#include "rt_owned_assembly.h"
namespace rt_owned_record {
using rt_owned_assembly::Read;using rt_owned_assembly::Write;using rt_owned_assembly::Method;
struct ThreadInit {const void* inherited=nullptr;std::uint32_t flags=0,padding=0;void* queue=nullptr;};
static_assert(sizeof(ThreadInit)==24,"native ThreadInit");
struct Api {
 void*(*queue)(std::int32_t);void(*initialize)(const ThreadInit*);void*(*device)();void(*bindCommon)(void*);
 void(*consumeToUav)(void*,std::int32_t);void(*uavToConsume)(void*,std::int32_t);
 void(*uavToCopy)(void*);void(*copy)(void*,void*);void(*copyToUav)(void*);
 void(*barrier)();void(*build)(std::uint32_t,void*,std::int32_t,void*,void*,bool);void*(*finish)();
 // Optional boundaries: thread, device, common bindings, transition-in,
 // provider updates, transition-out, BLAS/finalize, material IDs, TLAS/finish.
 // Never inside a provider/material loop. CPU intervals, not GPU durations.
 void(*phase)(unsigned)=nullptr;
};
struct Globals {const unsigned char* asyncQueue;const void* inherited;void* const* skinning;void* const* postbuild;void** cleanup;};
struct Stats {std::uint64_t updates=0,builds=0,secondaryBuilds=0,finalizes=0,materials=0;std::int32_t instances=0;bool copied=false;};
inline Stats Run(const Api& api,const Globals& globals,void* capture){
 Stats stats;ThreadInit init;init.inherited=globals.inherited;init.queue=api.queue(*globals.asyncQueue!=0);api.initialize(&init);
 auto context=[&]{return Read<void*>(capture);};auto* ctx=context();stats.instances=Read<std::int32_t>(ctx,0x78);
 if(api.phase)api.phase(0);
 if(stats.instances>0){
  if(Read<std::uint32_t>(ctx,0x28)){
   auto* device=api.device();if(api.phase)api.phase(1);
   api.bindCommon(static_cast<unsigned char*>(*globals.skinning)+8);if(api.phase)api.phase(2);
   ctx=context();api.consumeToUav(Read<void*>(ctx,0x30),Read<std::int32_t>(ctx,0x38));
   if(api.phase)api.phase(3);
   ctx=context();auto* list=Read<unsigned char*>(ctx,0x20);const auto count=Read<std::uint32_t>(ctx,0x28);
   for(std::uint32_t i=0;i<count;++i){auto* p=Read<void*>(list,std::size_t(i)*8);Method<void(*)(void*,void*)>(p,0x28)(p,device);++stats.updates;}
   if(api.phase)api.phase(4);
   ctx=context();api.uavToConsume(Read<void*>(ctx,0x30),Read<std::int32_t>(ctx,0x38));ctx=context();
  }else if(api.phase){for(unsigned i=1;i<5;++i)api.phase(i);}
  if(api.phase)api.phase(5);
  if(Read<std::uint32_t>(ctx,0x48)||Read<std::uint32_t>(ctx,0x58)){
   auto* list=Read<unsigned char*>(ctx,0x40);auto count=Read<std::uint32_t>(ctx,0x48);
   for(std::uint32_t i=0;i<count;++i){auto* p=Read<void*>(list,std::size_t(i)*8);Method<void(*)(void*)>(p,0x30)(p);++stats.builds;}
   // Native reloads context only if the first loop ran.
   if(count)ctx=context();list=Read<unsigned char*>(ctx,0x50);count=Read<std::uint32_t>(ctx,0x58);
   for(std::uint32_t i=0;i<count;++i){auto* p=Read<void*>(list,std::size_t(i)*8);Method<void(*)(void*)>(p,0x30)(p);++stats.secondaryBuilds;}
   if(count)ctx=context();
  }
  if(Read<std::uint32_t>(ctx,0x48)){
   auto* buffers=static_cast<unsigned char*>(*globals.postbuild);const auto parity=Read<std::uint64_t>(ctx,0xd8)&1;
   api.uavToCopy(Read<void*>(buffers));api.copy(Read<void*>(buffers,8+std::size_t(parity)*8),Read<void*>(buffers));api.copyToUav(Read<void*>(buffers));stats.copied=true;ctx=context();
  }
  if(Read<std::uint32_t>(ctx,0x68)){
   auto* list=Read<unsigned char*>(ctx,0x60);const auto count=Read<std::uint32_t>(ctx,0x68);
   for(std::uint32_t i=0;i<count;++i){auto* p=Read<void*>(list,std::size_t(i)*8);Method<void(*)(void*)>(p,0x38)(p);++stats.finalizes;}
   ctx=context();
  }
  if(api.phase)api.phase(6);
  if(Read<std::uint32_t>(ctx,0x88)){
   std::uint64_t i=0;do{ctx=context();auto* material=Read<void*>(Read<unsigned char*>(ctx,0x90),std::size_t(i)*8);auto* handle=Read<void*>(material,0xa20);
    Write(Read<unsigned char*>(ctx,0x80),std::size_t(i)*64+36,std::uint32_t(Read<std::uint16_t>(handle,4)));++i;++stats.materials;ctx=context();
   }while(i<Read<std::uint32_t>(ctx,0x88));
  }
 }else{if(api.phase){for(unsigned i=1;i<=6;++i)api.phase(i);}}
 if(api.phase)api.phase(7);
 api.barrier();ctx=context();api.build(4,Read<void*>(ctx,0xb8),stats.instances,Read<void*>(ctx,0xc0),Read<void*>(ctx,0xc8),false);
 *globals.cleanup=api.finish();if(api.phase)api.phase(8);return stats;
}
}
