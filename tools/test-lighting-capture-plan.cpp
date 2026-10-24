#include "../src/lighting_capture_plan.h"
#include <cassert>
#include <set>
int main(){
 using namespace native_lighting;using namespace capture_plan;
 std::set<Mode> modes;unsigned gpu=0;
 for(auto step:steps){assert(modes.insert(step.mode).second);if(step.gpu){++gpu;assert(step.mode==Mode::RoomBoth);}}
 assert(gpu==1);assert(modes.count(Mode::Native)&&modes.count(Mode::RoomAuthoredClip));
 assert(modes.count(Mode::RoomAuthoredSpatial)&&modes.count(Mode::RoomAuthoredRaw));
 assert(!Settled(2499,100,true));assert(!Settled(4000,89,true));assert(!Settled(4000,100,false));assert(Settled(2500,90,true));
 assert(timeoutMs>settleMs+imagesPerStep*burstIntervalMs);
}
