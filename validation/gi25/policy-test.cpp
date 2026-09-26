#include "../../src/rr_responsivity_policy.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 using namespace control_rr_responsivity;
 assert(Normalize(-500)==-100&&Normalize(500)==100&&Normalize(-50)==-50);
 assert(std::fabs(MaskValue(-50)+0.5f)<0.0001f);
 assert(std::fabs(MaskValue(100)-1.0f)<0.0001f);
 assert(!Enabled(true,0)&&!Enabled(false,-50)&&Enabled(true,-50));
 assert(std::fabs(ClampFrameTimeMs(0.0f)-16.666667f)<0.001f);
 assert(std::fabs(ClampFrameTimeMs(0.2f)-1.0f)<0.001f);
 assert(std::fabs(ClampFrameTimeMs(200.0f)-100.0f)<0.001f);
 assert(std::fabs(ClampFrameTimeMs(13.4f)-13.4f)<0.001f);
 std::puts("PASS GI25 RR temporal policy: signed responsivity -100..100, zero disables mask, frame-time clamped 1..100 ms");
}
