#include "../src/rr_input_capture_policy.h"
#include "../src/gi_reuse/motion_capture_layout.h"
#include <cassert>
#include <array>
#include <iostream>
int main(){
 using P=control_rr::InputCapturePolicy;constexpr unsigned N=16;
 std::array<P::Phase,N> phases;std::array<std::uint64_t,N> frames{};phases.fill(P::Requested);
 for(unsigned i=0;i<N;++i){assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,100)==i);phases[i]=P::Preparing;}
 assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,100)==N);
 phases.fill(P::Ready);
 for(unsigned i=0;i<N;++i){
  assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,100+i)==i);
  if(i)assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,101+i)==N);
  frames[i]=100+i;phases[i]=P::Submitted;
 }
 assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,116)==N);
 phases[9]=P::Failed;phases[10]=P::Ready;assert(control_rr::SelectInputCaptureSequence(phases.data(),frames.data(),N,110)==N);
 assert(gi_motion::Addressable(2560,1440,4,256ull<<20));
 assert(!gi_motion::Addressable(3840,2160,4,256ull<<20));
 assert(!gi_motion::Addressable(2560,1440,10,256ull<<20));
 assert(gi_motion::Bytes(3,2,4)==2048+3*2*4*224);
 assert(gi_motion::Reservation(2560,1440,4,400ull<<20)>(60ull<<30));
 std::cout<<"motion sequence, gap, failure and address/memory budget tests passed\n";
}
