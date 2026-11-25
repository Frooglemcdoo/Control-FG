#include "../src/rr_input_capture_policy.h"
#include "../src/gi_reuse/motion_export_lease.h"
#include "../src/gi_reuse/motion_capture_layout.h"
#include <array>
#include <cassert>
#include <thread>
using P=control_rr::InputCapturePolicy;

static void replay(unsigned count){
 std::array<P,16> slots{};
 std::array<std::uint64_t,16> frames{};
 std::array<P::Phase,16> phases{};
 for(auto& s:slots){assert(s.Request()&&s.BeginPrepare());s.Prepared(true);}
 for(unsigned i=0;i<count;++i){
  for(unsigned j=0;j<16;++j)phases[j]=slots[j].phase;
  auto frame=6916u+i;
  assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),16,frame)==i);
  frames[i]=frame;assert(slots[i].Record(frame));
  assert(!slots[i].CanSubmit(frame)&&slots[i].CanSubmit(frame+1));
  slots[i].Submit(true);assert(!slots[i].Retire(0));
 }
 // The last eight slots never receive an RR evaluation in the interrupted run.
 for(unsigned i=0;i<count;++i){assert(slots[i].Retire(1));slots[i].Exported();assert(slots[i].phase==P::Idle);}
 for(unsigned i=count;i<16;++i)assert(slots[i].phase==P::Ready);
 if(count<16){
  for(unsigned j=0;j<16;++j)phases[j]=slots[j].phase;
  assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),16,6916+count+1)==16);
 }
}
int main(){
 replay(8);replay(16);
 P removed;removed.phase=P::Submitted;assert(!removed.Retire(UINT64_MAX)&&removed.phase==P::Failed);
 std::atomic<unsigned> count{0};
 std::array<gi_motion::ExportLease,gi_motion::InFlight> gpu,worker;
 for(unsigned i=0;i<gpu.size();++i){assert(gpu[i].Acquire(count,gpu.size()));worker[i]=gpu[i];}
 gi_motion::ExportLease probe;assert(!probe.Acquire(count,gpu.size()));
 gpu[0]={};assert(!probe.Acquire(count,gpu.size()));
 std::thread writer([&]{worker[0]={};});writer.join();
 assert(probe.Acquire(count,gpu.size()));worker={};assert(count==gpu.size());
 gpu={};assert(count==1);probe={};assert(count==0);
}
