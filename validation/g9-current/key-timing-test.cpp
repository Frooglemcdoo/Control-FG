#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include "../../src/rr_albedo_prepare.h"
using namespace control_rr_albedo;

static std::map<std::uintptr_t,std::vector<unsigned char>> mem;
static std::uintptr_t mainT=0x2000, albT=0x3000, origS=0x4000, replS=0x5000;
static bool rd(std::uintptr_t a, void* o, std::size_t n){
  for(auto &kv:mem){ auto b=kv.first,e=b+kv.second.size(); if(a>=b && a+n<=e){ std::memcpy(o,kv.second.data()+(a-b),n); return true; } }
  return false;
}
static bool gs(std::uintptr_t t,std::uint32_t k,std::uintptr_t* out){
  if(t==mainT){*out=origS;return true;} if(t==albT){*out=replS;return true;} *out=0;return false;
}
template<class T> static void put(std::uintptr_t b,size_t off,T v){auto &x=mem[b]; if(x.size()<off+sizeof(T))x.resize(off+sizeof(T)); std::memcpy(x.data()+off,&v,sizeof(T));}
static void puts(std::uintptr_t b,const char* s){mem[b]=std::vector<unsigned char>(s,s+strlen(s)+1);} 
static bool run(const char* family, std::uint32_t mainOr,std::uint32_t mainClear,std::uint32_t mainCount,
                std::uint32_t albOr,std::uint32_t albClear,std::uint32_t albCount,std::uint32_t key){
  mem.clear(); const std::uintptr_t material=0x1000,name=0x6000;
  put(material,0x9A0,mainT); put(material,0x9D8,albT); puts(name,family); put(mainT,0x100,name);
  put(origS,4,key); put(mainT,4,mainOr);put(mainT,8,mainClear);put(mainT,0x240,mainCount);
  put(albT,4,albOr);put(albT,8,albClear);put(albT,0x240,albCount);
  auto ak=(key & ~0x80400000u)|0x02000002u; put(replS,4,ak);
  std::uintptr_t vs=0x7000,ps=0x8000,z=0;put(replS,0x10,vs);put(replS,0x50,ps);put(replS,0x20,z);put(replS,0x30,z);put(replS,0x40,z);put(replS,0x60,z);put(replS,0x70,z);
  OpaqueBatch b{}; b.shader=origS;b.mesh=0x9000;b.material=material;b.instanceCount=1;b.instanceByteOffset=0;
  Access a{rd,gs}; std::uintptr_t selected=0; std::uint8_t kind=0;
  return selectStandardAlbedo(a,b,selected,&kind) && selected==replS && kind!=0;
}
int main(){
  // Static getShader normalization examples from the exact G7/G8 runtime audit.
  assert(CanonicalTechniqueKey(0x02000000u,0x3Cu,0x02000022u)==0x02000002u); // foliage
  assert(CanonicalTechniqueKey(0x02000000u,0x0Fu,0x42000002u)==0x42000000u); // hair
  assert(CanonicalTechniqueKey(0x02000000u,0x00u,0x42000002u)==0x42000002u); // eye

  // G8 selection remains unchanged in G9.
  assert(run("standardmaterial",0x02000000u,0,1024,0x02000002u,0,128,0x02000020u));
  assert(run("character",0x02000000u,0,128,0x02000000u,64,16,0x02C00000u));
  assert(run("cloth",0x02000000u,1,64,0x02000002u,1,8,0x02C00008u));
  assert(!run("foliage",0x02000000u,4194332u,16,0x02000000u,60,4,0x02000020u));
  assert(!run("eye",0x02000000u,0,16,0x02000000u,0,4,0x02C00000u));
  assert(!run("hair",0x02000000u,14,32,0x02000000u,15,4,0xC2400000u));

  // Range attribution must preserve the selected batch list and report family composition only.
  PreparedReplay full{};
  full.sourceFirst=0; full.sourceEnd=6;
  full.batches.resize(5); full.sourceIndices={0,1,3,4,5}; full.familyKinds={1,2,2,3,1};
  full.coverage.sourceBatches=6; full.coverage.replayBatches=5; full.coverage.rejectedBatches=1;
  DrawRange range{};
  assert(prepareDrawRange(full,1,5,range));
  assert(range.coverage.sourceBatches==4);
  assert(range.coverage.replayBatches==3);
  assert(range.familyCounts[1]==0 && range.familyCounts[2]==2 && range.familyCounts[3]==1);
  return 0;
}
