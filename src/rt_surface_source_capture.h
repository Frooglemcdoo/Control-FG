#pragma once
#include "diagnostic_activity.h"
#include <array>
#include <atomic>
#include <cstdint>
#include "rr_gi_stage_policy.h"
namespace rt_surface_source {
// Transient diagnostic request. No saved setting is changed. Zero is gameplay.
inline std::atomic<unsigned> request{0};
// Session-only manual comparisons; automated capture has priority.
inline std::atomic<unsigned> manualGI{0}; // 0 saved, 1 removed, 2 stabilized, 3 owned denoiser
inline std::atomic<unsigned> manualContact{0}; // 0 auto, 1 preserved, 2 unfiltered
inline std::atomic<bool> ownedFilterEnabled{true};
inline unsigned EffectiveRequest() noexcept {
#ifdef CONTROL_PERFORMANCE_BUILD
 return ownedFilterEnabled.load(std::memory_order_relaxed)?7:0; // Controls only; no capture overrides.
#else
 const unsigned capture=request.load(std::memory_order_acquire);
 if(capture)return capture;
 const unsigned gi=manualGI.load();
 return gi==1?2:gi==2?3:gi==3?7:0;
#endif
}
inline bool ManualContactUnfiltered() noexcept {return !request.load() && manualContact.load()==2;}
inline bool ManualContactPreserved() noexcept {return !request.load() && manualContact.load()==1;}
inline void ResetManual() noexcept {manualGI.store(0);manualContact.store(0);}
inline thread_local unsigned giScope=0;
inline constexpr unsigned phases=9;
enum class Event : unsigned {GIEntry,GIEligible,GITemporal,GISpatial,GIZero,GIRaw, GISteady,
    ContactEntry,ContactNative,ContactNeutral,ContactTemporal,ContactSpatial,GIStableTemporal,Count};
inline constexpr const char* names[]={"gi_entry","gi_eligible","gi_temporal","gi_spatial","gi_zero","gi_raw","gi_steady",
    "contact_entry","contact_native","contact_neutral","contact_temporal","contact_spatial","gi_stable_temporal"};
inline constexpr unsigned count=unsigned(Event::Count);
static_assert(sizeof(names)/sizeof(names[0])==count);
using Snapshot=std::array<uint64_t,count>;
inline std::array<std::array<std::atomic<uint64_t>,count>,phases> counters{};
inline void Record(unsigned phase,Event e) noexcept {
    if(!control_diagnostics::Active())return;
#ifndef CONTROL_PERFORMANCE_BUILD
    if(phase && phase<phases)counters[phase][unsigned(e)].fetch_add(1,std::memory_order_relaxed);
#endif
}
inline Snapshot Read(unsigned phase) noexcept {
    Snapshot result{};if(phase>=phases)return result;
    for(unsigned i=0;i<count;++i)result[i]=counters[phase][i].load(std::memory_order_relaxed);
    return result;
}
inline unsigned SpatialPasses(unsigned phase,control_rr_gi::StageMode mode,unsigned native) noexcept {return phase==7?1:control_rr_gi::SpatialPasses(mode,native);}
// Report the active override instead of an unrelated saved stage selection.
inline const wchar_t* FilterLabel(unsigned phase,control_rr_gi::StageMode saved) noexcept {
 if(phase==8)return L"GI: RAW BYPASS";
 if(phase==7)return L"GI: OWNED FILTER";
 if(phase==2)return L"GI: REMOVED";
 if(phase==3||phase==5)return L"GI: STABLE FILTER";
 return control_rr_gi::StageLabel(saved);
}
inline void SelectSavedStage(control_rr_gi::StageMode stage) noexcept {
 manualGI.store(0);control_rr_gi::stageMode.store(stage,std::memory_order_release);
}
inline bool StabilizeGI(unsigned phase) noexcept {return phase==3 || phase==5;}
inline bool PreserveContact(unsigned phase) noexcept {return phase==4 || phase==5;}
inline control_rr_gi::StageMode Stage(unsigned phase,control_rr_gi::StageMode saved) noexcept {
    return phase==8?control_rr_gi::StageMode::RawBypass:phase==7?control_rr_gi::StageMode::TemporalOnly:phase==2?control_rr_gi::StageMode::ClampOnly:
        StabilizeGI(phase)?control_rr_gi::StageMode::BothSteadySpatial:saved;
}
inline control_rr_gi::DiagnosticView View(unsigned phase,control_rr_gi::StageMode mode,control_rr_gi::DiagnosticView saved) noexcept {
    return phase==2?control_rr_gi::DiagnosticView::Zero:
        StabilizeGI(phase)?control_rr_gi::DiagnosticView::Normal:control_rr_gi::EffectiveView(mode,saved);
}
inline bool Covered(unsigned phase,const Snapshot& delta) noexcept {
    if(phase==8 && (!delta[unsigned(Event::GIRaw)] || !delta[unsigned(Event::GIEligible)] || delta[unsigned(Event::GISpatial)]))return false;
    if(phase==2 && (!delta[unsigned(Event::GIZero)] || !delta[unsigned(Event::GIEligible)] || !delta[unsigned(Event::GITemporal)]))return false;
    if(StabilizeGI(phase) && (!delta[unsigned(Event::GIStableTemporal)] || !delta[unsigned(Event::GIEligible)] || !delta[unsigned(Event::GITemporal)] || !delta[unsigned(Event::GISteady)]))return false;
    if(PreserveContact(phase) && (!delta[unsigned(Event::ContactEntry)] || !delta[unsigned(Event::ContactNative)] || (!delta[unsigned(Event::ContactTemporal)] && !delta[unsigned(Event::ContactSpatial)])))return false;
    return phase>0 && phase<phases;
}
}
