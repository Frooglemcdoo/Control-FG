#pragma once
#include <atomic>
#include "rr_input_capture_policy.h"
#include "rr_suite_policy.h"
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
#include "rr_gi_boundary.h"
#endif
#if defined(CONTROL_RR_F9_SUITE) && !defined(CONTROL_GI_BOUNDARY_CAPTURE)
inline constexpr unsigned kRRCapturePairs=rr_suite::maxPairs;
#else
inline constexpr unsigned kRRCapturePairs=2;
#endif
namespace control_rr_capture_ui {
enum class State { Idle, Queued, Preparing, Capturing, Saving, Complete, Failed };
inline std::atomic<State> state{State::Idle};
inline std::atomic<bool> requested{false};
inline std::atomic<unsigned> completedPairs{0};
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
inline std::atomic<const wchar_t*> detail{L"Capture GI CLAMP ONLY, then GI BOTH at the same view. RR ON, FG OFF."};
#else
inline std::atomic<const wchar_t*> detail{L"F9: capture sequence at painting, then elevator. RR ON, FG OFF."};
#endif
inline bool Busy() noexcept {
 const auto s=state.load();return s==State::Queued||s==State::Preparing||s==State::Capturing||s==State::Saving;
}
inline bool Request() noexcept {
    auto current=state.load();
    if(completedPairs.load()>=kRRCapturePairs || (current!=State::Idle && current!=State::Complete))return false;
    if(!state.compare_exchange_strong(current,State::Queued))return false;
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
    control_rr_gi::boundaryRequested.store(true);
#endif
    detail.store(L"Waiting for gameplay and an evaluated frame.");
    requested.store(true,std::memory_order_release);return true;
}
inline void Publish(control_rr::InputCapturePolicy::Phase first,control_rr::InputCapturePolicy::Phase second,
                    unsigned requests,bool outputsValid) noexcept {
    using P=control_rr::InputCapturePolicy;
    if(requested.load() || !requests)return;
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
    if(first==P::Failed || second==P::Failed || (first==P::Idle && second==P::Idle))control_rr_gi::boundaryRequested.store(false);
#endif
    if(first==P::Failed || second==P::Failed){state.store(State::Failed);return;}
    if(first==P::Idle && second==P::Idle){
        if(outputsValid){completedPairs.store(requests);
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
 detail.store(requests<2?L"Clamp pair saved. Select GI BOTH at the same view and capture again.":L"Both pairs saved. Click PACKAGE RR to collect the ZIPs.");
#elif defined(CONTROL_RR_F9_SUITE)
 detail.store(L"Pair saved. F9 sequence advances automatically; hold still.");
#else
 detail.store(requests<2?L"Pair 1 saved. Move to the elevator and capture again.":L"Both pairs saved. Click PACKAGE RR to collect the ZIPs.");
#endif
state.store(State::Complete);}
        else {detail.store(L"Capture pair incomplete. PACKAGE RR will collect diagnostics.");state.store(State::Failed);}
    } else if(first==P::Exporting || second==P::Exporting){detail.store(L"Saving images on the background worker...");state.store(State::Saving);}
    else if(first==P::Preparing || second==P::Preparing){detail.store(L"Preparing capture buffers...");state.store(State::Preparing);}
    else {state.store(State::Capturing);}
}
inline const wchar_t* Label(State value) noexcept {
    switch(value){
    case State::Idle:return L"Ready";case State::Queued:return L"Requested";
    case State::Preparing:return L"Preparing";case State::Capturing:return L"Capturing";
    case State::Saving:return L"Saving";case State::Complete:return L"Saved";
    case State::Failed:return L"Failed";
    }return L"Unknown";
}
}
