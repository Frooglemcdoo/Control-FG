#include "../../src/rr_user_control.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(RRUserDirectDlfParity());
 assert(RRUserJitterModeValue()==RRJitterMode::Control);
 assert(!RRUserJitterRR1024());
 assert(RRUserDgiBounces()==0);
 auto g=RRUserImageQualityGeneration();
 RRUserSetJitterMode(RRJitterMode::RR1024);assert(RRUserJitterRR1024());assert(RRUserImageQualityGeneration()==g+1);g++;
 RRUserSetJitterMode(RRJitterMode::RR1024);assert(RRUserImageQualityGeneration()==g);
 for(unsigned v=1;v<=16;++v){RRUserSetDgiBounces(v);assert(RRUserDgiBounces()==v);}
 assert(RRUserDgiBounces()==16);
 RRUserSetDgiBounces(99);assert(RRUserDgiBounces()==16);
 RRUserSetDgiBounces(0);assert(RRUserDgiBounces()==0);
 RRUserPublishDgiBounces(6,12);assert(RRUserDgiNative()==6&&RRUserDgiEffective()==12);
 std::puts("PASS GI32 controls: live Control/RR1024 jitter, Native plus every DGI bounce 1-16, quality-generation resets");
}
