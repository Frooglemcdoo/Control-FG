#pragma once
#include <cstdint>
namespace control_rr {
struct InputCapturePolicy {
 enum Phase {Idle,Requested,Preparing,Ready,Recorded,Submitted,Exporting,Failed};
 Phase phase=Idle;unsigned requests=0;std::uint64_t present=0;
 bool Request(){if(phase!=Idle||requests>=
#ifdef CONTROL_RR_F9_SUITE
 32
#else
 4
#endif
 )return false;++requests;phase=Requested;return true;}
 bool BeginPrepare(){if(phase!=Requested)return false;phase=Preparing;return true;}
 void Prepared(bool okay){phase=okay?Ready:Failed;}
 bool Record(std::uint64_t token){if(phase!=Ready)return false;present=token;phase=Recorded;return true;}
 bool CanSubmit(std::uint64_t token)const{return phase==Recorded&&token>present;}
 void Submit(bool okay){phase=okay?Submitted:Failed;}
 bool Retire(std::uint64_t completed){if(phase!=Submitted)return false;if(completed==UINT64_MAX){phase=Failed;return false;}if(completed<1)return false;phase=Exporting;return true;}
 void Exported(){if(phase==Exporting)phase=Idle;}
};
// Returns 2 while waiting. Both readback slots must be ready before any copy.
inline unsigned SelectInputCapturePair(InputCapturePolicy::Phase first,InputCapturePolicy::Phase second,
 std::uint64_t firstFrame,std::uint64_t frame){
 using P=InputCapturePolicy;
 if(first==P::Requested)return 0;
 if(second==P::Requested)return 1;
 if(first==P::Ready&&second==P::Ready)return 0;
 if(second==P::Ready&&(first==P::Recorded||first==P::Submitted||first==P::Exporting||first==P::Idle)&&firstFrame&&frame>firstFrame)return 1;
 return 2;
}
}
