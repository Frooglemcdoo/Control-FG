#include "../../src/rr_user_control.h"
#include "../../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(RRUserDirectDlfParity());
 assert(control_rr_clamp::Requested()==60);
 assert(control_rr_clamp::Normalize(60)==60);
 RRUserSetDirectDlfParity(false);assert(!RRUserDirectDlfParity());
 RRUserSetDirectDlfParity(true);assert(RRUserDirectDlfParity());
 std::puts("PASS GI29 direct DLF policy: six-shader mode default ON; user specular clamp remains 60");
}
