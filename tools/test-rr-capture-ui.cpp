#include "../src/rr_capture_ui.h"
#include <cassert>
#include <iostream>
int main(){
 namespace ui=control_rr_capture_ui;using P=control_rr::InputCapturePolicy;
 assert(ui::Request());assert(!ui::Request());
 ui::Publish(P::Idle,P::Idle,0,false);assert(ui::state==ui::State::Queued);
 assert(ui::requested.exchange(false));
 ui::Publish(P::Requested,P::Requested,1,false);assert(ui::state==ui::State::Capturing);
 ui::Publish(P::Preparing,P::Ready,1,false);assert(ui::state==ui::State::Preparing);
 ui::Publish(P::Exporting,P::Submitted,1,true);assert(ui::state==ui::State::Saving);
 ui::Publish(P::Idle,P::Idle,1,true);assert(ui::state==ui::State::Complete && ui::completedPairs==1);
 assert(ui::Request());ui::requested=false;
 ui::Publish(P::Idle,P::Idle,2,true);assert(ui::completedPairs==2 && !ui::Request());
 ui::Publish(P::Failed,P::Idle,2,false);assert(ui::state==ui::State::Failed);
 ui::completedPairs=0;assert(!ui::Request());
 ui::Publish(P::Idle,P::Idle,1,false);assert(ui::state==ui::State::Failed);
 std::cout<<"PASS R152 overlay capture: queued requests survive idle polling, duplicate clicks reject, saved pairs require outputs, failures remain explicit\n";
}
