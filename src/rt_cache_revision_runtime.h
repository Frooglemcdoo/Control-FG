#pragma once
#include "rt_cache_revision.h"
#include <mutex>
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
}
