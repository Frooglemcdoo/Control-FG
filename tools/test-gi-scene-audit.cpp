#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <initializer_list>
static HMODULE verifiedRenderer=nullptr,verifiedD3d=nullptr;
static constexpr char kRendererHash[]="fixture",kD3dHash[]="fixture";
static bool HashMatches(HMODULE,const char*){return true;}
static void Log(const char*,...) noexcept {}
static void* AllocateExecutableRelayNear(const void* origin) noexcept {
 SYSTEM_INFO si{};GetSystemInfo(&si);const auto gran=si.dwAllocationGranularity;
 const auto start=reinterpret_cast<std::uintptr_t>(origin)&~(std::uintptr_t(gran)-1);
 for(std::uintptr_t offset=gran;offset<0x10000000;offset+=gran)
  if(auto* p=VirtualAlloc(reinterpret_cast<void*>(start+offset),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return p;
 return nullptr;
}
static bool Rel32Fits(const unsigned char* from,const void* to) noexcept {
 const auto d=reinterpret_cast<std::intptr_t>(to)-reinterpret_cast<std::intptr_t>(from);return d>=INT32_MIN&&d<=INT32_MAX;
}
#include "../src/gi_scene_audit.h"
static void Require(bool b){if(!b)throw std::runtime_error("scene audit contract failed");}
static unsigned forwarded=0;
static void Native(void*){++forwarded;}
static void Throw(void*){throw std::runtime_error("native exception");}
int main(){try{
 namespace s=gi_scene_audit;namespace a=gi_phase_audit;
 SetEnvironmentVariableA("CONTROLTOH_GI_SCENE_CHECK","1");SetEnvironmentVariableA("CONTROLTOH_GI_PHASE_AUDIT","1");
 a::frameReader=[]() noexcept -> unsigned long long{return 100;};
 // Build identity mismatch rejects before touching any callsite.
 Require(!s::Install(nullptr,nullptr)&&a::events[0].kind==25&&a::events[0].data[1]==0);
 s::original[0]=Native;s::original[1]=Native;s::original[2]=Native;s::original[3]=Native;s::installed=true;
 // Writer already in flight when the first GI phase opens is represented.
 {s::Writer w(0,nullptr);Require(s::active[0]==1&&s::serial==1);Require(a::PhaseWindow(100));}
 Require(s::active[0]==0&&s::serial==2);
 s::Material(nullptr);Require(forwarded==1&&s::active[0]==0);
 s::original[0]=Throw;try{s::Material(nullptr);Require(false);}catch(const std::runtime_error&){}
 Require(s::active[0]==0&&s::serial==6);
 s::Join(reinterpret_cast<void*>(1));Require(s::joined&&s::sceneFrame==100&&forwarded==2);
 s::Snapshot sample{};s::Read(&sample,100);Require(sample.fault!=0); // inaccessible scene safely marked
 s::scene=nullptr;s::renderer=nullptr;s::Observe(1,100,1);
 bool fault=false;for(unsigned i=0;i<a::next;++i)if(a::events[i].kind==22)fault=a::events[i].data[9]!=0;Require(fault);
 // Concurrent writers leave balanced counts; publication serial counts both boundaries.
 auto before=s::serial.load();std::thread t1([]{for(unsigned i=0;i<100;++i){s::Writer w(1,nullptr);}});
 std::thread t2([]{for(unsigned i=0;i<100;++i){s::Writer w(2,nullptr);}});t1.join();t2.join();
 Require(s::active[1]==0&&s::active[2]==0&&s::serial==before+400);
 a::stopped=true;before=s::serial;s::original[0]=Native;s::Material(nullptr);Require(s::serial==before&&forwarded==3);
 // E9 thunk and E8 call preserve original arguments through the real relay helper.
 auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Require(page!=nullptr);
 for(unsigned char opcode:{static_cast<unsigned char>(0xe8),static_cast<unsigned char>(0xe9)}){
  unsigned char* site=page;
  if(opcode==0xe8){const unsigned char sub[]={0x48,0x83,0xec,0x28};memcpy(page,sub,4);site=page+4;}
  site[0]=opcode;const int delta=static_cast<int>(page+128-(site+5));memcpy(site+1,&delta,4);page[128]=0xc3;
  const unsigned char tail[]={0x48,0x83,0xc4,0x28,0xc3};if(opcode==0xe8)memcpy(site+5,tail,5);
  NativeCpuCallPatch patch{};Require(NativeCpuPrepareCallPatch(site,page+128,reinterpret_cast<void*>(&Native),&patch,opcode));
  Require(NativeCpuExchangeCall(&patch,true)&&patch.writeHealthy);reinterpret_cast<s::Native>(page)(nullptr);
  Require(NativeCpuExchangeCall(&patch,false)&&patch.writeHealthy);Require(site[0]==opcode);
 }
 Require(forwarded==5);VirtualFree(page,0,MEM_RELEASE);
 puts("PASS scene audit: identity rejection, in-flight writer, exception unwind, concurrent publication, SEH fault, bounded stop, E8/E9 relay forwarding");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
