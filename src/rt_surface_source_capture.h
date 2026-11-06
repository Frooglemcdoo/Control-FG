#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include "rr_gi_stage_policy.h"
namespace rt_surface_source {
// Transient diagnostic request. No saved setting is changed. Zero is gameplay.
inline std::atomic<unsigned> request{0};
inline thread_local unsigned giScope=0;
inline constexpr unsigned phases=7;
enum class Event : unsigned {GIEntry,GIEligible,GITemporal,GISpatial,GIZero,GIRaw, GISteady,
    ContactEntry,ContactNative,ContactNeutral,ContactTemporal,ContactSpatial,Count};
inline constexpr const char* names[]={"gi_entry","gi_eligible","gi_temporal","gi_spatial","gi_zero","gi_raw","gi_steady",
    "contact_entry","contact_native","contact_neutral","contact_temporal","contact_spatial"};
inline constexpr unsigned count=unsigned(Event::Count);
static_assert(sizeof(names)/sizeof(names[0])==count);
using Snapshot=std::array<uint64_t,count>;
inline std::array<std::array<std::atomic<uint64_t>,count>,phases> counters{};
inline void Record(unsigned phase,Event e) noexcept {
    if(phase && phase<phases)counters[phase][unsigned(e)].fetch_add(1,std::memory_order_relaxed);
}
inline Snapshot Read(unsigned phase) noexcept {
    Snapshot result{};if(phase>=phases)return result;
    for(unsigned i=0;i<count;++i)result[i]=counters[phase][i].load(std::memory_order_relaxed);
    return result;
}
inline bool StabilizeGI(unsigned phase) noexcept {return phase==3 || phase==5;}
inline bool PreserveContact(unsigned phase) noexcept {return phase==4 || phase==5;}
inline control_rr_gi::StageMode Stage(unsigned phase,control_rr_gi::StageMode saved) noexcept {
    return phase==2?control_rr_gi::StageMode::ClampOnly:
        StabilizeGI(phase)?control_rr_gi::StageMode::BothSteadySpatial:saved;
}
inline control_rr_gi::DiagnosticView View(unsigned phase,control_rr_gi::StageMode mode,control_rr_gi::DiagnosticView saved) noexcept {
    return phase==2?control_rr_gi::DiagnosticView::Zero:
        StabilizeGI(phase)?control_rr_gi::DiagnosticView::Normal:control_rr_gi::EffectiveView(mode,saved);
}
inline bool Covered(unsigned phase,const Snapshot& delta) noexcept {
    if(phase==2 && !delta[unsigned(Event::GIZero)])return false;
    if(StabilizeGI(phase) && (!delta[unsigned(Event::GIEligible)] || !delta[unsigned(Event::GITemporal)] || !delta[unsigned(Event::GISteady)]))return false;
    if(PreserveContact(phase) && (!delta[unsigned(Event::ContactEntry)] || !delta[unsigned(Event::ContactNative)] || (!delta[unsigned(Event::ContactTemporal)] && !delta[unsigned(Event::ContactSpatial)])))return false;
    return phase>0 && phase<phases;
}
}
