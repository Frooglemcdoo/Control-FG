#pragma once
#include "rt_cache_revision.h"
#include "rt_cache_revision_patch.h"
#include "rt_owned_assembly.h"
namespace RTCacheRevision {
inline rt_cache_revision::Revisions revisions;
using Create=void*(*)(void*,void*,std::int32_t,std::int32_t,unsigned char,void*);
using Notify=void(*)(void*);
inline Create originalCreate=nullptr;
inline Notify originalDestroy=nullptr,originalMaterial=nullptr;
inline bool installed=false;
inline void* CreateHook(void* self,void* owner,std::int32_t variant,std::int32_t lod,unsigned char alternate,void* manager){
 revisions.ProviderCreated(self);return originalCreate(self,owner,variant,lod,alternate,manager);
}
inline void DestroyHook(void* self){revisions.ProviderDestroyed(self);originalDestroy(self);}
inline void MaterialHook(void* material){revisions.MaterialChanged(rt_owned_assembly::Read<std::uint16_t>(material,0x99e));originalMaterial(material);}
struct Patch {unsigned char* site=nullptr;unsigned char* relay=nullptr;unsigned char expected[8]{},replacement[8]{};std::size_t size=0;RUNTIME_FUNCTION unwind{};bool ready=false;};
inline Patch patches[3];
template<std::size_t N,std::size_t U>inline bool Prepare(Patch& p,unsigned char* site,void* hook,const unsigned char (&bytes)[N],const unsigned char (&unwind)[U])noexcept{
 static_assert(N<=8,"entry buffer");if(std::memcmp(site,bytes,N))return false;
 p.site=site;p.size=N;std::memcpy(p.expected,bytes,N);std::memset(p.replacement,0x90,N);
 p.relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(site));if(!p.relay)return false;
 if(!rt_cache_revision::Jump5(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(p.relay),p.replacement))return false;
 rt_cache_revision::AbsoluteJump(p.relay,reinterpret_cast<std::uintptr_t>(hook));
 std::memcpy(p.relay+0x80,bytes,N);rt_cache_revision::ResumeJump(p.relay+0x80+N,reinterpret_cast<std::uintptr_t>(site+N));
 std::memcpy(p.relay+0x100,unwind,U);p.unwind={0x80,static_cast<DWORD>(0x80+N+13),0x100};
 DWORD old=0;if(!VirtualProtect(p.relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),p.relay,4096))return false;
 if(!RtlAddFunctionTable(&p.unwind,1,reinterpret_cast<DWORD64>(p.relay)))return false;p.ready=true;return true;
}
inline bool Exchange(Patch& p,bool enable)noexcept{
 if(!p.ready)return false;const auto* before=enable?p.expected:p.replacement;const auto* after=enable?p.replacement:p.expected;
 if(std::memcmp(p.site,before,p.size))return false;
 DWORD old=0;if(!VirtualProtect(p.site,p.size,PAGE_EXECUTE_READWRITE,&old))return false;
 std::memcpy(p.site,after,p.size);const bool flushed=FlushInstructionCache(GetCurrentProcess(),p.site,p.size)!=FALSE;
 DWORD ignored=0;const bool restored=VirtualProtect(p.site,p.size,old,&ignored)!=FALSE;return flushed&&restored;
}
inline bool Install(HMODULE renderer)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);using namespace rt_cache_revision;
 if(!Prepare(patches[0],base+0x1504c0,reinterpret_cast<void*>(&CreateHook),ConstructorEntry,ConstructorUnwind)||
    !Prepare(patches[1],base+0x150aa0,reinterpret_cast<void*>(&DestroyHook),DestructorEntry,DestructorUnwind)||
    !Prepare(patches[2],base+0x1b6910,reinterpret_cast<void*>(&MaterialHook),MaterialEntry,MaterialUnwind))return false;
 originalCreate=reinterpret_cast<Create>(patches[0].relay+0x80);originalDestroy=reinterpret_cast<Notify>(patches[1].relay+0x80);originalMaterial=reinterpret_cast<Notify>(patches[2].relay+0x80);
 for(unsigned i=0;i<3;++i)if(!Exchange(patches[i],true)){
  for(unsigned j=0;j<=i;++j)if(!std::memcmp(patches[j].site,patches[j].replacement,patches[j].size))Exchange(patches[j],false);
  return false;
 }
 installed=true;return true;
}
struct Pending {rt_cache_revision::Entry entries[4096]{};bool enabled=false;unsigned remaining=0;UINT64 observed=0,cold=0,unchanged=0,invalidated=0,untracked=0,skipped=0;};
inline thread_local Pending pending;
inline std::atomic<UINT64> observed{0},cold{0},unchanged{0},invalidated{0},untracked{0},skipped{0};
inline void Begin(UINT64 loop)noexcept{pending.enabled=installed&&loop%64==0;pending.remaining=256;}
inline void Observe(void* context,void* owner,std::int32_t variant,void* provider,unsigned char alpha,unsigned char cull,const rt_owned_assembly::Result& result)noexcept{
 if(!pending.enabled||!pending.remaining)return;--pending.remaining;
 if(result.providerKind!=1||result.geometry>4){++pending.skipped;return;}
 using namespace rt_owned_assembly;using rt_cache_revision::Mix;
 constexpr UINT64 seed=14695981039346656037ull;
 const auto pointer=reinterpret_cast<std::uintptr_t>(provider);
 auto guard=Mix(seed,revisions.providers[rt_cache_revision::ProviderSlot(pointer)].load(std::memory_order_relaxed));
 guard=Mix(guard,reinterpret_cast<std::uintptr_t>(Read<void*>(provider,0x20)));guard=Mix(guard,result.geometry);
 auto payload=seed;const auto count=Read<std::uint32_t>(context,0x88),matCount=Read<std::uint32_t>(context,0x98);
 if(count<result.geometry||matCount<result.geometry){++pending.skipped;return;}
 const auto* data=result.geometry?Read<unsigned char*>(context,0x80)+std::size_t(count-result.geometry)*64:nullptr;
 const auto* materials=result.geometry?Read<unsigned char*>(context,0x90)+std::size_t(matCount-result.geometry)*8:nullptr;
 for(unsigned i=0;i<result.geometry;++i){
  auto* material=Read<void*>(materials,std::size_t(i)*8);guard=Mix(guard,reinterpret_cast<std::uintptr_t>(material));
  const auto id=Read<std::uint16_t>(material,0x99e);guard=Mix(guard,id);guard=Mix(guard,revisions.materials[id].load(std::memory_order_relaxed));
  if(!alpha){const auto* flags=Read<void*>(material,0x1f8);guard=Mix(guard,reinterpret_cast<std::uintptr_t>(flags));guard=Mix(guard,Read<std::uint32_t>(flags));}
  for(unsigned word=0;word<8;++word)payload=Mix(payload,Read<UINT64>(data,std::size_t(i)*64+word*8));
 }
 const auto instanceCount=Read<std::uint32_t>(context,0x78);if(!instanceCount){++pending.skipped;return;}
 const auto* instance=Read<unsigned char*>(context,0x70)+std::size_t(instanceCount-1)*64;
 payload=Mix(payload,Read<std::uint32_t>(instance,48));payload=Mix(payload,Read<std::uint32_t>(instance,52)&0xff000000u);
 // Transform, contribution prefix and live BLAS address are deliberately excluded:
 // a future cache must always refresh them. No stored native pointer is dereferenced.
 rt_cache_revision::Observation value{pointer,reinterpret_cast<std::uintptr_t>(owner),variant,static_cast<std::uint32_t>(alpha)|(static_cast<std::uint32_t>(cull)<<8),guard,payload};
 const auto hash=Mix(Mix(pointer,value.owner),static_cast<std::uint32_t>(variant));const auto slot=(hash^(hash>>32)^(hash>>12))&4095;
 const auto outcome=rt_cache_revision::Observe(pending.entries[slot],value);++pending.observed;
 switch(outcome){case rt_cache_revision::Outcome::Cold:++pending.cold;break;case rt_cache_revision::Outcome::Unchanged:++pending.unchanged;break;case rt_cache_revision::Outcome::Invalidated:++pending.invalidated;break;case rt_cache_revision::Outcome::Untracked:++pending.untracked;break;}
}
inline void End()noexcept{
 if(!pending.enabled)return;pending.enabled=false;
 observed.fetch_add(pending.observed,std::memory_order_relaxed);cold.fetch_add(pending.cold,std::memory_order_relaxed);unchanged.fetch_add(pending.unchanged,std::memory_order_relaxed);invalidated.fetch_add(pending.invalidated,std::memory_order_relaxed);untracked.fetch_add(pending.untracked,std::memory_order_relaxed);skipped.fetch_add(pending.skipped,std::memory_order_relaxed);
 pending.observed=pending.cold=pending.unchanged=pending.invalidated=pending.untracked=pending.skipped=0;
}
inline void Report(){
 Log("RT_CACHE_R401 active=%u creates=%llu destroys=%llu material_notifications=%llu observed=%llu cold=%llu unchanged=%llu invalidated=%llu untracked_changes=%llu skipped=%llu sample_period=64 max_samples=256 cache_reuse=0",unsigned(installed),revisions.creates.load(),revisions.destroys.load(),revisions.notifications.load(),observed.load(),cold.load(),unchanged.load(),invalidated.load(),untracked.load(),skipped.load());
}
}
