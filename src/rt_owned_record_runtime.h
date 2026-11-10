#pragma once
#include "rt_owned_record.h"
#include "rt_owned_record_bytes.h"
#include "rt_owned_record_patch.h"
namespace RTOwnedRecord {
inline rt_owned_record::Api api{};inline rt_owned_record::Globals globals{};
inline unsigned char* site=nullptr;inline unsigned char replacement[5]{};inline UINT64 frequency=0;
inline std::atomic<UINT64> jobs{0},instances{0},updates{0},builds{0},secondary{0},finalizes{0},materials{0},copies{0},timed{0};
inline std::atomic<UINT64> timedUpdates{0},timedDynamicJobs{0};
inline std::atomic<UINT64> phaseTicks[9]{};
struct Timing {bool enabled=false;LONGLONG last=0;UINT64 ticks[9]{};};inline thread_local Timing* timing=nullptr;
inline void Phase(unsigned phase){if(!timing||!timing->enabled)return;const DWORD error=GetLastError();LARGE_INTEGER now{};QueryPerformanceCounter(&now);timing->ticks[phase]=static_cast<UINT64>(now.QuadPart-timing->last);timing->last=now.QuadPart;SetLastError(error);}
inline UINT64 Micros(UINT64 ticks)noexcept{return ticks/frequency*1000000+(ticks%frequency)*1000000/frequency;}
inline void Hook(void* capture){
 RTDescriptorLease::JobScope descriptorLease;
 const auto n=jobs.fetch_add(1,std::memory_order_relaxed)+1;Timing local;local.enabled=(n%16)==1;
 struct Restore {Timing* previous;~Restore(){timing=previous;}} restore{timing};timing=&local;if(local.enabled){const DWORD error=GetLastError();LARGE_INTEGER start{};QueryPerformanceCounter(&start);local.last=start.QuadPart;SetLastError(error);}
 const auto stats=rt_owned_record::Run(api,globals,capture);
 instances.fetch_add(stats.instances>0?static_cast<UINT64>(stats.instances):0,std::memory_order_relaxed);updates.fetch_add(stats.updates,std::memory_order_relaxed);builds.fetch_add(stats.builds,std::memory_order_relaxed);secondary.fetch_add(stats.secondaryBuilds,std::memory_order_relaxed);finalizes.fetch_add(stats.finalizes,std::memory_order_relaxed);materials.fetch_add(stats.materials,std::memory_order_relaxed);copies.fetch_add(stats.copied?1u:0u,std::memory_order_relaxed);
 if(local.enabled){timedUpdates.fetch_add(stats.updates,std::memory_order_relaxed);timedDynamicJobs.fetch_add(stats.updates?1u:0u,std::memory_order_relaxed);for(unsigned i=0;i<9;++i)phaseTicks[i].fetch_add(local.ticks[i],std::memory_order_relaxed);timed.fetch_add(1,std::memory_order_relaxed);}
 if(n==1||n%512==0){const DWORD error=GetLastError();
  Log("RT_RECORD_R407 active=1 jobs=%llu instances=%llu dynamic_updates=%llu blas_callbacks=%llu secondary_callbacks=%llu finalize_callbacks=%llu material_ids=%llu postbuild_copies=%llu timed_jobs=%llu thread_setup_us=%llu dynamic_us=%llu blas_finalize_us=%llu material_ids_us=%llu tlas_finish_us=%llu sample_period=16 native_record_body=0 native_gpu_callbacks=1 native_queue=1 native_retirement=1 tlas_builds_per_job=1 ptlas=0",n,instances.load(),updates.load(),builds.load(),secondary.load(),finalizes.load(),materials.load(),copies.load(),timed.load(),Micros(phaseTicks[0].load()),Micros(phaseTicks[1].load()+phaseTicks[2].load()+phaseTicks[3].load()+phaseTicks[4].load()+phaseTicks[5].load()),Micros(phaseTicks[6].load()),Micros(phaseTicks[7].load()),Micros(phaseTicks[8].load()));
  Log("RT_RECORD_R407_DETAIL timed_jobs=%llu timed_dynamic_jobs=%llu timed_updates=%llu device_us=%llu common_bindings_us=%llu transition_in_us=%llu provider_updates_us=%llu transition_out_us=%llu sample_period=16 cpu_inclusive=1 gpu_duration=0",timed.load(),timedDynamicJobs.load(),timedUpdates.load(),Micros(phaseTicks[1].load()),Micros(phaseTicks[2].load()),Micros(phaseTicks[3].load()),Micros(phaseTicks[4].load()),Micros(phaseTicks[5].load()));SetLastError(error);
 }
}
inline bool Exchange(bool enable)noexcept{
 const auto* before=enable?nativeRecordTail:replacement;const auto* after=enable?replacement:nativeRecordTail;
 if(std::memcmp(site,before,5))return false;DWORD old=0;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old))return false;
 std::memcpy(site,after,5);const bool flush=FlushInstructionCache(GetCurrentProcess(),site,5)!=FALSE;DWORD ignored=0;return VirtualProtect(site,5,old,&ignored)!=FALSE&&flush;
}
inline bool Install(HMODULE renderer,HMODULE d3d)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d||!HashMatches(renderer,kRendererHash))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);site=base+0xa8f74;
 const unsigned char wrapper[]={0x48,0x83,0xc1,0x18};
 if(std::memcmp(base+0xa89c0,nativeRecordOracle,sizeof(nativeRecordOracle))||std::memcmp(site,nativeRecordTail,5)||std::memcmp(site-4,wrapper,4))return false;
 struct Import {unsigned rva;const char* name;};
 const Import imports[]={
  {0x5df510,"?getCommandQueue@CommandQueue@d3d@@SAPEAV12@W4CommandQueueType@2@@Z"},
  {0x5df520,"?initializeThread@ThreadingCore@d3d@@SAXAEBUThreadInit@2@@Z"},
  {0x5dfcb0,"?retrieveForThread@DeviceState@d3d@@SAAEAV12@XZ"},
  {0x5df528,"?transitionFromConsumeToUAV@NativeBufferUtilDX12@d3d@@SAXPEAPEAVNativeBuffer@2@H@Z"},
  {0x5df530,"?transitionFromUAVToConsume@NativeBufferUtilDX12@d3d@@SAXPEAPEAVNativeBuffer@2@H@Z"},
  {0x5df958,"?transitionFromUAVToCopy@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z"},
  {0x5df960,"?copy@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@0@Z"},
  {0x5df968,"?transitionFromCopyToUAV@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z"},
  {0x5df538,"?uavBarrier@NativeBufferUtilDX12@d3d@@SAXXZ"},
  {0x5df540,"?buildTopLevelAccelerationStructure@DeviceUtilRaytracing@d3d@@SAXW4RaytracingAccelerationBuildFlags@2@AEAVNativeBuffer@2@H11_N@Z"},
  {0x5df548,"?finishThread@ThreadingCore@d3d@@SAPEAVDeferredCleanupCommand@2@XZ"}};
 for(const auto& x:imports){auto* target=reinterpret_cast<void*>(GetProcAddress(d3d,x.name));if(!target||rt_owned_assembly::Read<void*>(base+x.rva)!=target)return false;}
 #define RT_RECORD_IMPORT(member,rva) api.member=reinterpret_cast<decltype(api.member)>(rt_owned_assembly::Read<void*>(base+rva))
 RT_RECORD_IMPORT(queue,0x5df510);RT_RECORD_IMPORT(initialize,0x5df520);RT_RECORD_IMPORT(device,0x5dfcb0);
 RT_RECORD_IMPORT(consumeToUav,0x5df528);RT_RECORD_IMPORT(uavToConsume,0x5df530);RT_RECORD_IMPORT(uavToCopy,0x5df958);RT_RECORD_IMPORT(copy,0x5df960);RT_RECORD_IMPORT(copyToUav,0x5df968);RT_RECORD_IMPORT(barrier,0x5df538);RT_RECORD_IMPORT(build,0x5df540);RT_RECORD_IMPORT(finish,0x5df548);
 #undef RT_RECORD_IMPORT
 api.bindCommon=reinterpret_cast<decltype(api.bindCommon)>(base+0x10f160);api.phase=&Phase;
 globals={base+0x7ec040,base+0x1291c60,reinterpret_cast<void* const*>(base+0x802950),reinterpret_cast<void* const*>(base+0x128d350),reinterpret_cast<void**>(base+0x8028c0)};
 LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 auto* relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(site));unsigned char stub[14]{};
 if(!relay||!rt_owned_record::TailPatch(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(relay),reinterpret_cast<std::uintptr_t>(&Hook),replacement,stub))return false;
 std::memcpy(relay,stub,sizeof(stub));DWORD old=0;if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay,sizeof(stub)))return false;
 if(Exchange(true))return true;if(!std::memcmp(site,replacement,5))Exchange(false);return false;
}
}
