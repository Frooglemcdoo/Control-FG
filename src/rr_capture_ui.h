#pragma once
#include <atomic>
#include "rr_input_capture_policy.h"
namespace control_rr_capture_ui {
enum class State { Idle, Queued, Preparing, Capturing, Saving, Complete, Failed };
inline std::atomic<State> state{State::Idle};
inline std::atomic<bool> requested{false};
inline std::atomic<unsigned> completedPairs{0};
inline std::atomic<const wchar_t*> detail{L"Capture the painting, then the elevator. Keep RR on and SPEC 4."};
inline bool Request() noexcept {
    auto current=state.load();
    if(completedPairs.load()>=2 || (current!=State::Idle && current!=State::Complete))return false;
    if(!state.compare_exchange_strong(current,State::Queued))return false;
    detail.store(L"Waiting for gameplay and an evaluated frame.");
    requested.store(true,std::memory_order_release);return true;
}
inline void Publish(control_rr::InputCapturePolicy::Phase first,control_rr::InputCapturePolicy::Phase second,
                    unsigned requests,bool outputsValid) noexcept {
    using P=control_rr::InputCapturePolicy;
    if(requested.load() || !requests)return;
    if(first==P::Failed || second==P::Failed){state.store(State::Failed);return;}
    if(first==P::Idle && second==P::Idle){
        if(outputsValid){completedPairs.store(requests);detail.store(requests<2?L"Pair 1 saved. Move to the elevator and capture again.":L"Both pairs saved. Click PACKAGE RR to collect the ZIPs.");state.store(State::Complete);}
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
