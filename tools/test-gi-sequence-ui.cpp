#define CONTROL_RR_GI_SEQUENCE
#define CONTROL_GI_BOUNDARY_CAPTURE
#include "../src/rr_capture_ui.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){namespace ui=control_rr_capture_ui;using P=control_rr::InputCapturePolicy;
 for(auto state:{ui::State::Idle,ui::State::Queued,ui::State::Preparing,ui::State::Capturing,ui::State::Saving,ui::State::Complete,ui::State::Failed}){
  ui::state=state;
  assert(ui::ActionAllowed(true)); // SAVE LOG / PACKAGE RR always reachable.
  assert(ui::ActionAllowed(false)==!ui::Busy()); // No lighting mutation mid-capture.
  assert(ui::state==state); // Recovery must not cancel or free in-flight work.
 }
 ui::state=ui::State::Idle;
 assert(kRRCapturePairs==4);assert(ui::Request());assert(!ui::Request());
 for(unsigned pair=1;pair<=4;++pair){
  assert(ui::requested.exchange(false));assert(control_rr_gi::boundaryRequested);
  ui::Publish(P::Ready,P::Ready,pair,false);assert(ui::Busy());
  assert(!control_rr::InputCapturePairMayExport(P::Submitted,P::Ready,false));
  ui::Publish(P::Exporting,P::Submitted,pair,true);assert(ui::Busy());
  ui::Publish(P::Idle,P::Idle,pair,true);assert(ui::completedPairs==pair);
  if(pair<4){assert(ui::Busy()&&ui::requested);assert(!ui::Request());}
 }
 assert(ui::state==ui::State::Complete&&!ui::Busy()&&!ui::requested&&!control_rr_gi::boundaryRequested);
 assert(!ui::Request());
 ui::completedPairs=0;ui::state=ui::State::Idle;assert(ui::Request());ui::requested=false;
 ui::Publish(P::Idle,P::Idle,1,false);assert(ui::state==ui::State::Failed&&!ui::requested);assert(!ui::Request());
 puts("PASS R263 sequence: one click queues four pairs, no duplicate clicks, no export before pair recorded, explicit failure, hard session limit");
}
