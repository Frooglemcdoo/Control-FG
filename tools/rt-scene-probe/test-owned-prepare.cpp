#include <windows.h>
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <limits>
#include "../../src/rt_owned_prepare.h"
#include "../../src/rt_owned_prepare_bytes.h"
#include "../../src/rt_owned_prepare_patch.h"
using namespace rt_owned_prepare;
using rt_owned_assembly::Write;
using RunFn=void(*)(void*);
static std::vector<std::uint64_t> events;
static unsigned assembled=0,updated=0,marked=0;
static unsigned char ReadyMock(void* p){events.push_back(0x10000000u+Read<unsigned>(p,8));return Read<unsigned char>(p,12);}
static std::uint32_t Count(void* p){auto* entity=Read<void*>(p,0x60);return entity?Read<std::uint32_t>(entity,0x10):0;}
static void Mark(void* p){events.push_back(0x30000000u+Read<unsigned>(p,0x10));++marked;}
static void Update(void* p,std::uint64_t frame,void* a,void* b,void* c,void* d){events.push_back(0x40000000u+Read<unsigned>(p,8));events.push_back(frame);assert(static_cast<unsigned char*>(b)==static_cast<unsigned char*>(a)+16&&static_cast<unsigned char*>(c)==static_cast<unsigned char*>(a)+48&&static_cast<unsigned char*>(d)==static_cast<unsigned char*>(a)+64);Write(a,8,Read<unsigned>(a,8)+Read<unsigned>(p,16));++updated;}
static void AssembleMock(void*,const void* transform,void* owner,std::int32_t variant,void* provider,unsigned char alpha,unsigned char cull,std::uint32_t* prefix){
 events.push_back(0x50000000u+Read<unsigned>(provider,8));events.push_back(reinterpret_cast<std::uintptr_t>(owner));events.push_back(static_cast<unsigned>(variant));events.push_back(alpha);events.push_back(cull);events.push_back(*prefix);
 for(unsigned i=0;i<12;++i)events.push_back(Read<unsigned>(transform,i*4));
 *prefix+=Read<unsigned>(provider,20);++assembled;
}
static std::uint32_t BatchMock(void* context,const void* transforms,std::uint32_t count,void* owner,void* provider,unsigned char alpha,unsigned char cull,std::uint32_t* prefix){
 assert(count>1);
 for(std::uint32_t i=0;i<count;++i)AssembleMock(context,static_cast<const unsigned char*>(transforms)+std::size_t(i)*48,owner,0,provider,alpha,cull,prefix);
 return count-1;
}
static Options liveOptions;
static Stats liveStats;
static void OwnedHook(void* capture){liveStats=Run({&Count,&Mark,&AssembleMock,&BatchMock},Read<void*>(capture),Read<unsigned char>(capture,0x18),liveOptions);}
struct OracleCode {unsigned char* code;RunFn run;};
static OracleCode Oracle(bool owned){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);
 // Save nonvolatiles and recreate the original loop's RSP/RBP relationship.
 const unsigned char prologue[]={0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x88,0x02,0,0,0x48,0x8d,0xac,0x24,0,1,0,0,0x49,0x89,0xcf,0x49,0x8b,0x0f};
 std::memcpy(code,prologue,sizeof(prologue));const unsigned body=sizeof(prologue);std::memcpy(code+body,nativePrepareOracle,sizeof(nativePrepareOracle));
 const unsigned char epilogue[]={0x49,0x89,0x4f,0x08,0x48,0x81,0xc4,0x88,0x02,0,0,0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41,0x5c,0x5f,0x5e,0x5d,0x5b,0xc3};std::memcpy(code+body+sizeof(nativePrepareOracle),epilogue,sizeof(epilogue));
 auto stub=[&](unsigned at,void* target){code[at]=0x48;code[at+1]=0xb8;std::memcpy(code+at+2,&target,8);code[at+10]=0xff;code[at+11]=0xe0;};
 // Exact leaf 1D18A0 preserves RDX; the native loop relies on that fact
 // when an empty instanced object precedes a mesh. A generic ABI mock cannot.
 const unsigned char countBody[]={0x48,0x8b,0x41,0x60,0x48,0x85,0xc0,0x74,0x04,0x8b,0x40,0x10,0xc3,0xc3};std::memcpy(code+0x600,countBody,sizeof(countBody));stub(0x620,reinterpret_cast<void*>(&Mark));stub(0x640,reinterpret_cast<void*>(&AssembleMock));
 auto call=[&](unsigned rva,unsigned target){const auto offset=body+rva-0xa8459;assert(code[offset]==0xe8);Write(code,offset+1,static_cast<std::int32_t>(target-offset-5));};
 for(unsigned site:{0xa84ceu,0xa8598u})call(site,0x600);
 call(0xa8793,0x620);
 for(unsigned site:{0xa8578u,0xa8713u,0xa87c2u})call(site,0x640);
 for(const auto& r:prepareRelocs){unsigned target=r.target==0x90e480?0x800:r.target==0x90e300?0x801:r.target==0x90e3c0?0x802:0x804;Write(code,body+r.offset+r.displacement,static_cast<std::int32_t>(target-body-r.next));}
 if(owned){unsigned char relay[27]{},patch[14]{};assert(!std::memcmp(nativePrepareOracle,kLoopEntry,sizeof(kLoopEntry)));assert(BuildLoopPatch(reinterpret_cast<std::uintptr_t>(code+body),reinterpret_cast<std::uintptr_t>(code+0xa00),reinterpret_cast<std::uintptr_t>(&OwnedHook),relay,patch));std::memcpy(code+body,patch,sizeof(patch));std::memcpy(code+0xa00,relay,sizeof(relay));}
 DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,4096));return {code,reinterpret_cast<RunFn>(code)};
}
struct Fixture {
 unsigned char context[0xe0]{},capture[32]{},owner[0x250]{},entity[0x40]{},subKind[0x11f0]{},ownerLink[0xa8]{};
 std::array<std::array<unsigned char,0x280>,8> objects{};
 std::array<std::array<unsigned char,0x3e0>,3> subs{};
 std::array<std::array<std::uint32_t,12>,4> transforms{};
 std::array<std::array<unsigned char,32>,12> providers{};
 void* vtable[5]{};
 std::array<void*,8> selected{};
 std::array<std::array<void*,8>,6> lods{};
 std::array<void*,6> primary{},alternate{};
 std::array<void*,8> dynamicA{},dynamicB{};
 Fixture(unsigned test){
  unsigned seed=test+1;auto random=[&]{seed=seed*1664525u+1013904223u;return seed;};
  vtable[1]=reinterpret_cast<void*>(&ReadyMock);vtable[4]=reinterpret_cast<void*>(&Update);
  for(unsigned i=0;i<12;++i){auto* p=providers[i].data();Write(p,0,vtable);Write(p,8,i);Write(p,12,static_cast<unsigned char>((test+i)%3!=0));Write(p,16,(test+i)%2);Write(p,20,i%5);}
  for(unsigned i=0;i<6;++i)for(unsigned j=0;j<8;++j)lods[i][j]=(test+i+j)%7==0?nullptr:providers[(i+j)%12].data();
  for(unsigned i=0;i<3;++i){primary[i*2]=lods[i].data();alternate[i*2]=lods[i+3].data();}
  Write(owner,0x230,primary.data());Write(owner,0x240,alternate.data());Write(owner,0x204,1u+test%8);
  for(unsigned i=0;i<8;++i){dynamicA[i]=(test+i)%5?providers[i].data():nullptr;dynamicB[i]=(test+i)%4?providers[i+4].data():nullptr;}
  for(auto& t:transforms)for(auto& v:t)v=random();
  Write(entity,0x10,test%4);
  Write(subKind,0x11ec,test%3==0?2u:3u);Write(ownerLink,0xa0,static_cast<void*>(owner));
  for(unsigned i=0;i<3;++i){auto* s=subs[i].data();Write(s,0x3d0,static_cast<void*>(subKind));Write(s,0x3d8,static_cast<void*>(ownerLink));Write(s,0x360,transforms.data());Write(s,0x368,(test+i)%5);Write(entity,0x18+i*8,static_cast<void*>(s));}
  for(unsigned i=0;i<8;++i){auto* o=objects[i].data();selected[i]=o;Write(o,8,i%3==0?0x800u:i%3==1?0x100u:0x400u);Write(o,0x10,i);Write(o,0x18,test%4);Write(o,0x1c,test%2);Write(o,0x60,i%3==0?static_cast<void*>(entity):static_cast<void*>(owner));Write(o,0x140,std::int32_t((test+i)%3));Write(o,0x258,dynamicA.data());Write(o,0x268,dynamicB.data());
   for(unsigned j=0;j<8;++j)Write(o,0x148+j*4,float(int(random()%7)-3)*0.001f);
   if(test%9==0)Write(o,0x148+(test%8)*4,std::numeric_limits<float>::quiet_NaN());
   for(unsigned j=0;j<12;++j)Write(o,0x70+j*4,random());
  }
  Write(context,0x10,selected.data());Write(context,0x18,test%9);Write(context,0x28,test%4);Write(context,0xd8,std::uint64_t(test)*123456789u);
  Write(capture,0,static_cast<void*>(context));Write(capture,0x18,static_cast<unsigned char>(test%2));
 }
};
int main(){auto oracle=Oracle(false);auto ownedOracle=Oracle(true);unsigned total=0,dynamic=0;
 for(unsigned test=0;test<4096;++test){Fixture f(test);const auto saved=f.objects;const auto before=Read<unsigned>(f.context,0x28);Options options;options.forceLodZero=static_cast<unsigned char>(test%5==0);options.ignoreAlpha=static_cast<unsigned char>(test%2);options.disableCull=static_cast<unsigned char>((test/2)%2);
  DWORD old=0;assert(VirtualProtect(oracle.code,4096,PAGE_EXECUTE_READWRITE,&old));Write(oracle.code,0x800,options.forceLodZero);Write(oracle.code,0x801,options.ignoreAlpha);Write(oracle.code,0x802,options.disableCull);Write(oracle.code,0x804,options.threshold);assert(VirtualProtect(oracle.code,4096,PAGE_EXECUTE_READ,&old));
  events.clear();assembled=updated=marked=0;oracle.run(f.capture);const auto expected=events;const auto count=Read<unsigned>(f.context,0x28);const auto expectedObjects=f.objects;const auto a=assembled,u=updated,m=marked;
  f.objects=saved;Write(f.context,0x28,before);events.clear();assembled=updated=marked=0;liveOptions=options;Write(f.capture,8,static_cast<void*>(nullptr));ownedOracle.run(f.capture);const auto stats=liveStats;assert(Read<void*>(f.capture,8)==f.context);
  assert(events==expected&&Read<unsigned>(f.context,0x28)==count&&f.objects==expectedObjects);assert(stats.instances==a&&stats.dynamicUpdates==u&&stats.marked==m);total+=a;dynamic+=u;
 }
 assert(total&&dynamic);assert(VirtualFree(ownedOracle.code,0,MEM_RELEASE));assert(VirtualFree(oracle.code,0,MEM_RELEASE));std::printf("PASS 4096 native outer-loop comparisons: LOD/NaN decisions, provider readiness/order, alternate paths, dynamic updates, instancing, transforms and prefix progression. Assemblies=%u dynamic updates=%u. Not an FPS benchmark.\n",total,dynamic);
}
