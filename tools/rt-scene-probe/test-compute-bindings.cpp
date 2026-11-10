#include <windows.h>
#include <array>
#include <vector>
#include <map>
#include <cassert>
#include <cstdio>
#include "../../src/rt_compute_bindings.h"
#include "../../src/rt_compute_bindings_bytes.h"
using namespace rt_compute_bindings;
static std::array<unsigned char,0x60000> tls{};
static std::array<unsigned char,0x200> testState{},nulls{};
static std::array<unsigned char,0x100> testTable{};
static unsigned char* code=nullptr;
static std::vector<std::array<std::uint64_t,4>> events;
static unsigned updates=0,many=0,ones=0;
static void* activeMaterial=nullptr;static bool mutateShaders=false;
static void* Retrieve(){events.push_back({3,0,0,0});return testState.data();}
static void SetShaders(void* device,const void* const* shaders,bool flag){assert(device==testState.data());events.push_back({4,flag,0,0});for(unsigned i=0;i<7;++i)events.push_back({5,i,reinterpret_cast<std::uintptr_t>(shaders[i]),0});if(mutateShaders)Write(activeMaterial,0x10,static_cast<void*>(nullptr));}
static void FinishUse(void* t,void* device){assert(device==testState.data());events.push_back({6,reinterpret_cast<std::uintptr_t>(t),0,0});}
static void NativeUse(void* t,void* device){if(Eligible(t)){reinterpret_cast<void(*)(void*)>(code)(t);FinishUse(t,device);}else events.push_back({7,reinterpret_cast<std::uintptr_t>(t),reinterpret_cast<std::uintptr_t>(device),0});}

static void UpdateConstants(void*){++updates;events.push_back({0,0,0,0});}
static void* ThreadData(){return tls.data();}
static void* State(void*){return testState.data();}
static void CbvCall(const Cbv* d,std::uintptr_t dest){events.push_back({1,dest,d->address,d->bytes});}
static void One(std::uintptr_t dest,std::uintptr_t source){++ones;events.push_back({2,dest,source,0});}
static void Many(unsigned n,const std::uintptr_t* d,const std::uintptr_t* s){++many;for(unsigned i=0;i<n;++i){for(unsigned j=0;j<n;++j)assert(d[i]!=s[j]);events.push_back({2,d[i],s[i],0});}}
static void NativeCbv(void*,const Cbv* d,std::uintptr_t dest){CbvCall(d,dest);}
static void NativeCopy(void*,unsigned n,std::uintptr_t dest,std::uintptr_t source,unsigned type){assert(type==0&&n==1);One(dest,source);}
struct Trace {Phase phase=Phase::Other;std::size_t cursor=0;unsigned marks=0;};
static void TraceMark(void* context,Phase phase){
 auto& trace=*static_cast<Trace*>(context);
 for(;trace.cursor<events.size();++trace.cursor){const auto event=events[trace.cursor][0];
  switch(trace.phase){
   case Phase::Retrieve:assert(event==3);break;
   case Phase::Shaders:assert(event==4||event==5);break;
   case Phase::Constants:assert(event==0);break;
   case Phase::Cbv:assert(event==1);break;
   case Phase::Copy:assert(event==2);break;
   case Phase::Uav:assert(event==6);break;
   case Phase::Native:assert(event==7);break;
   default:assert(false);break;
  }
 }
 trace.phase=phase;++trace.marks;
}
static void Rel(unsigned char* p,unsigned operand,unsigned next,unsigned target){const auto v=static_cast<std::int32_t>(target-next);Write(p,operand,v);}
static void Jump(unsigned char* p,void* target){const unsigned char h[]={0xff,0x25,0,0,0,0};std::memcpy(p,h,6);Write(p,6,target);}
int main(){
 code=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);
 std::memcpy(code,rt_compute_bytes::NativeBind,sizeof(rt_compute_bytes::NativeBind));
 #include "compute-oracle-relocations.h"
 // Replace GS static-TLS array load only in the test clone with a mock array.
 const unsigned char tlsLoad[]={0x48,0x8b,0x05,0,0,0,0,0x90,0x90};std::memcpy(code+0x3e,tlsLoad,9);Rel(code,0x41,0x45,0x1870);
 void* tlsArray[]={tls.data()};Write(code,0x1870,static_cast<void*>(tlsArray));Write(code,0x1810,0u);
 Jump(code+0x2000,reinterpret_cast<void*>(&UpdateConstants));code[0x2020]=0xc3;
 std::array<void*,25> vtable{};vtable[17]=reinterpret_cast<void*>(&NativeCbv);vtable[24]=reinterpret_cast<void*>(&NativeCopy);void** device=vtable.data();
 Write(code,0x1840,static_cast<void*>(&device));Write(code,0x1820,static_cast<void*>(testState.data()));Write(code,0x1850,static_cast<void*>(nulls.data()));Write(code,0x1830,32u);Write(code,0x1860,16u);
 std::memcpy(code+0x4000,rt_compute_bytes::NativeApply,sizeof(rt_compute_bytes::NativeApply));
 #include "compute-apply-relocations.h"
 Write(code,0x5808,reinterpret_cast<void*>(&Retrieve));Write(code,0x5810,reinterpret_cast<void*>(&SetShaders));Write(code,0x5818,reinterpret_cast<void*>(&NativeUse));
 DWORD old=0;assert(VirtualProtect(code+0x4000,0x1000,PAGE_EXECUTE_READ,&old));assert(VirtualProtect(code,0x1000,PAGE_EXECUTE_READ,&old));assert(VirtualProtect(code+0x2000,0x1000,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,0x3000));
 Api api{UpdateConstants,ThreadData,State,CbvCall,One,Many,code+0x3000,reinterpret_cast<std::uint32_t*>(code+0x1830),reinterpret_cast<void* const*>(code+0x1850)};
 std::array<unsigned char,40*4> cb{},direct{};std::array<unsigned char,24*36> buffers{};
 auto native=reinterpret_cast<void(*)(void*)>(code);
 auto applyNative=reinterpret_cast<void(*)(void*)>(code+0x4000);ShaderApi shaderApi{Retrieve,SetShaders,NativeUse,FinishUse};
 std::array<unsigned char,0x90> material{},otherTable{};
 unsigned batches=0,fallbackCopies=0;
 for(unsigned test=0;test<4096;++test){
  auto setup=[&]{tls.fill(0);testState.fill(0);nulls.fill(0);testTable.fill(0);cb.fill(0);direct.fill(0);buffers.fill(0);events.clear();updates=many=ones=0;
   Write(testState.data(),0x40,std::uintptr_t(0x100000));Write(nulls.data(),0x90,std::uintptr_t(0x900000));Write(tls.data(),0x20,static_cast<void*>(testState.data()));
   Write(testTable.data(),0,test%2?5u:6u);Write(testTable.data(),0x68,cb.data());Write(testTable.data(),0x70,4u);Write(testTable.data(),0x78,direct.data());Write(testTable.data(),0x80,4u);
   for(unsigned i=0;i<4;++i){auto* r=cb.data()+40*i;Write(r,0,static_cast<std::int32_t>(i+2));Write(r,0x20,static_cast<std::int32_t>(i));auto* a=tls.data()+0x456c0+i*40;Write(a,8,std::uint64_t(0x500000+test*512+i*4096));Write(a,0x20,i+3);Write(a,0x24,i+16);
    auto* d=direct.data()+40*i;Write(d,0,static_cast<std::int32_t>(i+2));Write(d,0x24,static_cast<std::int32_t>(i+40));Write(code,0x3000+(i+40)*16,static_cast<std::int32_t>(0x300+i*16));Cbv desc{0x700000ull+test*256+i*1024,256u+i*16,0};Write(tls.data(),0x4250+0x300+i*16,desc);
   }
   const auto n=2+test%35;Write(testTable.data(),0x38,buffers.data());Write(testTable.data(),0x40,n);
   for(unsigned i=0;i<n;++i){auto* r=buffers.data()+24*i;Write(r,0,static_cast<std::int32_t>(i));Write(r,4,static_cast<std::int32_t>(test%7==0?0:i));Write(r,8,1);Write(r,12,static_cast<std::int32_t>(test%3));Write(code,0x3000+i*16,static_cast<std::int32_t>(i*24));auto source=std::uintptr_t(test%5==0?0:0x800000+i*32);if(test%11==0)source=0x100000+5*32;Write(tls.data(),0x4250+i*24+(test%3)*8,source);}
  };
  setup();native(testTable.data());const auto expected=events;const auto expectedState=std::vector<unsigned char>(testState.begin(),testState.end());assert(updates==1);
  setup();const auto result=TryBind(api,testTable.data());assert(result.handled&&updates==1&&events==expected);assert(std::vector<unsigned char>(testState.begin(),testState.end())==expectedState);assert(result.descriptors==2+test%35);
  if(result.batched){assert(many==1&&ones==0);++batches;}else{assert(many==0&&ones==result.descriptors);fallbackCopies+=ones;}
  auto shaderSetup=[&]{setup();material.fill(0);otherTable.fill(0);Write(otherTable.data(),0,3u);Write(material.data(),1,static_cast<unsigned char>(test%2));
   for(unsigned i=0;i<7;++i){Write(material.data(),0x10+i*16,test%(i+2)?reinterpret_cast<void*>(std::uintptr_t(0x8000+i)):nullptr);Write(material.data(),0x18+i*16,i%2?otherTable.data():testTable.data());}activeMaterial=material.data();mutateShaders=test%3==0;
  };
  shaderSetup();applyNative(material.data());const auto expectedApply=events;const auto expectedApplyState=testState;const auto expectedUpdates=updates;
  shaderSetup();Trace trace;Probe probe{TraceMark,&trace};const bool traced=test%2==0;ApplyShader(api,shaderApi,material.data(),traced?&probe:nullptr);if(traced){TraceMark(&trace,Phase::Other);assert(trace.marks>=4&&trace.cursor==events.size());}assert(events==expectedApply&&testState==expectedApplyState&&updates==expectedUpdates);
 }
 for(auto off:{0x10,0x20}){Write(testTable.data(),static_cast<std::size_t>(off),1u);updates=0;assert(!TryBind(api,testTable.data()).handled&&updates==0);Write(testTable.data(),static_cast<std::size_t>(off),0u);}
 assert(batches>0&&fallbackCopies>0);assert(VirtualFree(code,0,MEM_RELEASE));
 std::printf("PASS 4096 native compute-binding cases; %u batched cases, %u overlap/duplicate fallback copies; current CBVs, null buffers, root constants, and unsupported texture/sampler rejection. Phase attribution matches native callback events with tracing enabled/disabled. Not an FPS benchmark.\n",batches,fallbackCopies);
}
