#include "rr_reflection_present.h"
#include "rr_reflection_capture.h"
#include <cassert>
#include <iostream>
using namespace control_rr_reflection;
struct Backend {
 Context context{90001,90,91,92,93};
 unsigned held[3]{},released=0,signals=0;std::size_t selected=0;
 std::uint64_t completed=0;bool failSignal=false;
 bool Current(const Context& c){return Same(c,context);}
 bool Destination(Address,const Shape&,Address){return true;}
 void Select(std::size_t s){selected=s;}
 bool HoldSources(Address,Address){held[selected]=2;return true;}
 bool Barriers(const Transition*,std::size_t){return true;}
 bool Copy(Address,Address){return true;}
 void ReleaseSources(std::size_t s){assert(held[s]==2);held[s]=0;++released;}
 bool Signal(control_rr::FenceKey,std::uint64_t){++signals;return !failSignal;}
 std::uint64_t Completed(control_rr::FenceKey){return completed;}
};
int main(){
 PresentLedger p;assert(!p.Covered({0,1},11));assert(!p.Record({3,1},10));assert(!p.Record({0,0},10));
 assert(p.Record({0,1},10));assert(!p.Covered({0,1},10)&&!p.Covered({0,1},9));assert(p.Covered({0,1},11));
 assert(!p.Covered({0,2},11));assert(p.Record({0,2},11));assert(!p.Covered({0,1},100));assert(!p.Covered({0,2},11));
 assert(p.Covered({0,2},20)); // skipped owner lock can delay signaling safely
 assert(p.Record({1,5},0));assert(p.Covered({1,5},1));assert(!p.Record({2,8},UINT64_MAX));
 Backend b;Capture<Backend> c(b);assert(c.Bind({92,91,94}));
 Address destinations[3][2]={{100,101},{102,103},{104,105}};
 Snapshot s{};s.context=b.context;s.material.resource=80;s.position.resource=81;s.material.state=s.position.state=0xc0;
 s.material.shape.width=s.position.shape.width=1280;s.material.shape.height=s.position.shape.height=720;
 control_rr::Lease lease;assert(c.Record(s,1,destinations,lease));assert(p.Record(lease,12));
 assert(!p.Covered(lease,12));assert(!c.SubmitCaptured({lease.index,lease.serial+1})&&!b.signals);
 assert(p.Covered(lease,13));assert(c.SubmitCaptured(lease));assert(!c.SubmitCaptured(lease)&&b.signals==1);
 assert(c.Collect()&&!b.released);b.completed=1;assert(c.Collect()&&b.released==1&&c.Idle());
 ++b.context.frame;s.context=b.context;assert(c.Record(s,1,destinations,lease));assert(p.Record(lease,13));
 assert(!p.Covered(lease,13));b.failSignal=true;assert(!c.SubmitCaptured(lease)&&c.Stopped());
 assert(!c.Collect()&&b.released==1); // retain second frame on ambiguous Signal failure
 std::cout<<"PASS independent frame/Present counters, slot serial reuse, delayed signaling, exactly-once submission and fault retention\n";
}
