#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <cstring>
namespace settings_check {
enum Event : unsigned {RRSuccess,RRFailure,OwnedGI,NativeGI,CharacterEnhanced,CharacterNative,VolumeEnhanced,VolumeNative,LightingEnhanced,LightingNative,Clamp60,ClampOther,RelaxRecorded,RelaxSubmitted,RelaxCompleted,RelaxFallback,GISampleQueries,OwnedRays,NativeRays,GIReuse,ReceiverExport,RelaxPasses,GINativeTemporal,GINativeSpatial,GICustomTemporal,GICustomSpatial,GIRawCopy,GINativeFilterFallback,SpecularNativeWithRR,BroadNativeWithRR,EnhancedShadowRays,RelaxWorkCreated,RelaxWorkReused,Count};
inline constexpr const char* names[]={"rr_evaluations_succeeded","rr_evaluations_failed","owned_gi_dispatches","native_gi_dispatches","enhanced_character_bindings","native_character_bindings","enhanced_volume_bindings","native_volume_bindings","enhanced_lighting_bindings","native_lighting_bindings","clamp60_invocations","other_clamp_invocations","relax_recorded","relax_submitted","relax_gpu_completed","relax_rejections","gi_sample_queries","owned_gi_ray_dispatches","native_gi_ray_dispatches","gi_temporal_visibility_dispatches","relax_receiver_export_dispatches","relax_internal_compute_dispatches","native_gi_temporal_dispatches","native_gi_spatial_dispatches","other_custom_gi_temporal_dispatches","other_custom_gi_spatial_dispatches","gi_raw_copy_dispatches","unintercepted_gi_filter_calls","native_specular_filter_calls_with_rr","native_broad_diffuse_filter_calls_with_rr","enhanced_shadow_ray_dispatches","relax_work_bundles_created","relax_work_bundles_reused"};
inline std::atomic<bool> active{false};
inline std::atomic<std::uint64_t> generation{0};
inline std::array<std::atomic<std::uint64_t>,Count> counts{};
inline std::atomic<unsigned> giSampleMask{0};
inline void RecordGISamples(unsigned value) noexcept {
 if(!active.load(std::memory_order_relaxed))return;
 ++counts[GISampleQueries];giSampleMask.fetch_or(1u<<(value<31?value:31));
}
inline std::atomic<const char*> reason{"none"};
struct Failure {char reason[96]{};std::uint64_t frame=0,present=0,guideFrame=0;unsigned recordings=0,pending=0;bool sameView=false;};
inline constexpr unsigned failureCapacity=256;
inline std::mutex failureMutex;
inline std::array<Failure,failureCapacity> failures{};
inline unsigned failureCount=0,failureDropped=0;
inline void RecordFailure(const char* why,std::uint64_t frame,std::uint64_t present,std::uint64_t guideFrame,unsigned recordings,unsigned pending,bool sameView){
 if(!active.load(std::memory_order_relaxed))return;
 std::lock_guard<std::mutex> lock(failureMutex);if(!active.load())return;
 if(failureCount==failureCapacity){++failureDropped;return;}
 auto& f=failures[failureCount++];f={};const char* text=why?why:"unknown";
 for(unsigned i=0;i<95&&text[i];++i)f.reason[i]=text[i];
 f.frame=frame;f.present=present;f.guideFrame=guideFrame;f.recordings=recordings;f.pending=pending;f.sameView=sameView;
}

inline void Record(Event e,std::uint64_t n=1) noexcept {if(active.load(std::memory_order_relaxed))counts[e].fetch_add(n,std::memory_order_relaxed);}
inline void RecordFor(Event e,std::uint64_t id) noexcept {if(id&&id==generation.load())Record(e);}
inline std::uint64_t Reset() noexcept {
 active=false;{std::lock_guard<std::mutex> lock(failureMutex);failureCount=0;failureDropped=0;}for(auto& c:counts)c=0;giSampleMask=0;reason="none";
 return ++generation;
}
inline std::uint64_t Begin() noexcept {const auto id=Reset();active=true;return id;}
inline void End() noexcept {active=false;}
inline const char* Verdict(bool requested,std::uint64_t evidence,std::uint64_t failureEvents=0) noexcept {
 return !requested?(evidence?"UNEXPECTED_ACTIVITY":"OFF"):
 failureEvents?"CHECK_FAILED":evidence?"OBSERVED":"NOT_OBSERVED";
}
}
