#pragma once
#include <cstdint>
namespace control_rr {
enum class CaptureState {Armed,Recording,Recorded,Submitted,Exporting,Failed};
struct LiveCapturePolicy {
 CaptureState state=CaptureState::Armed;unsigned slot=0;std::uint64_t serial=0,fence=0;
 bool Begin(unsigned i,std::uint64_t token) noexcept {if(state!=CaptureState::Armed||i>=3||!token)return false;slot=i;serial=token;state=CaptureState::Recording;return true;}
 void Recorded(bool okay) noexcept {state=okay?CaptureState::Recorded:CaptureState::Failed;}
 void Submit(unsigned i,std::uint64_t token,std::uint64_t value) noexcept {if(state==CaptureState::Recorded&&slot==i&&serial==token){if(value){fence=value;state=CaptureState::Submitted;}else state=CaptureState::Failed;}}
 bool Retire(std::uint64_t completed) noexcept {if(state!=CaptureState::Submitted)return false;if(completed==UINT64_MAX){state=CaptureState::Failed;return false;}if(completed<fence)return false;state=CaptureState::Exporting;return true;}
};
}
