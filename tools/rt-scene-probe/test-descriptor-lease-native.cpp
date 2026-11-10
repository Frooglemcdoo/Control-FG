#include <windows.h>
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include "../../src/rt_owned_assembly.h"
#include "../../src/rt_descriptor_lease.h"
#include "../../src/rt_setup_probe.h"
#include "../../src/rt_descriptor_lease_bytes.h"
using rt_owned_assembly::Write;
using namespace rt_descriptor_lease;
static int manager,timeline,owner;
static std::array<unsigned char,0x80> testState;
static U64 cursor=0,nativeCalls=0;
static bool useLease=false;
static Lease lease;
static std::vector<std::array<U64,4>> events;
static Snapshot Current(){return {&manager,testState.data(),&timeline,1,1,32};}
static bool Valid(const Block& b,const Snapshot&){return b.owner==&owner;}
static void NativeAllocate(void*,U64 count,U64* gpu,U64* cpu,void** heap){++nativeCalls;*gpu=0x100000+cursor*32;*cpu=0x800000+cursor*32;*heap=&owner;cursor+=count;}
static void Reserve(void* arg,U64 count,U64* gpu,U64* cpu,void** heap){assert(count==49);events.push_back({0,count,0,0});if(useLease){Api api{NativeAllocate,Current,Valid};Block output;Take(api,lease,arg,count,output);*gpu=output.gpu;*cpu=output.cpu;*heap=output.owner;}else NativeAllocate(arg,count,gpu,cpu,heap);}
static void Samplers(void*,U64 count,U64* gpu,U64* cpu,void** heap){assert(count==0||count==4);*gpu=0x900000;*cpu=0xa00000;*heap=&manager;events.push_back({1,count,0,0});}
static void Copy(void*,unsigned count,U64 dst,U64 src,unsigned type){assert(count==49&&type==0&&src==0xb00000);events.push_back({2,count,dst,src});}
static void InitializeTable(U64 destination){Copy(nullptr,49,destination,0xb00000,0);}
struct Trace {std::size_t cursor=0;unsigned previous=3,marks=0;};
static void TraceMark(void* context,unsigned phase){auto& trace=*static_cast<Trace*>(context);for(;trace.cursor<events.size();++trace.cursor)assert(events[trace.cursor][0]==trace.previous);trace.previous=phase;++trace.marks;}
static void Rel(unsigned char* code,unsigned operand,unsigned next,unsigned target){Write(code,operand,static_cast<std::int32_t>(target-next));}
static void Jump(unsigned char* code,void* target){const unsigned char stub[]={0xff,0x25,0,0,0,0};std::memcpy(code,stub,6);Write(code,6,target);}
int main(){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);std::memcpy(code,rt_lease_bytes::Prepare,sizeof(rt_lease_bytes::Prepare));
 #include "descriptor-lease-relocations.h"
 Jump(code+0x2000,reinterpret_cast<void*>(&Reserve));Jump(code+0x2020,reinterpret_cast<void*>(&Samplers));
 std::array<void*,25> vtable{};vtable[24]=reinterpret_cast<void*>(&Copy);auto* device=vtable.data();std::array<unsigned char,0xb0> nulls{};Write(nulls.data(),0xa8,U64(0xb00000));Write(code,0x1000,static_cast<void*>(&device));Write(code,0x1010,static_cast<void*>(nulls.data()));
 DWORD old=0;assert(VirtualProtect(code,0x1000,PAGE_EXECUTE_READ,&old));assert(VirtualProtect(code+0x2000,0x1000,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,0x3000));auto prepare=reinterpret_cast<void(*)(void*,bool,bool)>(code);
 for(unsigned test=0;test<4096;++test){const auto count=1+test%32;std::vector<std::array<unsigned char,0x80>> expected;
  useLease=false;cursor=nativeCalls=0;events.clear();
  for(unsigned i=0;i<count;++i){testState.fill(0);prepare(testState.data(),(test+i)%2!=0,(test+i)%3!=0);expected.push_back(testState);}const auto expectedEvents=events;assert(nativeCalls==count);
  useLease=true;cursor=nativeCalls=0;lease={};events.clear();
  for(unsigned i=0;i<count;++i){testState.fill(0);prepare(testState.data(),(test+i)%2!=0,(test+i)%3!=0);assert(testState==expected[i]);}assert(events==expectedEvents&&nativeCalls==(count+15)/16);
  useLease=false;cursor=nativeCalls=0;events.clear();rt_setup_probe::Api setupApi{Reserve,Samplers,InitializeTable};Trace trace;
  for(unsigned i=0;i<count;++i){testState.fill(0);rt_setup_probe::Run(setupApi,testState.data(),(test+i)%2!=0,(test+i)%3!=0,test%2?nullptr:TraceMark,&trace);assert(testState==expected[i]);}
  assert(events==expectedEvents&&nativeCalls==count);if(test%2==0)assert(trace.marks==count*4&&trace.cursor==events.size());

 }
 assert(VirtualFree(code,0,MEM_RELEASE));std::puts("PASS 4096 native compute preparation cases: five-argument reservation ABI, unique GPU/CPU table addresses, original sampler reservations and all 49-descriptor initialization calls preserved; sampled setup phases match original native events.");
}
