#include <windows.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <algorithm>
#include "../../src/rt_prepared_cache.h"
#include "native-assembly-oracle.h"
#include "../../src/rt_owned_provider_bytes.h"
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

static void TestPreparedCache(AssembleFn original,const Api& api){
 static rt_cache_revision::Revisions revisions;
 unsigned hits=0,rejected=0;
 for(unsigned test=0;test<2048;++test){
  unsigned char provider[0xb0]{},owner[0x160]{},allocation[0x20]{},otherAllocation[0x20]{};
  std::array<std::array<unsigned char,64>,5> geometry{},otherGeometry{};
  std::array<std::array<unsigned char,0xa30>,2> material{};
  std::uint32_t flags=0x80000000u,otherFlags=0xffffffffu;
  void* table[]={material[0].data(),material[1].data()};void* alternate[]={material[1].data(),material[0].data()};
  void* variants[6]{};variants[1]=alternate;variants[4]=table;
  Write(provider,0,api.staticMesh.table);Write(provider,0x20,geometry.data());Write(provider,0x28,static_cast<std::int32_t>(test%5));
  Write(provider,0x68,allocation);Write(provider,0x88,otherAllocation);Write(owner,0x130,table);Write(owner,0x150,variants);Write(owner,0x158,1u);
  for(unsigned i=0;i<2;++i){Write(material[i].data(),0x1f8,&flags);Write(material[i].data(),0x1f0,(test&1)?0x80000000u:0u);Write(material[i].data(),0x204,test%4);Write(material[i].data(),0x99e,static_cast<std::uint16_t>(i));}
  for(unsigned i=0;i<5;++i){std::memset(geometry[i].data(),static_cast<int>(test&255),64);Write(geometry[i].data(),0,std::int32_t(i%2));}
  std::array<std::uint32_t,12> transform{};const auto variant=static_cast<std::int32_t>(test%3);
  const auto ignore=static_cast<unsigned char>((test/3)%2),cull=static_cast<unsigned char>((test/7)%2);
  rt_prepared_cache::Entry entry;
  auto capture=[&]{Fixture f(128);std::uint32_t prefix=0;auto result=Assemble(api,f.context,transform.data(),owner,variant,provider,ignore,cull,&prefix);assert(rt_prepared_cache::Capture(api,revisions,entry,f.context,owner,variant,provider,ignore,cull,result));};
  capture();
  // Transform, instance mask, prefix, compacted BLAS state/address remain live.
  for(unsigned i=0;i<12;++i)transform[i]=test*0x1234567u+i;transform[5]=0x7fc01234u;
  Write(provider,0x60,test*0x1020304u);Write(provider,0xac,static_cast<std::int32_t>(test%7)-2);
  Write(provider,0x70,std::int32_t(-2147483647-1));Write(provider,0x90,std::int32_t(-1));
  Write(allocation,0x10,std::uint64_t(test)*0xabcdef123456789ull);Write(otherAllocation,0x10,~std::uint64_t(test)*0x7654321ull);
  auto compare=[&]{const auto start=test%9,cap=128u|((test%4)<<30);Fixture ref(cap),cached(cap);for(auto off:{0x78,0x88,0x98}){Write(ref.context,off,start);Write(cached.context,off,start);}auto a=test*0xfedcba9u,b=a;Result result;
   original(ref.context,transform.data(),owner,variant,provider,ignore,cull,&a);
   assert(rt_prepared_cache::Replay(api,revisions,entry,cached.context,transform.data(),owner,variant,provider,ignore,cull,&b,result));
   assert(a==b&&ref.geometry==cached.geometry&&ref.instances==cached.instances&&ref.materials==cached.materials);
   for(auto off:{0x78,0x7c,0x88,0x8c,0x98,0x9c})assert(Read<std::uint32_t>(ref.context,off)==Read<std::uint32_t>(cached.context,off));++hits;
  };compare();
  auto reject=[&](unsigned char requestedIgnore,unsigned char requestedCull,const Api& chosen,unsigned capacity=128){Fixture f(capacity);std::array<unsigned char,0xe0> before{};std::memcpy(before.data(),f.context,before.size());std::uint32_t prefix=17;Result result;
   assert(!rt_prepared_cache::Replay(chosen,revisions,entry,f.context,transform.data(),owner,variant,provider,requestedIgnore,requestedCull,&prefix,result));
   assert(prefix==17&&!std::memcmp(before.data(),f.context,before.size())&&f.geometry==Fixture(capacity).geometry&&f.instances==Fixture(capacity).instances&&f.materials==Fixture(capacity).materials);++rejected;
  };
  reject(ignore,cull,api,0);reject(static_cast<unsigned char>(!ignore),cull,api);reject(ignore,static_cast<unsigned char>(!cull),api);
  auto unknown=api;unknown.staticMesh.table=nullptr;reject(ignore,cull,unknown);
  revisions.ProviderDestroyed(provider);revisions.ProviderCreated(provider);reject(ignore,cull,api);capture();compare();
  Write(provider,0x28,5);reject(ignore,cull,api);Write(provider,0x28,static_cast<std::int32_t>(test%5));
  otherGeometry=geometry;Write(provider,0x20,otherGeometry.data());reject(ignore,cull,api);Write(provider,0x20,geometry.data());
  if(test%5){
   // Deliberately omit notifications: exact input checks must still reject changes.
   geometry[0][17]^=1;reject(ignore,cull,api);capture();compare();
   Write(geometry[0].data(),0,std::int32_t(1));reject(ignore,cull,api);capture();compare();
   auto* liveTable=static_cast<void**>(rt_prepared_cache::MaterialTable(owner,variant));auto* liveMaterial=liveTable[1];
   Write(liveMaterial,0x204,Read<std::uint32_t>(liveMaterial,0x204)^1u);reject(ignore,cull,api);capture();compare();
   if(!ignore){Write(liveMaterial,0x1f0,Read<std::uint32_t>(liveMaterial,0x1f0)^0x80000000u);reject(ignore,cull,api);capture();compare();flags^=0x80000000u;reject(ignore,cull,api);capture();compare();Write(liveMaterial,0x1f8,&otherFlags);reject(ignore,cull,api);capture();compare();}
   revisions.MaterialChanged(Read<std::uint16_t>(liveMaterial,0x99e));reject(ignore,cull,api);capture();compare();
   Write(liveMaterial,0x99e,std::uint16_t(65535));reject(ignore,cull,api);capture();compare();
   std::swap(liveTable[0],liveTable[1]);reject(ignore,cull,api);capture();compare();
   Fixture corrupt(128);std::uint32_t prefix=0;auto result=Assemble(api,corrupt.context,transform.data(),owner,variant,provider,ignore,cull,&prefix);corrupt.geometry[0][11]^=1;
   assert(!rt_prepared_cache::Capture(api,revisions,entry,corrupt.context,owner,variant,provider,ignore,cull,result)&&!entry.valid);
  }
 }
 std::printf("PASS prepared cache: %u exact native comparisons, %u unchanged-output rejections; unnotified geometry/material changes, lifetime/ID reuse, live transforms/masks/BLAS, options, variants, capacities. Not an FPS benchmark.\n",hits,rejected);
}

// Execute the eight exact renderer leaf functions, not reimplemented mocks.
static void TestLiveProviders(AssembleFn original){
 using namespace rt_provider_bytes;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);
 void* tables[2][13]{};unsigned cursor=0;
 const auto install=[&](unsigned kind,unsigned slot,const auto& bytes){std::memcpy(code+cursor,bytes,sizeof(bytes));tables[kind][slot]=code+cursor;cursor+=64;};
 install(0,8,StaticAddress);install(0,9,StaticCount);install(0,10,StaticGeometry);install(0,12,StaticMask);
 install(1,8,SkinnedAddress);install(1,9,SkinnedCount);install(1,10,SkinnedGeometry);install(1,12,SkinnedMask);
 DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
 Api api{&MockAppend64,&MockAppend8};
 const auto binding=[&](unsigned k){return ProviderBinding{tables[k],tables[k][8],tables[k][9],tables[k][10],tables[k][12]};};
 api.staticMesh=binding(0);api.skinnedMesh=binding(1);
 TestPreparedCache(original,api);
 unsigned cases=0;
 for(unsigned kind=0;kind<2;++kind){
  alignas(16) unsigned char provider[0xb0]{},allocationA[0x20]{},allocationB[0x20]{},state[0x38]{};
  std::array<std::array<unsigned char,80>,64> storage{};
  alignas(8) unsigned char materialOwner[0x160]{},material[0x208]{};
  void* materials[]={material};std::uint32_t flags=0x80000000;Write(material,0x1f8,&flags);Write(materialOwner,0x130,materials);
  std::array<std::uint32_t,12> transform{};
  Write(provider,0,tables[kind]);
  Write(provider,kind?0x18:0x20,storage.data());Write(provider,0x68,allocationA);
  if(kind){Write(provider,0x88,state);Write(state,0x28,allocationB);}else Write(provider,0x88,allocationB);
  for(unsigned test=0;test<4096;++test){
   // Mutate the SAME provider on every call: addresses, count, mask and records.
   // Static signed state selects both BLAS allocations, including negative values.
   const auto count=static_cast<std::int32_t>(test%65);
   Write(provider,kind?0x30:0x28,count);Write(provider,kind?0x54:0x60,test*0x1020304u);
   Write(provider,0xac,test%5==0?std::int32_t(-2147483647-1):static_cast<std::int32_t>(test%7));
   const auto offset=(test&1)?std::int32_t(-2147483647-1):std::int32_t(2147483647);
   Write(provider,0x70,offset);Write(provider,0x90,std::int32_t(-1));Write(state,0x30,offset);
   Write(allocationA,0x10,std::uint64_t(test)*0xabcdef0123456789ull);Write(allocationB,0x10,~std::uint64_t(test)*0x123456789ull);
   for(auto& x:transform)x=test*0x1234567u+static_cast<std::uint32_t>(&x-transform.data());
   transform[4]=0x7fc01234u;
   Write(material,0x204,test%4);Write(material,0x1f0,(test&2)?0x80000000u:0u);
   for(int index=0;index<count;++index){auto* record=reinterpret_cast<unsigned char*>(storage.data())+index*(kind?80:64)+(kind?16:0);std::memset(record,static_cast<int>(test&255),64);Write(record,0,std::int32_t(0));}
   const auto capacity=(test&1)?128u:0u;
   Fixture reference(capacity),owned(capacity);std::uint32_t aPrefix=test*0x1234567u,bPrefix=aPrefix;
   original(reference.context,transform.data(),materialOwner,0,provider,static_cast<unsigned char>(test%2),0,&aPrefix);
   // An unrecognized vtable or redirected slot must retain virtual calls.
   auto chosen=api;unsigned expected=kind+1;
   if(test%7==0){chosen.staticMesh.table=nullptr;chosen.skinnedMesh.table=nullptr;expected=0;}
   if(test%11==0){if(kind)chosen.skinnedMesh.geometry=nullptr;else chosen.staticMesh.geometry=nullptr;expected=0;}
   const auto result=Assemble(chosen,owned.context,transform.data(),materialOwner,0,provider,static_cast<unsigned char>(test%2),0,&bPrefix);
   assert(result.providerKind==expected&&result.avoidedCalls==(expected?static_cast<unsigned>(count)+3u:0u));
   assert(aPrefix==bPrefix&&reference.geometry==owned.geometry&&reference.instances==owned.instances&&reference.materials==owned.materials);
   for(unsigned off:{0x78u,0x7cu,0x88u,0x8cu,0x98u,0x9cu})assert(Read<std::uint32_t>(reference.context,off)==Read<std::uint32_t>(owned.context,off));
   ++cases;
   if(kind==0){
    const auto batchCount=(test/9)%9,recordCount=test%9,start=test%8;
    Write(provider,0x28,static_cast<std::int32_t>(recordCount));
    for(unsigned j=0;j<recordCount;++j)Write(reinterpret_cast<unsigned char*>(storage.data())+j*64,0,std::int32_t(0));
    std::array<std::array<std::uint32_t,12>,8> transforms{};
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<12;++j)transforms[i][j]=test*0x9876543u+i*0x1234567u+j;
    transforms[3][4]=0x7fc01234u;
    const auto cap=((test%3)?128u:start)|((test%4)<<30);
    Fixture batchReference(cap),batchOwned(cap);
    for(unsigned off:{0x78u,0x88u,0x98u}){Write(batchReference.context,off,start);Write(batchOwned.context,off,start);}
    auto nativePrefix=test*0x13579bdu,ownedPrefix=nativePrefix;
    std::array<unsigned char,0xe0> before{};std::memcpy(before.data(),batchOwned.context,before.size());
    Result aggregate;
    const bool batched=TryAssembleBatch(chosen,batchOwned.context,transforms.data(),batchCount,materialOwner,provider,static_cast<unsigned char>(test%2),static_cast<unsigned char>((test/2)%2),&ownedPrefix,aggregate);
    const bool expectedBatch=batchCount>=2&&expected==1&&(test%3)!=0;
    assert(batched==expectedBatch);
    if(!batched){
     assert(!std::memcmp(before.data(),batchOwned.context,before.size())&&ownedPrefix==nativePrefix);
     assert(batchOwned.geometry==Fixture(cap).geometry&&batchOwned.materials==Fixture(cap).materials&&batchOwned.instances==Fixture(cap).instances);
    }else{assert(aggregate.reusedGeometry==(batchCount-1)*recordCount&&aggregate.geometry==batchCount*recordCount&&aggregate.fastAppends==batchCount*(recordCount+1)&&aggregate.materialFastAppends==batchCount*recordCount);}
    for(unsigned i=0;i<batchCount;++i){
     original(batchReference.context,transforms[i].data(),materialOwner,0,provider,static_cast<unsigned char>(test%2),static_cast<unsigned char>((test/2)%2),&nativePrefix);
     if(!batched)Assemble(chosen,batchOwned.context,transforms[i].data(),materialOwner,0,provider,static_cast<unsigned char>(test%2),static_cast<unsigned char>((test/2)%2),&ownedPrefix);
    }
    assert(nativePrefix==ownedPrefix&&batchReference.geometry==batchOwned.geometry&&batchReference.instances==batchOwned.instances&&batchReference.materials==batchOwned.materials);
    for(unsigned off:{0x78u,0x7cu,0x88u,0x8cu,0x98u,0x9cu})assert(Read<std::uint32_t>(batchReference.context,off)==Read<std::uint32_t>(batchOwned.context,off));
   }else{
    Result untouched;std::uint32_t prefix=7;std::array<std::uint32_t,24> rejectedTransforms{};
    assert(!TryAssembleBatch(api,owned.context,rejectedTransforms.data(),2,materialOwner,provider,0,0,&prefix,untouched));assert(prefix==7);
   }
  }
 }
 unsigned char invalidVector[16]{};Write(invalidVector,8,11u);Write(invalidVector,12,10u);assert(!FitsBatch(invalidVector,0));
 assert(VirtualFree(code,0,MEM_RELEASE));
 std::puts("PASS 4096 native repeated-assembly/batch comparisons; prefix wrap, transforms, flags, capacities/growth fallback, unknown/skinned rejection and unchanged outputs on rejection.");
 std::printf("PASS %u live-provider native-machine-code comparisons; static/skinned layouts, signed BLAS branches/offsets, changing live data, and virtual fallback. Not an FPS benchmark.\n",cases);
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
 TestLiveProviders(original);
 assert(VirtualFree(reinterpret_cast<void*>(original),0,MEM_RELEASE));
 std::printf("PASS 4096 exact native-machine-code comparisons; transforms/NaNs, masks, material overrides, geometry bytes, counters, provider call order and growth. Native append calls=%u owned growth calls=%u. Native material calls=%u owned material growth=%u. Not an FPS benchmark.\n",nativeAppends,ownedAppends,nativeMaterials,ownedMaterials);
}
