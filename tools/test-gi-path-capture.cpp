#define CONTROL_RR_GI_SEQUENCE
#define CONTROL_GI_BOUNDARY_CAPTURE
#include "../src/rr_capture_ui.h"
#include <cassert>
#include <iostream>
int main(){
 namespace ui=control_rr_capture_ui;using P=control_rr::InputCapturePolicy;
 using S=control_rr_gi::StageMode;
 // R377 failed at frame 9108 despite one temporal + all six Stable spatial passes.
 assert(gi_path_capture::BoundaryMatches(2,true,true,9108,9108,S::BothSteadySpatial,6,6));
 assert(gi_path_capture::BoundaryMatches(1,true,true,8039,8039,S::RawBypass,0,0));
 assert(gi_path_capture::BoundaryMatches(0,true,true,8039,8039,S::RawBypass,0,0));
 for(unsigned condition:{0u,1u})assert(!gi_path_capture::BoundaryMatches(condition,true,true,9108,9108,S::BothSteadySpatial,6,6));
 assert(!gi_path_capture::BoundaryMatches(2,true,true,9108,9108,S::RawBypass,0,0));
 assert(!gi_path_capture::BoundaryMatches(2,true,true,9108,9108,S::BothSteadySpatial,6,5));
 assert(!gi_path_capture::BoundaryMatches(2,true,true,9108,9109,S::BothSteadySpatial,6,6));
 assert(!gi_path_capture::BoundaryMatches(2,true,false,9108,9108,S::BothSteadySpatial,6,6));
 assert(!gi_path_capture::BoundaryMatches(2,false,true,9108,9108,S::BothSteadySpatial,6,6));
 assert(!gi_path_capture::BoundaryMatches(2,true,true,9108,9108,S::BothSteadySpatial,0,0));
 assert(gi_path_capture::BoundaryMatches(3,true,true,9108,9108,S::RawBypass,0,0));
 assert(!gi_path_capture::BoundaryMatches(3,true,true,9108,9108,S::BothSteadySpatial,6,6));
 assert(gi_path_capture::BoundaryMatches(4,true,true,10,10,S::Both,6,6));
 assert(!gi_path_capture::BoundaryMatches(4,true,true,10,10,S::RawBypass,0,0));
 assert(!gi_path_capture::BoundaryMatches(4,true,true,10,10,S::Both,6,5));
 assert(!gi_path_capture::BoundaryMatches(4,true,true,10,11,S::Both,6,6));
 assert(!gi_path_capture::BoundaryMatches(4,true,false,10,10,S::Both,6,6));
 assert(!gi_path_capture::BoundaryMatches(4,true,true,10,10,S::Both,0,0));
 assert(gi_path_capture::ExportStride(4)==1);
 assert(gi_path_capture::PairAligned(4,1,10,11));
 assert(!gi_path_capture::PairAligned(4,1,10,12));
 assert(gi_path_capture::ExportStride(3)==1 && gi_path_capture::ExportStride(1)==4);
 assert(gi_path_capture::PairAligned(3,0,0,10));
 assert(gi_path_capture::PairAligned(3,1,10,11));
 assert(!gi_path_capture::PairAligned(3,1,10,12));
 assert(!gi_path_capture::PairAligned(3,1,0,12));
 assert(gi_path_capture::Extent(3840,4)==960 && gi_path_capture::Extent(2161,4)==541);
 gi_path_capture::enabled=true;
 for(unsigned n=1;n<=2;++n){
  assert(ui::Request());assert(!ui::automaticPairs);ui::requested=false;
  ui::Publish(P::Ready,P::Ready,n,false);assert(ui::Busy());
  ui::Publish(P::Idle,P::Idle,n,true);assert(ui::state==ui::State::Complete);
  assert(!ui::requested && !control_rr_gi::boundaryRequested);
 }
 gi_path_capture::enabled=false;
 // Normal per-present polling after restoration must NOT launch extra captures.
 for(unsigned i=0;i<100;++i)ui::Publish(P::Idle,P::Idle,2,true);
 assert(!ui::requested && !ui::Busy());
 assert(ui::Request());assert(ui::automaticPairs);ui::requested=false;
 ui::Publish(P::Idle,P::Idle,3,true);assert(ui::requested && ui::state==ui::State::Queued);
 std::cout<<"PASS bounded manual pairs, no post-finish rearming, ordinary capture restored\n";
}
