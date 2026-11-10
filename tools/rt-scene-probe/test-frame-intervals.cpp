#include "../../src/rt_frame_intervals.h"
#include <cassert>
#include <cstdio>
int main(){rt_frame_intervals::Window window;std::uint64_t expected=0;
 for(unsigned i=0;i<300;++i){const auto duration=i==15?200000u:i==121?60000u:i==250?40000u:i==272?30000u:10000u;expected+=duration;window.Add({i*1000000ull,(i+1)*1000000ull,i+1,duration,i==15?77u:0u});}
 assert(window.observerUs==77&&window.top[0].observerUs==77);
 assert(window.count==300&&window.totalUs==expected);assert(window.over25==4&&window.over33==3&&window.over50==2&&window.over100==1);
 assert(window.top[0].present==16&&window.top[1].present==122&&window.top[2].present==251&&window.top[3].present==273);assert(window.top[0].begin==15000000&&window.top[0].end==16000000);
 window={};window.Add({1,2,301,25000});assert(window.count==1&&window.over25==1&&window.over33==0&&window.top[1].us==0);
 std::puts("PASS engine interval windows: threshold counts, top-four QPC spans, ordering and reset. Not a GPU or generated-frame measurement.");}
