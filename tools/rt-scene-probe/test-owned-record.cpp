#include <windows.h>
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include "../../src/rt_owned_record.h"
#include "../../src/rt_owned_record_bytes.h"
#include "../../src/rt_owned_record_patch.h"
using namespace rt_owned_assembly;
struct Event {std::uint64_t op,a,b,c,d,e,f;bool operator==(const Event& v)const{return op==v.op&&a==v.a&&b==v.b&&c==v.c&&d==v.d&&e==v.e&&f==v.f;}};
static std::vector<Event> events;
static std::uint64_t Bits(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
static void* Token(std::uint64_t v){return reinterpret_cast<void*>(static_cast<std::uintptr_t>(v));}
static void* currentContext=nullptr;static void** currentCapture=nullptr;static void* switchedContext=nullptr;static unsigned mutation=0;
static void Add(unsigned op,const void* a=nullptr,std::uint64_t b=0,std::uint64_t c=0,std::uint64_t d=0,std::uint64_t e=0,std::uint64_t f=0){events.push_back({op,Bits(a),b,c,d,e,f});}
static void* Queue(std::int32_t type){Add(1,nullptr,static_cast<std::uint32_t>(type));return Token(0x100+type);}
static void Initialize(const rt_owned_record::ThreadInit* init){assert(init->flags==0);Add(2,init->inherited,Bits(init->queue));}
static void* Device(){Add(3);return Token(0x200);}
static void Flush(void* p){Add(4,p);}
static void ToUav(void* p,std::int32_t n){Add(5,p,static_cast<std::uint32_t>(n));}
static void ToConsume(void* p,std::int32_t n){Add(7,p,static_cast<std::uint32_t>(n));}
static void ToCopy(void* p){Add(9,p);}
static void Copy(void* dst,void* src){Add(10,dst,Bits(src));}
static void FromCopy(void* p){Add(11,p);}
static void Barrier(){Add(13);}
static void Build(std::uint32_t flags,void* input,std::int32_t count,void* dest,void* scratch,bool update){Add(14,input,flags,static_cast<std::uint32_t>(count),Bits(dest),Bits(scratch),update);}
static void* Finish(){Add(15);return Token(0xf00d);}
struct Provider {void** vtable;unsigned id;};
static void Mutate(){
 if(mutation==1){Write(currentContext,0x38,3u);mutation=0;}
 else if(mutation==2){*currentCapture=switchedContext;currentContext=switchedContext;mutation=0;}
}
static void Update(void* p,void* device){Add(6,p,Bits(device));Mutate();}
static void BuildBlas(void* p){Add(8,p);Mutate();}
static void Finalize(void* p){Add(12,p);Mutate();}
static unsigned phaseMask=0;static void Phase(unsigned n){
 assert(n<9);assert(phaseMask==((1u<<n)-1u));phaseMask|=1u<<n;
 // Verify boundaries surround the actual callbacks, including empty stages.
 bool dynamic=false;for(const auto& e:events)dynamic|=e.op==3;
 if(n>=1&&n<=5){const unsigned expected[]={0,3,4,5,6,7};assert(events.back().op==(dynamic?expected[n]:2u));}
}
using Native=void(*)(void*);
static rt_owned_record::Api* ownedApi=nullptr;static rt_owned_record::Globals ownedGlobals{};static rt_owned_record::Stats ownedStats;
static void OwnedHook(void* capture){ownedStats=rt_owned_record::Run(*ownedApi,ownedGlobals,capture);}

struct Oracle {unsigned char* code;Native run;rt_owned_record::Globals globals;};
static Oracle MakeOracle(){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);std::memcpy(code,nativeRecordOracle,sizeof(nativeRecordOracle));
 void* functions[]={reinterpret_cast<void*>(&Queue),reinterpret_cast<void*>(&Initialize),reinterpret_cast<void*>(&Device),reinterpret_cast<void*>(&ToUav),reinterpret_cast<void*>(&ToConsume),reinterpret_cast<void*>(&ToCopy),reinterpret_cast<void*>(&Copy),reinterpret_cast<void*>(&FromCopy),reinterpret_cast<void*>(&Barrier),reinterpret_cast<void*>(&Build),reinterpret_cast<void*>(&Finish)};
 std::memcpy(code+0x1000,functions,sizeof(functions));
 // Relocations generated from the verified original image; only imported calls,
 // globals and the direct Flush target change. Native body/branches stay intact.
 Write(code,0x12,std::int32_t(4608-22));
 Write(code,0x1b,std::int32_t(4096-31));
 Write(code,0x22,std::int32_t(4864-38));
 Write(code,0x3b,std::int32_t(4104-63));
 Write(code,0x68,std::int32_t(4112-108));
 Write(code,0x6f,std::int32_t(5120-115));
 Write(code,0x7b,std::int32_t(0x1500-127));
 Write(code,0x8b,std::int32_t(4120-143));
 Write(code,0xc5,std::int32_t(4128-201));
 Write(code,0x130,std::int32_t(5136-308));
 Write(code,0x140,std::int32_t(4136-324));
 Write(code,0x151,std::int32_t(4144-341));
 Write(code,0x15a,std::int32_t(4152-350));
 Write(code,0x1dc,std::int32_t(4160-480));
 Write(code,0x20c,std::int32_t(4168-528));
 Write(code,0x212,std::int32_t(4176-534));
 Write(code,0x219,std::int32_t(5152-541));
 code[0x1500]=0x48;code[0x1501]=0xb8;Write(code,0x1502,reinterpret_cast<void*>(&Flush));code[0x150a]=0xff;code[0x150b]=0xe0;
 unsigned char jump[5]{},stub[14]{};assert(rt_owned_record::TailPatch(reinterpret_cast<std::uintptr_t>(code+0x1604),reinterpret_cast<std::uintptr_t>(code+0x1700),reinterpret_cast<std::uintptr_t>(&OwnedHook),jump,stub));
 const unsigned char wrapper[]={0x48,0x83,0xc1,0x18};std::memcpy(code+0x1600,wrapper,4);std::memcpy(code+0x1604,jump,5);std::memcpy(code+0x1700,stub,14);
 assert(!rt_owned_record::TailPatch(0x10000,0x80020000,0,jump,stub));
 DWORD old=0;assert(VirtualProtect(code,0x1000,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,8192));
 // Stub page remains executable/writable in this test only (contains mock globals).
 assert(VirtualProtect(code+0x1000,0x1000,PAGE_EXECUTE_READWRITE,&old));
 return {code,reinterpret_cast<Native>(code),{code+0x1200,code+0x1300,reinterpret_cast<void* const*>(code+0x1400),reinterpret_cast<void* const*>(code+0x1410),reinterpret_cast<void**>(code+0x1420)}};
}
int main(){
 auto oracle=MakeOracle();rt_owned_record::Api api{Queue,Initialize,Device,Flush,ToUav,ToConsume,ToCopy,Copy,FromCopy,Barrier,Build,Finish,Phase};
 ownedApi=&api;ownedGlobals=oracle.globals;auto routed=reinterpret_cast<Native>(oracle.code+0x1600);
 void* table[8]{};table[5]=reinterpret_cast<void*>(&Update);table[6]=reinterpret_cast<void*>(&BuildBlas);table[7]=reinterpret_cast<void*>(&Finalize);
 std::array<Provider,4> providers{};std::array<void*,4> list{};for(unsigned i=0;i<4;++i){providers[i]={table,i};list[i]=&providers[i];}
 std::array<std::array<unsigned char,0xa28>,8> materials{};std::array<std::array<unsigned char,8>,8> handles{};std::array<void*,8> refs{};
 for(unsigned i=0;i<8;++i){refs[i]=materials[i].data();Write(refs[i],0xa20,handles[i].data());}
 unsigned char skinning[16]{};void* copyBuffers[]={Token(0x300),Token(0x400),Token(0x500)};
 Write(oracle.code,0x1400,skinning);Write(oracle.code,0x1410,copyBuffers);
 std::uint64_t totalEvents=0;
 for(unsigned test=0;test<4096;++test){
  unsigned char ctx[0xe0]{},other[0xe0]{};std::array<std::array<unsigned char,64>,8> geometry{},secondaryGeometry{};
  auto setup=[&]{std::memset(ctx,0,sizeof(ctx));std::memset(other,0,sizeof(other));for(auto& g:geometry)g.fill(0xa5);for(auto& g:secondaryGeometry)g.fill(0x5a);
   for(auto off:{0x20,0x40,0x50,0x60})Write(ctx,off,list.data());Write(ctx,0x30,Token(0x1234));
   Write(ctx,0x28,test%5);Write(ctx,0x38,test%4);Write(ctx,0x48,(test/5)%5);Write(ctx,0x58,(test/25)%5);Write(ctx,0x68,(test/125)%5);
   Write(ctx,0x78,test%17==0?-1:test%13==0?0:static_cast<std::int32_t>(test%100+1));Write(ctx,0x88,test%9);Write(ctx,0x80,geometry.data());Write(ctx,0x90,refs.data());
   Write(ctx,0xb8,Token(0x600));Write(ctx,0xc0,Token(0x700));Write(ctx,0xc8,Token(0x800));Write(ctx,0xd8,std::uint64_t(test));
   std::memcpy(other,ctx,sizeof(ctx));Write(other,0x80,secondaryGeometry.data());Write(other,0x78,999);Write(other,0xb8,Token(0x666));
   for(unsigned i=0;i<8;++i)Write(handles[i].data(),4,static_cast<std::uint16_t>(test*71+i));
   oracle.code[0x1200]=static_cast<unsigned char>(test%3);Write(oracle.code,0x1420,static_cast<void*>(nullptr));currentContext=ctx;switchedContext=other;mutation=(test/9)%3;
  };
  unsigned char job[0x20]{};auto* capture= reinterpret_cast<void**>(job+0x18);
  setup();Write(job,0x18,static_cast<void*>(ctx));currentCapture=capture;events.clear();oracle.run(capture);const auto expected=events;const auto expectedGeometry=geometry,expectedSecondary=secondaryGeometry;const auto expectedCleanup=*oracle.globals.cleanup;const auto expectedCapture=*capture;
  setup();Write(job,0x18,static_cast<void*>(ctx));events.clear();phaseMask=0;api.phase=(test%2)?Phase:nullptr;routed(job);const auto stats=ownedStats;
  assert(events==expected&&geometry==expectedGeometry&&secondaryGeometry==expectedSecondary&&*oracle.globals.cleanup==expectedCleanup&&*capture==expectedCapture&&phaseMask==((test%2)?511u:0u));
  assert(stats.instances==(test%17==0?-1:test%13==0?0:static_cast<std::int32_t>(test%100+1)));
  std::uint64_t updates=0,builds=0,finalizes=0,copies=0;for(auto e:events){updates+=e.op==6;builds+=e.op==8;finalizes+=e.op==12;copies+=e.op==10;}
  assert(stats.updates==updates&&stats.builds+stats.secondaryBuilds==builds&&stats.finalizes==finalizes&&stats.copied==(copies!=0));totalEvents+=events.size();
 }
 assert(VirtualFree(oracle.code,0,MEM_RELEASE));std::printf("PASS 4096 native AS-recording / production-tail comparisons; %llu exact callback events, queue/thread setup, transitions, dynamic callbacks, context replacement, current material IDs, parity copies, zero/negative instance count, TLAS flags and cleanup. Not an FPS benchmark.\n",static_cast<unsigned long long>(totalEvents));
}
