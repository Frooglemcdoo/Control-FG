#include "../src/rr_performance_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr_perf;
 Key off{};off.width=off.outputWidth=3840;off.height=off.outputHeight=2160;
 Stability s;double ms=0;
 for(unsigned f=1;f<=201;++f){bool okay=s.Observe(off,f,f*20,1000,false,true,ms);assert(okay==(f>=151));}
 Key warm=off;warm.requested=true;
 for(unsigned f=202;f<=402;++f)assert(!s.Observe(warm,f,f*20,1000,false,true,ms));
 Key rr=warm;rr.rr=true;rr.ready=true;rr.giRequest=4;
 for(unsigned f=403;f<=603;++f){bool okay=s.Observe(rr,f,f*20,1000,false,true,ms);assert(okay==(f>=553));}
 auto segment=s.segment;rr.giRequest=8;
 assert(!s.Observe(rr,604,12080,1000,false,true,ms)&&s.segment==segment+1);
 puts("PASS: steady SR eligible; RR warmup excluded; RR steady eligible; GI selection starts fresh segment");
}
