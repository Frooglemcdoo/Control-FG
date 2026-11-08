#include "../src/settings_check_state.h"
#include <cassert>
#include <cstring>
int main(){
 using namespace settings_check;
 Record(RRSuccess);assert(counts[RRSuccess]==0);
 const auto first=Begin();Record(RRSuccess);RecordFor(RelaxCompleted,first);
 assert(counts[RRSuccess]==1&&counts[RelaxCompleted]==1);
 End();Record(RRSuccess);RecordFor(RelaxCompleted,first);assert(counts[RRSuccess]==1&&counts[RelaxCompleted]==1);
 const auto second=Begin();assert(second!=first&&counts[RRSuccess]==0);
 RecordFor(RelaxCompleted,first);assert(counts[RelaxCompleted]==0);
 RecordFor(RelaxCompleted,second);assert(counts[RelaxCompleted]==1);
 assert(!strcmp(Verdict(true,0),"NOT_OBSERVED"));assert(!strcmp(Verdict(true,3),"OBSERVED"));
 assert(!strcmp(Verdict(true,3,1),"CHECK_FAILED"));assert(!strcmp(Verdict(false,1),"UNEXPECTED_ACTIVITY"));
 assert(!strcmp(Verdict(false,0),"OFF"));End();
}
