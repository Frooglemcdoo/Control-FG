#pragma once
#include "rt_owned_assembly.h"
namespace rt_owned_prepare {
using rt_owned_assembly::Read;
using rt_owned_assembly::Method;
using Assemble=void(*)(void*,const void*,void*,std::int32_t,void*,unsigned char,unsigned char,std::uint32_t*);
using AssembleBatch=std::uint32_t(*)(void*,const void*,std::uint32_t,void*,void*,unsigned char,unsigned char,std::uint32_t*);
struct Api {std::uint32_t(*count)(void*);void(*markUpdated)(void*);Assemble assemble;AssembleBatch batch=nullptr;};
struct Options {unsigned char forceLodZero=0,ignoreAlpha=0,disableCull=0;float threshold=0.001f;};
struct Stats {std::uint64_t objects=0,instances=0,dynamicUpdates=0,marked=0,batchGroups=0,reusedInstances=0;};
inline bool Ready(void* provider){using Fn=unsigned char(*)(void*);return provider&&Method<Fn>(provider,8)(provider)!=0;}
inline Stats Run(const Api& api,void* context,unsigned char alternate,const Options& options){
 Stats stats;std::uint32_t prefix=0;
 auto* list=Read<unsigned char*>(context,0x10);const auto size=Read<std::uint32_t>(context,0x18);
 for(std::uint32_t i=0;i<size;++i){
  auto* object=Read<void*>(list,std::size_t(i)*8);++stats.objects;
  const auto type=Read<std::uint32_t>(object,8);
  if(type==0x800){
   std::uint32_t index=0;if(!api.count(object))continue;
   do{
    auto* sub=Read<void*>(Read<unsigned char*>(object,0x60),0x18+std::size_t(index)*8);
    if(Read<std::uint32_t>(Read<void*>(sub,0x3d0),0x11ec)==3){
     auto* owner=Read<void*>(sub,0x3d8);if(owner)owner=Read<void*>(owner,0xa0);
     auto* provider=Read<void*>(Read<void*>(Read<void*>(owner,0x230)));
     if(Ready(provider)){
      auto* transforms=Read<unsigned char*>(sub,0x360);const auto count=Read<std::uint32_t>(sub,0x368);
      if(api.batch&&count>1){++stats.batchGroups;stats.reusedInstances+=api.batch(context,transforms,count,owner,provider,options.ignoreAlpha,options.disableCull,&prefix);stats.instances+=count;}
      else for(std::uint32_t t=0;t<count;++t){api.assemble(context,transforms+std::size_t(t)*48,owner,0,provider,options.ignoreAlpha,options.disableCull,&prefix);++stats.instances;}
     }
    }
    ++index;
   }while(index<api.count(object));
  }else if(type==0x100){
   auto* owner=Read<void*>(object,0x60);std::int32_t lod=0;
   if(!options.forceLodZero){
    lod=static_cast<std::int32_t>(Read<std::uint32_t>(owner,0x204)-1u);
    // Native COMISS/JBE stops on equality or unordered (NaN), including LOD 0.
    while(lod>0&&options.threshold>Read<float>(static_cast<unsigned char*>(object)+0x148+std::ptrdiff_t(lod)*4))--lod;
   }
   const auto variant=Read<std::int32_t>(object,0x140);const auto offset=std::ptrdiff_t(variant)*16;
   auto providerAt=[&](std::size_t tableOffset){return Read<void*>(Read<unsigned char*>(Read<unsigned char*>(owner,tableOffset)+offset)+std::ptrdiff_t(lod)*8);};
   void* primary[2]={providerAt(0x230),nullptr};if(alternate)primary[1]=providerAt(0x240);
   void* dynamic[2]={Read<void*>(Read<unsigned char*>(object,0x258)+std::ptrdiff_t(lod)*8),nullptr};
   if(alternate)dynamic[1]=Read<void*>(Read<unsigned char*>(object,0x268)+std::ptrdiff_t(lod)*8);
   const auto* transform=static_cast<unsigned char*>(object)+0x70;
   for(auto* provider:primary)if(Ready(provider)){api.assemble(context,transform,owner,variant,provider,options.ignoreAlpha,options.disableCull,&prefix);++stats.instances;}
   for(auto* provider:dynamic)if(Ready(provider)){
    const auto before=Read<std::uint32_t>(context,0x28);
    using Update=void(*)(void*,std::uint64_t,void*,void*,void*,void*);
    auto* ctx=static_cast<unsigned char*>(context);
    Method<Update>(provider,0x20)(provider,Read<std::uint64_t>(context,0xd8),ctx+0x20,ctx+0x30,ctx+0x50,ctx+0x60);++stats.dynamicUpdates;
    if(Read<std::uint32_t>(context,0x28)>before){api.markUpdated(object);++stats.marked;}
    api.assemble(context,transform,owner,variant,provider,options.ignoreAlpha,options.disableCull,&prefix);++stats.instances;
   }
  }
 }
 return stats;
}
}
