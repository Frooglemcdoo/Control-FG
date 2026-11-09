#pragma once
#include "rt_owned_assembly.h"
#include "rt_cache_revision.h"
namespace rt_prepared_cache {
using namespace rt_owned_assembly;
inline constexpr unsigned MaxGeometry=4;
struct MaterialInput {
 std::uintptr_t pointer=0,flagPointer=0;std::uint64_t revision=0;
 std::uint32_t properties=0,mask=0,cull=0;std::uint16_t id=0;
 bool operator==(const MaterialInput& b)const noexcept{return pointer==b.pointer&&flagPointer==b.flagPointer&&revision==b.revision&&properties==b.properties&&mask==b.mask&&cull==b.cull&&id==b.id;}
};
struct Entry {
 std::uintptr_t provider=0,owner=0,source=0;std::uint64_t revision=0;
 std::int32_t variant=0;std::uint32_t count=0,flags=0;unsigned char ignoreAlpha=0,disableCull=0;bool valid=false;
 unsigned char raw[MaxGeometry][64]{},prepared[MaxGeometry][64]{};
 MaterialInput inputs[MaxGeometry]{};void* materials[MaxGeometry]{};
};
inline std::size_t Slot(void* provider,void* owner,std::int32_t variant)noexcept{
 auto h=rt_cache_revision::Mix(reinterpret_cast<std::uintptr_t>(provider),reinterpret_cast<std::uintptr_t>(owner));h=rt_cache_revision::Mix(h,static_cast<std::uint32_t>(variant));return static_cast<std::size_t>((h^(h>>32)^(h>>12))&4095);
}
inline void* MaterialTable(void* owner,std::int32_t variant)noexcept{
 if(variant!=0&&Read<std::uint32_t>(owner,0x158)!=0)return Read<void*>(Read<unsigned char*>(owner,0x150)+(std::ptrdiff_t(variant)*3-2)*8);
 return Read<void*>(owner,0x130);
}
inline MaterialInput Input(rt_cache_revision::Revisions& revisions,void* material,unsigned char ignoreAlpha)noexcept{
 MaterialInput v;v.pointer=reinterpret_cast<std::uintptr_t>(material);v.id=Read<std::uint16_t>(material,0x99e);v.revision=revisions.materials[v.id].load(std::memory_order_relaxed);v.cull=Read<std::uint32_t>(material,0x204);
 if(!ignoreAlpha){auto* flags=Read<void*>(material,0x1f8);v.flagPointer=reinterpret_cast<std::uintptr_t>(flags);v.properties=Read<std::uint32_t>(material,0x1f0);v.mask=Read<std::uint32_t>(flags);}
 return v;
}
inline bool Alpha(const MaterialInput& input)noexcept{return ((input.properties&input.mask)&0x80000000u)!=0;}
inline std::uint64_t Revision(rt_cache_revision::Revisions& revisions,void* provider)noexcept{return revisions.providers[rt_cache_revision::ProviderSlot(reinterpret_cast<std::uintptr_t>(provider))].load(std::memory_order_relaxed);}
inline bool Eligible(const Api& api,void* provider,std::uint32_t& count)noexcept{
 if(!MatchesProvider(provider,api.staticMesh))return false;const auto n=GeometryCount<1>(provider);if(n<0||n>static_cast<std::int32_t>(MaxGeometry))return false;count=static_cast<std::uint32_t>(n);return true;
}
// Every successful hit compares actual live inputs, not a hash or just revisions.
// No stored native pointer is dereferenced: table and material pointers come from
// the current live call, and geometry is addressed through the current provider.
inline bool Replay(const Api& api,rt_cache_revision::Revisions& revisions,const Entry& entry,void* context,const void* transform,void* owner,std::int32_t variant,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix,Result& result){
 if(!entry.valid||entry.provider!=reinterpret_cast<std::uintptr_t>(provider)||entry.owner!=reinterpret_cast<std::uintptr_t>(owner)||entry.variant!=variant||entry.ignoreAlpha!=ignoreAlpha||entry.disableCull!=disableCull)return false;
 std::uint32_t count=0;if(!Eligible(api,provider,count)||count!=entry.count)return false;
 const auto revision=Revision(revisions,provider);auto* source=Read<unsigned char*>(provider,0x20);
 if(revision!=entry.revision||reinterpret_cast<std::uintptr_t>(source)!=entry.source)return false;
 auto* ctx=static_cast<unsigned char*>(context);if(!FitsBatch(ctx+0x70,1)||!FitsBatch(ctx+0x80,count)||!FitsBatch(ctx+0x90,count))return false;
 if(count&&std::memcmp(source,entry.raw,std::size_t(count)*64))return false;
 auto* table=count?static_cast<unsigned char*>(MaterialTable(owner,variant)):nullptr;
 for(unsigned i=0;i<count;++i){
  const auto index=Read<std::int32_t>(source,std::size_t(i)*64);auto* material=Read<void*>(table+std::ptrdiff_t(index)*8);
  if(!(Input(revisions,material,ignoreAlpha)==entry.inputs[i]))return false;
 }
 if(Revision(revisions,provider)!=revision)return false;
 const auto geomStart=Read<std::uint32_t>(ctx,0x88),matStart=Read<std::uint32_t>(ctx,0x98),instanceStart=Read<std::uint32_t>(ctx,0x78);
 std::uint32_t instance[16]{};
 for(unsigned column=0;column<4;++column)for(unsigned row=0;row<3;++row)instance[row*4+column]=Read<std::uint32_t>(transform,(column*3+row)*4);
 instance[12]=(InstanceMask<1>(provider)<<24)|(entry.flags&0x04000000u);instance[13]=((*prefix*2u)&0xffffffu)|entry.flags;
 const auto address=BlasAddress<1>(provider);std::memcpy(instance+14,&address,8);
 if(count){
  std::memcpy(Read<unsigned char*>(ctx,0x80)+std::size_t(geomStart)*64,entry.prepared,std::size_t(count)*64);
  std::memcpy(Read<unsigned char*>(ctx,0x90)+std::size_t(matStart)*8,entry.materials,std::size_t(count)*8);
 }
 std::memcpy(Read<unsigned char*>(ctx,0x70)+std::size_t(instanceStart)*64,instance,64);
 Write(ctx,0x88,geomStart+count);Write(ctx,0x98,matStart+count);Write(ctx,0x78,instanceStart+1);*prefix+=count;
 result={};result.geometry=count;result.fastAppends=count+1;result.materialFastAppends=count;result.providerKind=1;result.avoidedCalls=count+3;result.reusedGeometry=count;return true;
}
// Populate only from completed ordinary assembly, and check that the live source
// still reconstructs that output exactly. No partially captured entry becomes valid.
inline bool Capture(const Api& api,rt_cache_revision::Revisions& revisions,Entry& entry,void* context,void* owner,std::int32_t variant,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,const Result& result){
 entry.valid=false;if(result.providerKind!=1||result.geometry>MaxGeometry)return false;
 std::uint32_t count=0;if(!Eligible(api,provider,count)||count!=result.geometry)return false;
 Entry next;next.provider=reinterpret_cast<std::uintptr_t>(provider);next.owner=reinterpret_cast<std::uintptr_t>(owner);next.variant=variant;next.count=count;next.ignoreAlpha=ignoreAlpha;next.disableCull=disableCull;next.revision=Revision(revisions,provider);
 auto* source=Read<unsigned char*>(provider,0x20);next.source=reinterpret_cast<std::uintptr_t>(source);
 const auto geometryUsed=Read<std::uint32_t>(context,0x88),materialsUsed=Read<std::uint32_t>(context,0x98),instancesUsed=Read<std::uint32_t>(context,0x78);
 if(geometryUsed<count||materialsUsed<count||!instancesUsed)return false;
 auto* table=count?static_cast<unsigned char*>(MaterialTable(owner,variant)):nullptr;
 bool alpha=false,cull=disableCull!=0;
 for(unsigned i=0;i<count;++i){
  std::memcpy(next.raw[i],source+std::size_t(i)*64,64);std::memcpy(next.prepared[i],next.raw[i],64);
  const auto index=Read<std::int32_t>(next.raw[i]);auto* material=Read<void*>(table+std::ptrdiff_t(index)*8);next.inputs[i]=Input(revisions,material,ignoreAlpha);next.materials[i]=material;
  const bool a=Alpha(next.inputs[i]);alpha|=a;cull|=next.inputs[i].cull!=2;Write(next.prepared[i],4,std::uint32_t(a));
  const auto* actual=Read<unsigned char*>(context,0x80)+std::size_t(geometryUsed-count+i)*64;
  if(std::memcmp(actual,next.prepared[i],64)||Read<void*>(Read<unsigned char*>(context,0x90)+std::size_t(materialsUsed-count+i)*8)!=material)return false;
 }
 next.flags=(!alpha?0x04000000u:0u)|(cull?0x01000000u:0u);
 const auto* instance=Read<unsigned char*>(context,0x70)+std::size_t(instancesUsed-1)*64;
 if((Read<std::uint32_t>(instance,52)&0xff000000u)!=next.flags||Revision(revisions,provider)!=next.revision)return false;
 next.valid=true;entry=next;return true;
}
}
