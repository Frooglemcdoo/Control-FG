#include "transition.h"
#include <cstdio>
#include <cstdlib>
using namespace hdrbutton;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 State s{};
 CHECK(!s.active());
 CHECK(ButtonText(s.phase,false,true)[4]=='O');
 s.phase=Phase::RequestOff;
 CHECK(s.active());
 CHECK(ButtonText(s.phase,false,true)[3]==L'.');
 s.phase=Phase::WaitFreshDomain;
 CHECK(s.active());
 CHECK(kFreshDomainFramesRequired==3);
 CHECK(ThirtySecondStallIsSafe(1000));
 CHECK(!TransitionTimedOut(1000,31000));
 CHECK(!TransitionTimedOut(1000,61000));
 CHECK(TransitionTimedOut(1000,61001));
 s.sourceSelection=6;
 s.fail("forced_test_failure",62000);
 CHECK(!s.active());
 CHECK(s.phase==Phase::Failed);
 CHECK(s.sourceSelection==6);
 CHECK(s.phaseStartedMs==62000);
 CHECK(ButtonText(s.phase,false,true)[4]==L'E');
 s.phase=Phase::Complete;
 CHECK(!s.active());
 std::puts("PASS R34 transition policy: three fresh frames, 30s stall survives, 60s timeout, saved selection retained");
 return 0;
}
