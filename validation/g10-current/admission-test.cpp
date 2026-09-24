#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include "../../src/rr_albedo_prepare.h"
using namespace control_rr_albedo;
static std::map<std::uintptr_t,std::vector<unsigned char>> mem;
static std::uintptr_t mainT=0x2000, albT=0x3000, origS=0x4000, replS=0x5000;
static std::uint32_t albedoOr=0, albedoClear=0;
static bool rd(std::uintptr_t a, void* o, std::size_t n){
  for(auto &kv:mem){ auto b=kv.first,e=b+kv.second.size(); if(a>=b && a+n<=e){ std::memcpy(o,kv.second.data()+(a-b),n); return true; } }
  return false;
}
static bool gs(std::uintptr_t t,std::uint32_t k,std::uintptr_t* out){
  if(t==mainT){*out=origS;return true;}
  if(t==albT){*out=replS;return true;}
  *out=0;return false;
}
template<class T> static void put(std::uintptr_t b,size_t off,T v){auto &x=mem[b]; if(x.size()<off+sizeof(T))x.resize(off+sizeof(T)); std::memcpy(x.data()+off,&v,sizeof(T));}
static void puts(std::uintptr_t b,const char* s){mem[b]=std::vector<unsigned char>(s,s+strlen(s)+1);}
static bool run(const char* family, std::uint32_t mainOr,std::uint32_t mainClear,std::uint32_t mainCount,
                std::uint32_t albOr,std::uint32_t albClear,std::uint32_t albCount,std::uint32_t key,
                std::uint8_t expectedKind){
  mem.clear(); const std::uintptr_t material=0x1000,name=0x6000;
  put(material,0x9A0,mainT); put(material,0x9D8,albT); puts(name,family); put(mainT,0x100,name);
  put(origS,4,key); put(mainT,4,mainOr);put(mainT,8,mainClear);put(mainT,0x240,mainCount);
  put(albT,4,albOr);put(albT,8,albClear);put(albT,0x240,albCount);
  auto requested=(key & ~0x80400000u)|0x02000002u;
  auto effective=CanonicalTechniqueKey(albOr,albClear,requested);
  put(replS,4,effective);
  std::uintptr_t vs=0x7000,ps=0x8000,z=0;put(replS,0x10,vs);put(replS,0x50,ps);put(replS,0x20,z);put(replS,0x30,z);put(replS,0x40,z);put(replS,0x60,z);put(replS,0x70,z);
  OpaqueBatch b{}; b.shader=origS;b.mesh=0x9000;b.material=material;b.instanceCount=1;b.instanceByteOffset=0;
  Access a{rd,gs}; std::uintptr_t selected=0; std::uint8_t kind=0;
  return selectStandardAlbedo(a,b,selected,&kind) && selected==replS && kind==expectedKind;
}
int main(){
  assert(run("standardmaterial",0x02000000u,0,1024,0x02000002u,0,128,0x02000020u,1));
  assert(run("character",0x02000000u,0,128,0x02000000u,64,16,0x02C00000u,2));
  assert(run("cloth",0x02000000u,1,64,0x02000002u,1,8,0x02C00008u,3));
  // G9-observed exact foliage and hair contracts are conditionally admitted.
  assert(run("foliage",0x02000000u,0x0040001Cu,16,0x02000000u,0x3Cu,4,0x02000020u,4));
  assert(run("hair",0x02000000u,0x0Eu,32,0x02000000u,0x0Fu,4,0xC2400000u,5));
  // Nearby technique-contract variants remain rejected.
  assert(!run("foliage",0x02000000u,0x0040001Cu,15,0x02000000u,0x3Cu,4,0x02000020u,4));
  assert(!run("hair",0x02000000u,0x0Eu,32,0x02000000u,0x0Eu,4,0xC2400000u,5));
  // Eye remains unsupported even if a mock shader exists.
  assert(!run("eye",0x02000000u,0,16,0x02000000u,0,4,0x02C00000u,0));
  return 0;
}
