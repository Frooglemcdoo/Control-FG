#include <windows.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <algorithm>
#include "../../src/rt_owned_assembly.h"
#include "native-assembly-oracle.h"
using namespace rt_owned_assembly;
using AssembleFn=void(*)(void*,const void*,void*,std::int32_t,void*,unsigned char,unsigned char,std::uint32_t*);
static unsigned appendCalls=0,materialCalls=0;
static void Append(void* vector,const void* source,std::size_t stride){
 auto count=Read<std::uint32_t>(vector,8),cap=Read<std::uint32_t>(vector,12)&0x3fffffffu;assert(count<128);
 if(count==cap)Write(vector,12,(std::max)(8u,count+1));
 auto* data=Read<unsigned char*>(vector);Write(vector,8,count+1);std::memcpy(data+count*stride,source,stride);
}
static void MockAppend64(void* vector,const void* source){++appendCalls;Append(vector,source,64);}
static void MockAppend8(void* vector,const void* source){++materialCalls;Append(vector,source,8);}
struct Fixture {
 alignas(16) unsigned char context[0xe0]{};
 std::array<std::array<unsigned char,64>,128> geometry{},instances{};
 std::array<void*,128> materials{};
 Fixture(unsigned capacity){Write(context,0x70,instances.data());Write(context,0x80,geometry.data());Write(context,0x90,materials.data());for(auto off:{0x7c,0x8c,0x9c})Write(context,off,capacity);}
};
struct Provider {void** table=nullptr;int count=0;std::uint32_t mask=0;std::uint64_t blas=0;std::array<std::array<unsigned char,64>,64> geometry{};std::array<unsigned,128> calls{};unsigned callCount=0;};
static int Count(void* p){auto& v=*static_cast<Provider*>(p);v.calls[v.callCount++]=0x100;return v.count;}
static const void* Geometry(void* p,int i){auto& v=*static_cast<Provider*>(p);v.calls[v.callCount++]=unsigned(i);return v.geometry[i].data();}
static std::uint32_t Mask(void* p){auto& v=*static_cast<Provider*>(p);v.calls[v.callCount++]=0x200;return v.mask;}
static std::uint64_t Blas(void* p){auto& v=*static_cast<Provider*>(p);v.calls[v.callCount++]=0x300;return v.blas;}
static AssembleFn Oracle(){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);
 std::memcpy(code,nativeAssemblyOracle,sizeof(nativeAssemblyOracle));
 auto call=[&](unsigned offset,unsigned target){assert(code[offset]==0xe8);const auto delta=std::int32_t(target-offset-5);std::memcpy(code+offset+1,&delta,4);};
 auto stub=[&](unsigned offset,void* target){code[offset]=0x48;code[offset+1]=0xb8;std::memcpy(code+offset+2,&target,8);code[offset+10]=0xff;code[offset+11]=0xe0;};
 stub(0x400,reinterpret_cast<void*>(&MockAppend64));stub(0x420,reinterpret_cast<void*>(&MockAppend8));code[0x440]=0xc3;
 call(0xa8d93-0xa8ce0,0x400);call(0xa8ded-0xa8ce0,0x420);call(0xa8efc-0xa8ce0,0x400);
 // Only security-cookie mechanics are neutralized; assembly logic is unchanged.
 call(0xa8f10-0xa8ce0,0x440);const auto displacement=std::int32_t(0x800-(0xa8cff-0xa8ce0)-7);std::memcpy(code+(0xa8cff-0xa8ce0)+3,&displacement,4);
 DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,4096));return reinterpret_cast<AssembleFn>(code);
}
int main(){
 const auto original=Oracle();void* vtable[13]{};vtable[8]=reinterpret_cast<void*>(&Blas);vtable[9]=reinterpret_cast<void*>(&Count);vtable[10]=reinterpret_cast<void*>(&Geometry);vtable[12]=reinterpret_cast<void*>(&Mask);
 std::uint32_t seed=0x12345678;auto random=[&]{seed=seed*1664525u+1013904223u;return seed;};
 unsigned nativeAppends=0,ownedAppends=0,nativeMaterials=0,ownedMaterials=0;
 for(unsigned test=0;test<4096;++test){
  const unsigned start=(test/2)%32;
  const unsigned capacity=(test%2?128:start)|((test%4)<<30);
  Fixture reference(capacity),owned(capacity);
  for(auto offset:{0x78u,0x88u,0x98u}){Write(reference.context,offset,start);Write(owned.context,offset,start);}
  Provider provider;provider.table=vtable;provider.count=int(test%65);provider.mask=test%256;provider.blas=std::uint64_t(random())<<8;
  std::array<std::uint32_t,12> transform;for(auto& v:transform)v=random();if(test%3==0)transform[3]=0x7fc01234; // NaN payload, copied as bits.
  alignas(8) unsigned char materialOwner[0x160]{};
  std::array<std::array<unsigned char,0x208>,7> materialRecords{};std::array<std::uint32_t,7> flags{};std::array<void*,7> base{},alternate{};
  for(unsigned i=0;i<7;++i){base[i]=materialRecords[i].data();flags[i]=random();Write(base[i],0x1f0,random());Write(base[i],0x1f8,&flags[i]);Write(base[i],0x204,std::uint32_t((test+i)%4));}
  for(unsigned i=0;i<7;++i)alternate[i]=base[6-i];
  std::array<void*,6> variants{};variants[1]=alternate.data();variants[4]=base.data();
  Write(materialOwner,0x130,base.data());Write(materialOwner,0x150,variants.data());Write(materialOwner,0x158,std::uint32_t(test%2));
  for(int i=0;i<provider.count;++i){for(auto& b:provider.geometry[i])b=static_cast<unsigned char>(random());Write(provider.geometry[i].data(),0,std::int32_t(i%7));}
  std::uint32_t aPrefix=random(),bPrefix=aPrefix;const auto variant=std::int32_t(test%3);const auto ignore=static_cast<unsigned char>((test/3)%2),cull=static_cast<unsigned char>((test/5)%2);
  appendCalls=materialCalls=0;original(reference.context,transform.data(),materialOwner,variant,&provider,ignore,cull,&aPrefix);nativeAppends+=appendCalls;nativeMaterials+=materialCalls;
  const auto expectedCalls=provider.calls;const auto expectedCount=provider.callCount;provider.callCount=0;provider.calls={};
  appendCalls=materialCalls=0;const auto result=Assemble({&MockAppend64,&MockAppend8},owned.context,transform.data(),materialOwner,variant,&provider,ignore,cull,&bPrefix);ownedAppends+=appendCalls;ownedMaterials+=materialCalls;
  assert(result.materialFastAppends+result.materialGrowthAppends==static_cast<unsigned>(provider.count));
  assert(result.materialGrowthAppends==materialCalls);
  assert(aPrefix==bPrefix&&expectedCalls==provider.calls&&expectedCount==provider.callCount);
  assert(result.geometry==static_cast<unsigned>(provider.count));
  assert(reference.geometry==owned.geometry&&reference.instances==owned.instances&&reference.materials==owned.materials);
  for(unsigned offset:{0x78u,0x7cu,0x88u,0x8cu,0x98u,0x9cu})assert(Read<std::uint32_t>(reference.context,offset)==Read<std::uint32_t>(owned.context,offset));
 }
 assert(ownedAppends<nativeAppends&&ownedMaterials<nativeMaterials);
 assert(VirtualFree(reinterpret_cast<void*>(original),0,MEM_RELEASE));
 std::printf("PASS 4096 exact native-machine-code comparisons; transforms/NaNs, masks, material overrides, geometry bytes, counters, provider call order and growth. Native append calls=%u owned growth calls=%u. Native material calls=%u owned material growth=%u. Not an FPS benchmark.\n",nativeAppends,ownedAppends,nativeMaterials,ownedMaterials);
}
