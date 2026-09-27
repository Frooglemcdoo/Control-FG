#include "../../src/rr_user_control.h"
#include "../../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(!RRUserDirectDlfParity());
 assert(control_rr_clamp::Requested()==60);
 assert(control_rr_clamp::Normalize(60)==60);
 RRUserSetDirectDlfParity(true);assert(RRUserDirectDlfParity());
 RRUserSetDirectDlfParity(false);assert(!RRUserDirectDlfParity());
 std::puts("PASS GI30 DLF A/B policy: clean baseline defaults OFF; six-shader mode remains toggleable; specular clamp remains 60");
}
