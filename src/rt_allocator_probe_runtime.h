#pragma once
#include "rt_allocator_probe.h"
#include "rt_allocator_probe_patch.h"
#include "rt_descriptor_lease_bytes.h"
namespace RTAllocatorProbe {
inline rt_allocator_probe::Api api{};inline thread_local rt_allocator_probe::Sample* active=nullptr;
inline bool ready=false;inline UINT64 frequency=0;inline SRWLOCK statsLock=SRWLOCK_INIT;
struct Totals {UINT64 calls=0,wait=0,hold=0,release=0,growth=0;};inline Totals totals[3]{};inline UINT64 unobserved=0;inline rt_allocator_probe::Sample peakWait{},peakHold{};
inline std::uint64_t Clock(){LARGE_INTEGER now{};QueryPerformanceCounter(&now);return static_cast<std::uint64_t>(now.QuadPart);}
inline std::uint32_t Error(){return GetLastError();}inline void Restore(std::uint32_t error){SetLastError(error);}
inline rt_allocator_probe::Snapshot Capture(void* pointer){
 using rt_owned_assembly::Read;auto* manager=static_cast<unsigned char*>(pointer)-0x18;auto* heap=Read<void*>(manager,0x60);
 return {heap,Read<std::uint64_t>(manager,8),heap?Read<std::uint64_t>(heap,0x30):0,heap?Read<std::uint32_t>(heap,0x18):0,Read<std::uint32_t>(manager,0x50)};
}
inline void Enter(void* lock){rt_allocator_probe::Enter(api,active,lock);}
inline void Leave(void* lock){rt_allocator_probe::Leave(api,active,lock);}
struct Scope {
 rt_allocator_probe::Sample local;rt_allocator_probe::Sample* previous=active;
 explicit Scope(std::uint64_t count){local.requested=count;active=&local;}
 ~Scope(){active=previous;const DWORD error=GetLastError();AcquireSRWLockExclusive(&statsLock);
  if(local.entered&&local.left){auto& t=totals[rt_allocator_probe::Classify(local)];++t.calls;t.wait+=local.wait;t.hold+=local.hold;t.release+=local.release;t.growth+=local.after.pool>local.before.pool?1u:0u;
   if(local.wait>peakWait.wait)peakWait=local;if(local.hold>peakHold.hold)peakHold=local;
  }else ++unobserved;
  ReleaseSRWLockExclusive(&statsLock);SetLastError(error);
 }
 Scope(const Scope&)=delete;
};
inline UINT64 Micros(UINT64 value){return value/frequency*1000000+(value%frequency)*1000000/frequency;}
inline void Report(){
 if(!frequency)return;const DWORD error=GetLastError();Totals copy[3];UINT64 missing;rt_allocator_probe::Sample wait,hold;
 AcquireSRWLockShared(&statsLock);for(unsigned i=0;i<3;++i)copy[i]=totals[i];missing=unobserved;wait=peakWait;hold=peakHold;ReleaseSRWLockShared(&statsLock);
 for(unsigned i=0;i<3;++i){const auto& t=copy[i];Log("RT_ALLOC_R411 class=%u sampled_calls=%llu wait_us=%llu hold_us=%llu release_us=%llu pool_growth_calls=%llu unobserved_calls=%llu ready=%u reservation_size=49 cpu_inclusive=1",i,t.calls,Micros(t.wait),Micros(t.hold),Micros(t.release),t.growth,missing,unsigned(ready));}
 Log("RT_ALLOC_R411_PEAK wait_us=%llu wait_hold_us=%llu wait_begin_qpc=%llu wait_end_qpc=%llu wait_class=%u hold_us=%llu hold_wait_us=%llu hold_begin_qpc=%llu hold_end_qpc=%llu hold_class=%u",Micros(wait.wait),Micros(wait.hold),wait.begin,wait.end,rt_allocator_probe::Classify(wait),Micros(hold.hold),Micros(hold.wait),hold.begin,hold.end,rt_allocator_probe::Classify(hold));SetLastError(error);
}
struct Patch {unsigned char* site=nullptr;unsigned char original[6]{},replacement[6]{};};inline Patch patches[2]{};
inline bool PreparePatch(unsigned char* site,void* expected,void* hook,Patch& patch){
 if(site[0]!=0xff||site[1]!=0x15)return false;std::int32_t displacement=0;std::memcpy(&displacement,site+2,4);
 if(rt_owned_assembly::Read<void*>(site+6+displacement)!=expected)return false;
 auto* relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(site));if(!relay||!rt_allocator_probe::Call6(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(relay),patch.replacement))return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(relay,jump,6);std::memcpy(relay+6,&hook,8);DWORD old=0;
 if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay,14))return false;
 patch.site=site;std::memcpy(patch.original,site,6);return true;
}
inline bool Exchange(Patch& patch,bool install){
 const auto* before=install?patch.original:patch.replacement;const auto* after=install?patch.replacement:patch.original;
 if(!patch.site||std::memcmp(patch.site,before,6))return false;DWORD old=0;if(!VirtualProtect(patch.site,6,PAGE_EXECUTE_READWRITE,&old))return false;
 std::memcpy(patch.site,after,6);const bool flushed=FlushInstructionCache(GetCurrentProcess(),patch.site,6)!=FALSE;DWORD ignored=0;
 return VirtualProtect(patch.site,6,old,&ignored)!=FALSE&&flushed;
}
inline bool Install(HMODULE d3d){
 if(!d3d||d3d!=verifiedD3d||!HashMatches(d3d,kD3dHash))return false;auto* base=reinterpret_cast<unsigned char*>(d3d);
 if(std::memcmp(base+0x1aa00,rt_lease_bytes::Allocate,sizeof(rt_lease_bytes::Allocate)))return false;
 const auto kernel=GetModuleHandleW(L"kernel32.dll");auto* acquire=reinterpret_cast<void*>(GetProcAddress(kernel,"AcquireSRWLockExclusive"));auto* release=reinterpret_cast<void*>(GetProcAddress(kernel,"ReleaseSRWLockExclusive"));if(!acquire||!release)return false;
 api={reinterpret_cast<void(*)(void*)>(acquire),reinterpret_cast<void(*)(void*)>(release),Clock,Error,Restore,Capture};LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 if(!PreparePatch(base+0x1aa67,acquire,reinterpret_cast<void*>(&Enter),patches[0])||!PreparePatch(base+0x1ac9f,release,reinterpret_cast<void*>(&Leave),patches[1]))return false;
 if(Exchange(patches[0],true)&&Exchange(patches[1],true)){ready=true;return true;}
 for(auto& patch:patches)if(patch.site&&!std::memcmp(patch.site,patch.replacement,6))Exchange(patch,false);return false;
}
}
