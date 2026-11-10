#pragma once
#include "rt_setup_probe.h"
#include "rt_descriptor_lease_bytes.h"
#include "rt_setup_probe_bytes.h"
#include "native_cpu_call_patch.h"
namespace RTSetupProbe {
inline unsigned char* base=nullptr;inline NativeCpuCallPatch patch{};inline rt_setup_probe::Api api{};
inline void(*original)(void*,bool,bool)=nullptr;
struct Sample {UINT64 ticks[3]{},calls=0,clears=0,samplers=0;LONGLONG begin=0,last=0,end=0;};
inline thread_local Sample* active=nullptr;inline thread_local bool preparing=false;
inline SRWLOCK lock=SRWLOCK_INIT;inline UINT64 totalTicks[3]{},totalCalls=0,clears=0,samplers=0,peakTicks=0;inline Sample peak{};inline UINT64 frequency=0;
inline UINT64 Micros(UINT64 value){return value/frequency*1000000+(value%frequency)*1000000/frequency;}
inline void Mark(void* context,unsigned phase){
 const DWORD error=GetLastError();LARGE_INTEGER now{};QueryPerformanceCounter(&now);auto& sample=*static_cast<Sample*>(context);
 if(phase==0){if(!sample.calls)sample.begin=now.QuadPart;++sample.calls;}else sample.ticks[phase-1]+=static_cast<UINT64>(now.QuadPart-sample.last);
 sample.last=now.QuadPart;sample.end=now.QuadPart;SetLastError(error);
}
struct Scope {
 Sample local;Sample* previous=active;bool done=false;
 explicit Scope(bool enabled){active=enabled?&local:nullptr;}
 void Finish(){if(done)return;done=true;active=previous;if(!local.calls)return;const DWORD error=GetLastError();AcquireSRWLockExclusive(&lock);
  totalCalls+=local.calls;clears+=local.clears;samplers+=local.samplers;UINT64 sum=0;for(unsigned i=0;i<3;++i){totalTicks[i]+=local.ticks[i];sum+=local.ticks[i];}
  if(sum>peakTicks){peakTicks=sum;peak=local;}ReleaseSRWLockExclusive(&lock);SetLastError(error);
 }
 ~Scope(){Finish();}Scope(const Scope&)=delete;
};
inline void Initialize(std::uint64_t destination){
 auto* device=rt_owned_assembly::Read<ID3D12Device*>(base,0x111c08);
 auto* manager=rt_owned_assembly::Read<void*>(base,0x111bb8);
 device->CopyDescriptorsSimple(49,{destination},{rt_owned_assembly::Read<UINT64>(manager,0xa8)},D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}
inline void Prepare(void* state,bool clear,bool sampler){
 if(!active||preparing){original(state,clear,sampler);return;}
 struct Guard{Guard(){preparing=true;}~Guard(){preparing=false;}} guard;
 active->clears+=clear?1u:0u;active->samplers+=sampler?1u:0u;
 rt_setup_probe::Run(api,state,clear,sampler,Mark,active);
}
inline void Report(){
 if(!frequency)return;const DWORD error=GetLastError();UINT64 times[3],count,clearCount,samplerCount,maximum;Sample worst;
 AcquireSRWLockShared(&lock);for(unsigned i=0;i<3;++i)times[i]=totalTicks[i];count=totalCalls;clearCount=clears;samplerCount=samplers;maximum=peakTicks;worst=peak;ReleaseSRWLockShared(&lock);
 Log("RT_SETUP_R408 sampled_calls=%llu allocation_us=%llu sampler_us=%llu initialization_us=%llu clear_calls=%llu nonzero_sampler_calls=%llu peak_us=%llu peak_allocation_us=%llu peak_sampler_us=%llu peak_initialization_us=%llu peak_begin_qpc=%lld peak_end_qpc=%lld reservation_size=49 lease_enabled=0 cpu_inclusive=1 gpu_duration=0",count,Micros(times[0]),Micros(times[1]),Micros(times[2]),clearCount,samplerCount,Micros(maximum),Micros(worst.ticks[0]),Micros(worst.ticks[1]),Micros(worst.ticks[2]),worst.begin,worst.end);SetLastError(error);
}
inline bool Install(HMODULE d3d)noexcept{
 if(!d3d||d3d!=verifiedD3d||!HashMatches(d3d,kD3dHash))return false;base=reinterpret_cast<unsigned char*>(d3d);
 if(std::memcmp(base+0x1bd50,rt_lease_bytes::Prepare,sizeof(rt_lease_bytes::Prepare))||std::memcmp(base+0x2f050,rt_setup_bytes::SetShaders,sizeof(rt_setup_bytes::SetShaders)))return false;
 original=reinterpret_cast<decltype(original)>(base+0x1bd50);
 api={reinterpret_cast<decltype(api.allocate)>(base+0x1aa00),reinterpret_cast<decltype(api.samplers)>(base+0x1acf0),Initialize};
 LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 if(!NativeCpuPrepareCallPatch(base+0x2f0a9,base+0x1bd50,reinterpret_cast<void*>(&Prepare),&patch))return false;
 if(NativeCpuExchangeCall(&patch,true)&&patch.writeHealthy)return true;
 if(!std::memcmp(patch.site,patch.replacement,5))NativeCpuExchangeCall(&patch,false);return false;
}
}
