#include "../src/rr_capture_ui.h"
#include "../src/rr_input_capture_settle.h"
#include <cassert>
#include <iostream>
int main(){
 control_rr::InputCaptureSettle settle;
 assert(!settle.Ready(100,false,1000));
 assert(!settle.Ready(101,false,2000));
 assert(!settle.Ready(102,false,5999));
 assert(settle.Ready(103,false,6000));
 assert(!settle.Ready(200,false,7000)); // menu gap restarts the timer
 assert(!settle.Ready(201,true,9000)); // an explicit history reset restarts it
 assert(!settle.Ready(202,false,10000));
 assert(settle.Ready(203,false,14000));
 settle.Reset();assert(!settle.Ready(204,false,15000)); // failed evaluation / new request

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
