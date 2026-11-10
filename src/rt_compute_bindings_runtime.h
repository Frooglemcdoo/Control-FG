#pragma once
#include "rt_compute_bindings.h"
#include "rt_compute_bindings_bytes.h"
#include "native_cpu_call_patch.h"
namespace RTComputeBindings {
inline unsigned char* d3dBase=nullptr;inline rt_compute_bindings::Api api{};inline NativeCpuCallPatch patch{};
inline void(*nativeApply)(void*)=nullptr;inline std::atomic<UINT64> observerFallbacks{0};
inline void*(*retrieve)()=nullptr;inline void(*setShaders)(void*,const void* const*,bool)=nullptr;
inline void(*nativeUse)(void*,void*)=nullptr;inline void(*finishUse)(void*,void*)=nullptr;
inline std::atomic<UINT64> calls{0},tables{0},batched{0},descriptors{0},fallbacks{0},timedCalls{0},ticks{0};inline UINT64 frequency=0;
// Detail timings have their own cohort; ordinary apply samples exclude them.
constexpr unsigned phaseCount=static_cast<unsigned>(rt_compute_bindings::Phase::Count);
struct Detail {LARGE_INTEGER last;rt_compute_bindings::Phase phase;UINT64 duration[phaseCount];};
inline std::atomic<UINT64> detailCalls{0},phaseTicks[phaseCount]{};
inline UINT64 Micros(UINT64 value){return value/frequency*1000000+(value%frequency)*1000000/frequency;}
inline void MarkDetail(void* context,rt_compute_bindings::Phase phase){
 const DWORD error=GetLastError();LARGE_INTEGER now{};QueryPerformanceCounter(&now);
 auto& detail=*static_cast<Detail*>(context);detail.duration[static_cast<unsigned>(detail.phase)]+=static_cast<UINT64>(now.QuadPart-detail.last.QuadPart);detail.last=now;detail.phase=phase;SetLastError(error);
}
inline void* ThreadData(){auto** slots=reinterpret_cast<void**>(__readgsqword(0x58));return slots[rt_owned_assembly::Read<std::uint32_t>(d3dBase,0x1115fc)];}
inline void* State(void* tls){auto* state=rt_owned_assembly::Read<void*>(tls,0x20);return state?state:rt_owned_assembly::Read<void*>(d3dBase,0x111c10);}
inline ID3D12Device* Device(){return rt_owned_assembly::Read<ID3D12Device*>(d3dBase,0x111c08);}
inline void Cbv(const rt_compute_bindings::Cbv* src,std::uintptr_t dst){D3D12_CONSTANT_BUFFER_VIEW_DESC desc{src->address,src->bytes};Device()->CreateConstantBufferView(&desc,{dst});}
inline void One(std::uintptr_t dst,std::uintptr_t src){Device()->CopyDescriptorsSimple(1,{dst},{src},D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
inline void Many(unsigned n,const std::uintptr_t* dst,const std::uintptr_t* src){
 D3D12_CPU_DESCRIPTOR_HANDLE d[36]{},s[36]{};for(unsigned i=0;i<n;++i){d[i].ptr=dst[i];s[i].ptr=src[i];}
 Device()->CopyDescriptors(n,d,nullptr,n,s,nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}
inline void Apply(void* material){
 // These capture observers consume CopyDescriptorsSimple notifications. Preserve
 // their original path rather than bypass their descriptor shadows with batching.
 if(PTGlassRuntime::captureKind||gi_dual_bridge::observing){observerFallbacks.fetch_add(1,std::memory_order_relaxed);nativeApply(material);return;}
 const auto number=calls.fetch_add(1,std::memory_order_relaxed)+1;const bool detailed=number%256==1;const bool sampled=number%32==1&&!detailed;LARGE_INTEGER start{},end{};
 Detail detail;const rt_compute_bindings::Probe probe{MarkDetail,&detail};
 const DWORD error=GetLastError();if(sampled)QueryPerformanceCounter(&start);if(detailed){detail={};QueryPerformanceCounter(&detail.last);}SetLastError(error);
 const rt_compute_bindings::ShaderApi shaderApi{retrieve,setShaders,nativeUse,finishUse};
 const auto result=rt_compute_bindings::ApplyShader(api,shaderApi,material,detailed?&probe:nullptr);
 if(detailed){MarkDetail(&detail,rt_compute_bindings::Phase::Other);for(unsigned i=0;i<phaseCount;++i)phaseTicks[i].fetch_add(detail.duration[i],std::memory_order_relaxed);detailCalls.fetch_add(1,std::memory_order_relaxed);}
 const DWORD after=GetLastError();if(sampled){QueryPerformanceCounter(&end);ticks.fetch_add(static_cast<UINT64>(end.QuadPart-start.QuadPart),std::memory_order_relaxed);timedCalls.fetch_add(1,std::memory_order_relaxed);}SetLastError(after);
 tables.fetch_add(result.handled,std::memory_order_relaxed);batched.fetch_add(result.batches,std::memory_order_relaxed);descriptors.fetch_add(result.descriptors,std::memory_order_relaxed);fallbacks.fetch_add(result.native,std::memory_order_relaxed);
 if(number==1||number%16384==0){const auto value=ticks.load();Log("RT_BIND_R406 calls=%llu handled_tables=%llu batches=%llu descriptors=%llu native_tables=%llu sampled_calls=%llu apply_us=%llu observer_fallbacks=%llu persistent_cache=0 skipped_updates=0",number,tables.load(),batched.load(),descriptors.load(),fallbacks.load(),timedCalls.load(),value/frequency*1000000+(value%frequency)*1000000/frequency,observerFallbacks.load());
 Log("RT_BIND_R406_DETAIL calls=%llu other_us=%llu retrieve_us=%llu shaders_us=%llu eligibility_us=%llu constants_us=%llu cbv_us=%llu resolve_us=%llu copy_us=%llu uav_us=%llu native_us=%llu sample_period=256 cpu_inclusive=1 gpu_duration=0 ordinary_samples_exclude_detail=1",detailCalls.load(),Micros(phaseTicks[0].load()),Micros(phaseTicks[1].load()),Micros(phaseTicks[2].load()),Micros(phaseTicks[3].load()),Micros(phaseTicks[4].load()),Micros(phaseTicks[5].load()),Micros(phaseTicks[6].load()),Micros(phaseTicks[7].load()),Micros(phaseTicks[8].load()),Micros(phaseTicks[9].load()));SetLastError(after);}
}
inline bool Install(HMODULE renderer,HMODULE d3d)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d||!HashMatches(renderer,kRendererHash)||!HashMatches(d3d,kD3dHash))return false;
 auto* r=reinterpret_cast<unsigned char*>(renderer);d3dBase=reinterpret_cast<unsigned char*>(d3d);
 if(std::memcmp(r+0x1db750,rt_compute_bytes::NativeApply,sizeof(rt_compute_bytes::NativeApply))||std::memcmp(d3dBase+0x12e50,rt_compute_bytes::NativeBind,sizeof(rt_compute_bytes::NativeBind)))return false;
 // Validate all imports called by the original shader application wrapper.
 const unsigned slots[]={0x5dfcb0,0x5dfb10,0x5dfb18};const unsigned targets[]={0x1ca10,0x2f050,0x10770};
 for(unsigned i=0;i<3;++i)if(rt_owned_assembly::Read<void*>(r+slots[i])!=d3dBase+targets[i])return false;
 nativeApply=reinterpret_cast<decltype(nativeApply)>(r+0x1db750);
 retrieve=reinterpret_cast<decltype(retrieve)>(d3dBase+0x1ca10);setShaders=reinterpret_cast<decltype(setShaders)>(d3dBase+0x2f050);nativeUse=reinterpret_cast<decltype(nativeUse)>(d3dBase+0x10770);finishUse=reinterpret_cast<decltype(finishUse)>(d3dBase+0x132d0);
 api={reinterpret_cast<void(*)(void*)>(d3dBase+0x10570),ThreadData,State,Cbv,One,Many,d3dBase+0x112840,reinterpret_cast<const std::uint32_t*>(d3dBase+0x111bc0),reinterpret_cast<void* const*>(d3dBase+0x111bb8)};
 LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 if(!NativeCpuPrepareCallPatch(r+0xa6b6a,r+0x1db750,reinterpret_cast<void*>(&Apply),&patch))return false;
 if(NativeCpuExchangeCall(&patch,true)&&patch.writeHealthy)return true;
 if(!std::memcmp(patch.site,patch.replacement,5))NativeCpuExchangeCall(&patch,false);return false;
}
}
