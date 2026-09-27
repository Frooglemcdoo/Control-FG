#include "../../src/rr_user_control.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(RRUserDirectDlfParity());
 assert(RRUserJitterModeValue()==RRJitterMode::Control);
 assert(!RRUserJitterRR1024());
 assert(RRUserDiffuseSamples()==0);
 auto g=RRUserImageQualityGeneration();
 RRUserSetJitterMode(RRJitterMode::RR1024);assert(RRUserJitterRR1024());assert(RRUserImageQualityGeneration()==g+1);g++;
 RRUserSetJitterMode(RRJitterMode::RR1024);assert(RRUserImageQualityGeneration()==g);
 for(unsigned v=1;v<=16;++v){RRUserSetDiffuseSamples(v);assert(RRUserDiffuseSamples()==v);}
 assert(RRUserDiffuseSamples()==16);
 RRUserSetDiffuseSamples(99);assert(RRUserDiffuseSamples()==16);
 RRUserSetDiffuseSamples(0);assert(RRUserDiffuseSamples()==0);
 RRUserPublishDiffuseSamples(1,8);assert(RRUserDiffuseSamplesNative()==1&&RRUserDiffuseSamplesEffective()==8);
 std::puts("PASS GI33 controls: live Control/RR1024 jitter, Native plus every RT diffuse sample count 1-16, quality-generation resets");
}
