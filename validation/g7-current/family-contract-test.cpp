#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include "../../src/rr_albedo_prepare.h"
using namespace control_rr_albedo;
static std::map<std::uintptr_t,std::vector<unsigned char>> mem;
static bool rd(std::uintptr_t a, void* o, std::size_t n){
  for(auto &kv:mem){ auto b=kv.first,e=b+kv.second.size(); if(a>=b && a+n<=e){ std::memcpy(o,kv.second.data()+(a-b),n); return true; } }
  return false;
}
static std::uintptr_t mainT=0x2000, albT=0x3000, origS=0x4000, replS=0x5000;
static bool gs(std::uintptr_t t,std::uint32_t k,std::uintptr_t* out){
  if(t==mainT){*out=origS;return true;} if(t==albT){*out=replS;return true;} *out=0;return false;
}
template<class T> static void put(std::uintptr_t b,size_t off,T v){auto &x=mem[b]; if(x.size()<off+sizeof(T))x.resize(off+sizeof(T)); std::memcpy(x.data()+off,&v,sizeof(T));}
static void puts(std::uintptr_t b,const char* s){mem[b]=std::vector<unsigned char>(s,s+strlen(s)+1);} 
int main(){
  const std::uintptr_t material=0x1000,name=0x6000;
  put(material,0x9A0,mainT); put(material,0x9D8,albT); puts(name,"foliage"); put(mainT,0x100,name);
  std::uint32_t key=0x02000020u; put(origS,4,key); put(mainT,4,0x02000000u);put(mainT,8,0u);put(mainT,0x240,1024u);
  put(albT,4,0x02000002u);put(albT,8,0u);put(albT,0x240,128u);
  auto ak=(key & ~0x80400000u)|0x02000002u; put(replS,4,ak);
  std::uintptr_t vs=0x7000,ps=0x8000,z=0;put(replS,0x10,vs);put(replS,0x50,ps);put(replS,0x20,z);put(replS,0x30,z);put(replS,0x40,z);put(replS,0x60,z);put(replS,0x70,z);
  OpaqueBatch b{}; b.shader=origS;b.mesh=0x9000;b.material=material;b.instanceCount=1;b.instanceByteOffset=0;
  RejectionExample e{}; Access a{rd,gs}; auto r=DiagnoseReject(a,b,e);
  assert(r==RejectReason::UnsupportedMaterialFamily); assert(!strcmp(e.materialFamily,"foliage"));
  assert(e.techniqueContractReadable&&e.mainOr==0x02000000u&&e.mainCount==1024u&&e.albedoOr==0x02000002u&&e.albedoCount==128u);
  assert(e.originalLookupSucceeded&&e.originalMatchesBatch&&e.albedoLookupSucceeded&&e.replacementKeyMatches);
  assert(e.stageContractReadable&&e.hasVertex&&e.hasPixel&&!e.hasHull&&!e.hasDomain&&!e.hasGeometry&&!e.hasCompute&&!e.hasRay);
  // Audit must not change selection behavior: foliage remains unsupported.
  std::uintptr_t selected=0; assert(!selectStandardAlbedo(a,b,selected));
  return 0;
}
