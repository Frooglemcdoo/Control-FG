#include "../src/settings_check_state.h"
#include <cassert>
#include <cstring>
int main(){
 using namespace settings_check;
 RecordFailure("inactive",1,2,3,4,5,true);assert(failureCount==0);
 Record(RRSuccess);RecordGISamples(4);assert(counts[RRSuccess]==0&&giSampleMask==0);
 const auto first=Begin();RecordGISamples(4);RecordGISamples(8);assert(giSampleMask==((1u<<4)|(1u<<8)));Record(RRSuccess);RecordFor(RelaxCompleted,first);
 RecordFailure("native_camera_frame_changed",7,8,6,2,1,false);
 assert(failureCount==1&&!strcmp(failures[0].reason,"native_camera_frame_changed")&&failures[0].frame==7&&failures[0].guideFrame==6&&!failures[0].sameView);
 assert(counts[RRSuccess]==1&&counts[RelaxCompleted]==1);
 End();Record(RRSuccess);RecordFor(RelaxCompleted,first);assert(counts[RRSuccess]==1&&counts[RelaxCompleted]==1);
 RecordFailure("inactive",1,2,3,4,5,true);assert(failureCount==1);
 const auto second=Begin();assert(failureCount==0&&failureDropped==0);assert(second!=first&&counts[RRSuccess]==0&&giSampleMask==0);
 RecordFor(RelaxCompleted,first);assert(counts[RelaxCompleted]==0);
 for(unsigned i=0;i<failureCapacity+2;++i)RecordFailure("bounded",1,2,3,4,5,true);
 assert(failureCount==failureCapacity&&failureDropped==2);
 RecordFor(RelaxCompleted,second);assert(counts[RelaxCompleted]==1);
 assert(!strcmp(Verdict(true,0),"NOT_OBSERVED"));assert(!strcmp(Verdict(true,3),"OBSERVED"));
 assert(!strcmp(Verdict(true,3,1),"CHECK_FAILED"));assert(!strcmp(Verdict(false,1),"UNEXPECTED_ACTIVITY"));
 assert(!strcmp(Verdict(false,0),"OFF"));End();
}
