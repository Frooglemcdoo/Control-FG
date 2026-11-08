#pragma once
#include <atomic>
#include <array>
#include <cstdint>
namespace settings_check {
enum Event : unsigned {RRSuccess,RRFailure,OwnedGI,NativeGI,CharacterEnhanced,CharacterNative,VolumeEnhanced,VolumeNative,LightingEnhanced,LightingNative,Clamp60,ClampOther,RelaxRecorded,RelaxSubmitted,RelaxCompleted,RelaxFallback,GISampleQueries,Count};
inline constexpr const char* names[]={"rr_evaluations_succeeded","rr_evaluations_failed","owned_gi_dispatches","native_gi_dispatches","enhanced_character_bindings","native_character_bindings","enhanced_volume_bindings","native_volume_bindings","enhanced_lighting_bindings","native_lighting_bindings","clamp60_invocations","other_clamp_invocations","relax_recorded","relax_submitted","relax_gpu_completed","relax_rejections","gi_sample_queries"};
inline std::atomic<bool> active{false};
inline std::atomic<std::uint64_t> generation{0};
inline std::array<std::atomic<std::uint64_t>,Count> counts{};
inline std::atomic<unsigned> giSampleMask{0};
inline void RecordGISamples(unsigned value) noexcept {
 if(!active.load(std::memory_order_relaxed))return;
 ++counts[GISampleQueries];giSampleMask.fetch_or(1u<<(value<31?value:31));
}
inline std::atomic<const char*> reason{"none"};
inline void Record(Event e) noexcept {if(active.load(std::memory_order_relaxed))++counts[e];}
inline void RecordFor(Event e,std::uint64_t id) noexcept {if(id&&id==generation.load())Record(e);}
inline std::uint64_t Reset() noexcept {
 active=false;for(auto& c:counts)c=0;giSampleMask=0;reason="none";
 return ++generation;
}
inline std::uint64_t Begin() noexcept {const auto id=Reset();active=true;return id;}
inline void End() noexcept {active=false;}
inline const char* Verdict(bool requested,std::uint64_t evidence,std::uint64_t failures=0) noexcept {
 return !requested?(evidence?"UNEXPECTED_ACTIVITY":"OFF"):
 failures?"CHECK_FAILED":evidence?"OBSERVED":"NOT_OBSERVED";
}
}
