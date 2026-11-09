#include <windows.h>
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>
#include "../../src/rt_owned_selection.h"
#include "native-selection-oracle.h"
using namespace rt_owned_selection;
using QueryFn=void(*)(void*,void*,std::uint32_t,std::uint32_t,std::uint32_t,void*);
static std::vector<std::uint64_t> events;
static unsigned allocs=0,frees=0;
static void* Alloc(std::size_t bytes,unsigned alignment){assert(bytes==0x4004&&alignment==8);++allocs;return std::malloc(bytes);}
static unsigned char Special(void*){return 0;}
static void Free(void* p){++frees;std::free(p);}
static void Append(void* out,const void* pointer){auto count=Read<unsigned>(out,8);assert(count<1024);Write(Read<void*>(out),std::size_t(count)*8,Read<void*>(pointer));Write(out,8,count+1);}
static void Refresh(void*,int layer,void* node){events.push_back(0x10000000u+unsigned(layer)*1000+Read<unsigned>(node,0x3c));Write(node,0x39,static_cast<unsigned char>(1));}
static unsigned char Test(void* object,void*,std::uint32_t mask){events.push_back(0x20000000u+Read<unsigned>(object,0x10));events.push_back(mask);return static_cast<unsigned char>(Read<unsigned>(object,0x10)%3!=0);}
template<unsigned N>static void PartialFilter(void* volume,std::uint32_t mask,std::uint32_t types,std::uint32_t flags,std::uint32_t required,void* out,void* node,const void* packed){
 unsigned count=0;for(unsigned i=0;i<24;++i)if(mask&(1u<<i)){assert(!std::memcmp(static_cast<unsigned char*>(volume)+i*16,static_cast<const unsigned char*>(packed)+count*16,16));++count;}assert(count==N);
 events.push_back(0x30000000u+Read<unsigned>(node,0x3c));events.push_back(mask);events.push_back(types);events.push_back(flags);events.push_back(required);
 // These callbacks are deliberately retained native in production. Compare
 // traversal's callback choice, arguments, compact planes and invocation order.
 auto* records=Read<unsigned char*>(node,0x20);for(unsigned i=0;i<Read<unsigned>(node,0x28);++i){auto* object=Read<void*>(records+i*48,0x20);Append(out,&object);}
}
template<std::size_t... I>static Api MakeApi(std::index_sequence<I...>){return {&Refresh,&Append,{&PartialFilter<static_cast<unsigned>(I)>...}};}
static QueryFn Oracle(const Api& api){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);std::memcpy(code,nativeSelectionOracle,sizeof(nativeSelectionOracle));
 auto stub=[&](unsigned at,void* target){code[at]=0x48;code[at+1]=0xb8;std::memcpy(code+at+2,&target,8);code[at+10]=0xff;code[at+11]=0xe0;};
 for(unsigned n=0;n<25;++n)stub(0x600+n*16,reinterpret_cast<void*>(api.partial[n]));
 stub(0x800,reinterpret_cast<void*>(&Refresh));stub(0x820,reinterpret_cast<void*>(&Append));code[0x840]=0xc3;
 Write(code,0x900,&Alloc);Write(code,0x908,&Special);Write(code,0x910,&Free);
 for(unsigned i=0;i<4;++i){Write(code,0x920+i*4,0x7fffffffu);Write(code,0x930+i*4,0x80000000u);}
 for(const auto& r:selectionRelocs){unsigned target=0;
  if(r.target==0x7ec010)target=0x940;
  else if(r.target==0x5e0160)target=0x900;
  else if(r.target==0x5e0428)target=0x908;
  else if(r.target==0x5e0158)target=0x910;
  else if(r.target==0x680b10)target=0x920;
  else if(r.target==0x680b40)target=0x930;
  else if(r.target==0x165f70)target=0x600;
  else {assert(r.target>=0x166030&&r.target<=0x167e60);target=0x610+(r.target-0x166030)/0x150*16;}
  Write(code,r.offset+r.displacement,static_cast<std::int32_t>(target-r.next));
 }
 auto call=[&](unsigned offset,unsigned target){assert(code[offset]==0xe8);Write(code,offset+1,static_cast<std::int32_t>(target-offset-5));};
 call(0x1657da-0x1655c0,0x800);call(0x165938-0x1655c0,0x820);call(0x165aad-0x1655c0,0x840);
 std::memcpy(code+0xb00,nativeQueryOracle,sizeof(nativeQueryOracle));call(0xb00+0x55,0);call(0xb00+0x75,0);
 DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,4096));return reinterpret_cast<QueryFn>(code+0xb00);
}
struct Fixture {
 alignas(16) unsigned char grid[0x340]{},volume[0x190]{},out[16]{};
 std::array<std::array<unsigned char,64>,42> nodes{};
 std::array<std::array<unsigned char,48>,126> records{};
 std::array<std::array<unsigned char,32>,126> objects{};
 std::array<std::array<void*,16>,6> tables{};
 std::array<void*,1024> selected{};
 std::array<unsigned char,42> dirty{};
 void* vtable[6]{};
 Fixture(unsigned seed){
  vtable[5]=reinterpret_cast<void*>(&Test);Write(grid,0x64,3u);Write(out,0,selected.data());Write(out,12,1024u);
  auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
  for(unsigned layer=0;layer<2;++layer)for(unsigned level=0;level<3;++level){auto& table=tables[layer*3+level];Write(grid,0x68+layer*256+level*8,table.data());const unsigned width=1u<<level,base=layer*21+(level==0?0:level==1?1:5);
   for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){const auto id=base+y*width+x;auto* node=nodes[id].data();table[y*width+x]=node;
    Write(node,0x34,static_cast<std::uint16_t>(x));Write(node,0x36,static_cast<std::uint16_t>(y));Write(node,0x38,static_cast<unsigned char>(level));dirty[id]=static_cast<unsigned char>(random()%2);Write(node,0x39,dirty[id]);Write(node,0x3c,id);
    Write(node,0x30,random()%5?1u:0u);Write(node,0x28,3u);Write(node,0x20,records[id*3].data());
    for(unsigned axis=0;axis<4;++axis){Write(node,axis*4,float(int(random()%21)-10));Write(node,16+axis*4,float(random()%8));}
    for(unsigned k=0;k<3;++k){const auto objectId=id*3+k;auto* object=objects[objectId].data();auto* record=records[objectId].data();Write(object,0,vtable);Write(object,8,1u<<(random()%4));Write(object,0xc,random()%16);Write(object,0x10,objectId);Write(record,0x20,object);Write(record,0x28,1u<<(random()%4));}
   }
  }
  Write(volume,0x180,int(seed%25));for(unsigned i=0;i<96;++i)Write(volume,i*4,float(int(random()%21)-10)/3.f);
 }
 void Reset(){for(unsigned i=0;i<42;++i)Write(nodes[i].data(),0x39,dirty[i]);Write(out,8,0u);selected={};events.clear();}
};
int main(){const auto api=MakeApi(std::make_index_sequence<25>{});const auto original=Oracle(api);std::uint64_t nodes=0,partial=0,rejected=0,accepted=0,refresh=0;
 for(unsigned test=0;test<4096;++test){Fixture f(test+1);events.clear();Write(f.volume,0x180,int(test%25));
  std::array<std::vector<void*>,32> chain{};
  if(test%17==0){Write(f.grid,0x64,16u);for(unsigned layer=0;layer<2;++layer)for(unsigned depth=0;depth<16;++depth){chain[layer*16+depth].resize((1u<<depth)+2);auto* node=f.nodes[layer*16+depth].data();chain[layer*16+depth][0]=node;Write(f.grid,0x68+layer*256+depth*8,chain[layer*16+depth].data());Write(node,0x34,std::uint16_t(0));Write(node,0x36,std::uint16_t(0));Write(node,0x38,static_cast<unsigned char>(depth));Write(node,0x30,1u);}}
  if(test%7==0)for(unsigned i=0;i<24;++i){Write(f.volume,i*16,1.f);Write(f.volume,i*16+4,0.f);Write(f.volume,i*16+8,0.f);Write(f.volume,i*16+12,100.f);}
  if(test%11==0)Write(f.volume,0,std::numeric_limits<float>::quiet_NaN());
  if(test%13==0)Write(f.volume,12,std::numeric_limits<float>::infinity());
  if(test%19==0)Write(f.volume,4,0x80000000u);
  const unsigned flags=(test%2?8u:12u)|(test%3==0?1u<<17:0),required=(test%5==0?8u:0)|(test%9==0?1u<<17:0),types=1u<<(test%4);
  assert(Supported(f.grid,f.volume));
  const auto planeCount=Read<int>(f.volume,0x180);Write(f.volume,0x180,-1);assert(!Supported(f.grid,f.volume));Write(f.volume,0x180,25);assert(!Supported(f.grid,f.volume));Write(f.volume,0x180,planeCount);
  const auto levels=Read<unsigned>(f.grid,0x64);Write(f.grid,0x64,17u);assert(!Supported(f.grid,f.volume));Write(f.grid,0x64,levels);
  original(f.grid,f.volume,types,flags,required,f.out);const auto expected=f.selected;const auto count=Read<unsigned>(f.out,8);const auto expectedEvents=events;std::array<unsigned char,42> dirty;for(unsigned i=0;i<42;++i)dirty[i]=Read<unsigned char>(f.nodes[i].data(),0x39);
  f.Reset();const auto stats=Query(api,f.grid,f.volume,types,flags,required,f.out);
  assert(f.selected==expected&&Read<unsigned>(f.out,8)==count&&events==expectedEvents);for(unsigned i=0;i<42;++i)assert(dirty[i]==Read<unsigned char>(f.nodes[i].data(),0x39));
  nodes+=stats.nodes;partial+=stats.partial;rejected+=stats.rejected;accepted+=stats.accepted;refresh+=stats.refreshes;
 }
 assert(nodes&&partial&&rejected&&accepted&&refresh&&allocs==frees);assert(VirtualFree(reinterpret_cast<unsigned char*>(original)-0xb00,0,MEM_RELEASE));
 std::printf("PASS 4096 native selection comparisons: output order, masks, partial callbacks, object tests, dirty updates, empty nodes, both layers, NaNs/infinity. Native temporary allocations=%u; owned traversal heap allocations=0. Not an FPS benchmark.\n",allocs);
}
