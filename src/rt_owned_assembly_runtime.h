#pragma once
#include "rt_owned_assembly.h"
#include "rt_owned_provider_bytes.h"
namespace RTOwnedAssembly {
inline rt_owned_assembly::Api api{};
inline RRAlbedoCallPatch patches[3]{};
inline std::atomic<UINT64> calls{0},geometries{0},fast{0},growth{0},materialFast{0},materialGrowth{0},staticProviders{0},skinnedProviders{0},virtualProviders{0},avoidedCalls{0},reusedGeometry{0};
inline void Record(const rt_owned_assembly::Result& result,UINT64 instanceCount=1){
 struct Pending {UINT64 calls=0,geometry=0,fast=0,growth=0,materialFast=0,materialGrowth=0,staticProviders=0,skinnedProviders=0,virtualProviders=0,avoidedCalls=0,reusedGeometry=0;bool started=false;};static thread_local Pending pending;
 pending.calls+=instanceCount;pending.geometry+=result.geometry;pending.fast+=result.fastAppends;pending.growth+=result.growthAppends;pending.materialFast+=result.materialFastAppends;pending.materialGrowth+=result.materialGrowthAppends;
 pending.staticProviders+=result.providerKind==1?instanceCount:0;pending.skinnedProviders+=result.providerKind==2?instanceCount:0;pending.virtualProviders+=result.providerKind==0?instanceCount:0;pending.avoidedCalls+=result.avoidedCalls;pending.reusedGeometry+=result.reusedGeometry;
 if(pending.calls>=4096||!pending.started){
  geometries.fetch_add(pending.geometry,std::memory_order_relaxed);fast.fetch_add(pending.fast,std::memory_order_relaxed);growth.fetch_add(pending.growth,std::memory_order_relaxed);
  materialFast.fetch_add(pending.materialFast,std::memory_order_relaxed);materialGrowth.fetch_add(pending.materialGrowth,std::memory_order_relaxed);
  staticProviders.fetch_add(pending.staticProviders,std::memory_order_relaxed);skinnedProviders.fetch_add(pending.skinnedProviders,std::memory_order_relaxed);virtualProviders.fetch_add(pending.virtualProviders,std::memory_order_relaxed);avoidedCalls.fetch_add(pending.avoidedCalls,std::memory_order_relaxed);
  reusedGeometry.fetch_add(pending.reusedGeometry,std::memory_order_relaxed);
  const auto before=calls.fetch_add(pending.calls,std::memory_order_relaxed);const auto n=before+pending.calls;pending={};pending.started=true;
  if(!before||(before>>18)!=(n>>18)){const DWORD error=GetLastError();
   Log("RT_ASSEMBLY_R402 active=1 calls=%llu geometry=%llu fast_appends=%llu native_growth_appends=%llu material_fast_appends=%llu material_growth_appends=%llu static_providers=%llu skinned_providers=%llu virtual_providers=%llu avoided_provider_calls=%llu reused_geometry_preparations=%llu native_object_assembler_calls=0 native_updates=1 ptlas=0",n,geometries.load(),fast.load(),growth.load(),materialFast.load(),materialGrowth.load(),staticProviders.load(),skinnedProviders.load(),virtualProviders.load(),avoidedCalls.load(),reusedGeometry.load());SetLastError(error);
  }
 }
}
inline void Hook(void* context,const void* transform,void* materials,std::int32_t variant,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 const auto result=rt_owned_assembly::Assemble(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix);
 Record(result);
}
inline std::uint32_t HookBatch(void* context,const void* transforms,std::uint32_t count,void* materials,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 rt_owned_assembly::Result total;
 if(rt_owned_assembly::TryAssembleBatch(api,context,transforms,count,materials,provider,ignoreAlpha,disableCull,prefix,total)){Record(total,count);return count-1;}
 for(std::uint32_t i=0;i<count;++i)Hook(context,static_cast<const unsigned char*>(transforms)+std::size_t(i)*48,materials,0,provider,ignoreAlpha,disableCull,prefix);
 return 0;
}
template<std::size_t N>inline bool GetterMatches(unsigned char* base,std::uint32_t rva,const unsigned char (&bytes)[N])noexcept{return !std::memcmp(base+rva,bytes,N);}
inline void BindProviders(unsigned char* base)noexcept{
 using namespace rt_provider_bytes;
 const auto bind=[&](std::uint32_t table,std::uint32_t address,std::uint32_t count,std::uint32_t geometry,std::uint32_t mask){
  rt_owned_assembly::ProviderBinding binding{base+table,base+address,base+count,base+geometry,base+mask};
  // Check slots as well as code: another mod may have redirected the vtable.
  void* object=base+table;
  return rt_owned_assembly::MatchesProvider(&object,binding)?binding:rt_owned_assembly::ProviderBinding{};
 };
 if(GetterMatches(base,StaticAddressRva,StaticAddress)&&GetterMatches(base,StaticCountRva,StaticCount)&&GetterMatches(base,StaticGeometryRva,StaticGeometry)&&GetterMatches(base,StaticMaskRva,StaticMask))
  api.staticMesh=bind(0x6332f0,StaticAddressRva,StaticCountRva,StaticGeometryRva,StaticMaskRva);
 if(GetterMatches(base,SkinnedAddressRva,SkinnedAddress)&&GetterMatches(base,SkinnedCountRva,SkinnedCount)&&GetterMatches(base,SkinnedGeometryRva,SkinnedGeometry)&&GetterMatches(base,SkinnedMaskRva,SkinnedMask))
  api.skinnedMesh=bind(0x627e38,SkinnedAddressRva,SkinnedCountRva,SkinnedGeometryRva,SkinnedMaskRva);
}
inline bool Install(HMODULE renderer)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);const DWORD sites[]={0xa8578,0xa8713,0xa87c2};
 api={reinterpret_cast<void(*)(void*,const void*)>(base+0xa6d20),reinterpret_cast<void(*)(void*,const void*)>(base+0x22730)};
 BindProviders(base);
 for(unsigned i=0;i<3;++i)if(!RRAlbedoPrepareCallPatch(base+sites[i],base+0xa8ce0,reinterpret_cast<void*>(&Hook),&patches[i]))return false;
 for(unsigned i=0;i<3;++i){
  if(RRAlbedoExchangeCall(&patches[i],true)&&patches[i].writeHealthy)continue;
  for(unsigned j=0;j<=i;++j)if(patches[j].site&&!memcmp(patches[j].site,patches[j].replacement,5))RRAlbedoExchangeCall(&patches[j],false);
  return false;
 }
 return true;
}
}
