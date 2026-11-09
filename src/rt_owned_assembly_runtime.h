#pragma once
#include "rt_owned_assembly.h"
namespace RTOwnedAssembly {
inline rt_owned_assembly::Api api{};
inline RRAlbedoCallPatch patches[3]{};
inline std::atomic<UINT64> calls{0},geometries{0},fast{0},growth{0},materialFast{0},materialGrowth{0};
inline void Hook(void* context,const void* transform,void* materials,std::int32_t variant,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 const auto result=rt_owned_assembly::Assemble(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix);
 struct Pending {UINT64 calls=0,geometry=0,fast=0,growth=0,materialFast=0,materialGrowth=0;bool started=false;};static thread_local Pending pending;
 ++pending.calls;pending.geometry+=result.geometry;pending.fast+=result.fastAppends;pending.growth+=result.growthAppends;pending.materialFast+=result.materialFastAppends;pending.materialGrowth+=result.materialGrowthAppends;
 if(pending.calls==4096||!pending.started){
  geometries.fetch_add(pending.geometry,std::memory_order_relaxed);fast.fetch_add(pending.fast,std::memory_order_relaxed);growth.fetch_add(pending.growth,std::memory_order_relaxed);
  materialFast.fetch_add(pending.materialFast,std::memory_order_relaxed);materialGrowth.fetch_add(pending.materialGrowth,std::memory_order_relaxed);
  const auto before=calls.fetch_add(pending.calls,std::memory_order_relaxed);const auto n=before+pending.calls;pending={};pending.started=true;
  if(!before||(before>>18)!=(n>>18)){const DWORD error=GetLastError();
   Log("RT_ASSEMBLY_R398 active=1 calls=%llu geometry=%llu fast_appends=%llu native_growth_appends=%llu material_fast_appends=%llu material_growth_appends=%llu native_object_assembler_calls=0 native_updates=1 ptlas=0",n,geometries.load(),fast.load(),growth.load(),materialFast.load(),materialGrowth.load());SetLastError(error);
  }
 }
}
inline bool Install(HMODULE renderer)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);const DWORD sites[]={0xa8578,0xa8713,0xa87c2};
 api={reinterpret_cast<void(*)(void*,const void*)>(base+0xa6d20),reinterpret_cast<void(*)(void*,const void*)>(base+0x22730)};
 for(unsigned i=0;i<3;++i)if(!RRAlbedoPrepareCallPatch(base+sites[i],base+0xa8ce0,reinterpret_cast<void*>(&Hook),&patches[i]))return false;
 for(unsigned i=0;i<3;++i){
  if(RRAlbedoExchangeCall(&patches[i],true)&&patches[i].writeHealthy)continue;
  for(unsigned j=0;j<=i;++j)if(patches[j].site&&!memcmp(patches[j].site,patches[j].replacement,5))RRAlbedoExchangeCall(&patches[j],false);
  return false;
 }
 return true;
}
}
