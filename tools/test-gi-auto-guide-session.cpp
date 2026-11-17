#include "../src/gi_reuse/auto_guide_session.h"
#include <cassert>
#include <iostream>
int main(){using namespace gi_reuse;GIAutoGuideSession session;
 assert(!session.Admit(0,2)&&!session.Admit(1,0));
 const auto first=session.Admit(1,2);assert(first==1);
 // Completed targets lag several recording frames. Native heap epochs advance
 // independently; using them as target versions recreated an empty buffer.
 for(std::uint32_t frame=10;frame<6000;++frame){
  const auto current=session.Admit(1,2),priorVersion=first;
  const auto oldHeapEpoch=frame-4,newHeapEpoch=frame;
  assert(current==first&&GIAutoPriorCompatible(true,priorVersion,frame-4,current,frame));
  assert(!GIAutoPriorCompatible(true,oldHeapEpoch,frame-4,newHeapEpoch,frame));
 }
 assert(!session.Admit(3,2)&&!session.Admit(1,4)&&!session.Admit(3,4));
 assert(session.Admit(1,2)==first); // rejection must not mutate the session
 assert(!GIAutoPriorCompatible(false,1,4,1,8));
 assert(!GIAutoPriorCompatible(true,2,4,1,8));
 assert(!GIAutoPriorCompatible(true,1,8,1,8));
 assert(!GIAutoPriorCompatible(true,1,9,1,8));
 assert(!GIAutoPriorCompatible(true,1,0,1,8));
 assert(!GIAutoPriorCompatible(true,0,4,0,8));
 std::cout<<"PASS stable guide version over changing native recording epochs; device/queue identity, future/empty/wrong-version rejection.\n";
}
