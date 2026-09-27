#include "../../src/rr_user_control.h"
#include "../../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(RRUserDirectDlfParity());
 assert(control_rr_clamp::Requested()==60);
 assert(control_rr_clamp::Normalize(60)==60);
 RRUserSetDirectDlfParity(true);assert(RRUserDirectDlfParity());
 RRUserSetDirectDlfParity(false);assert(!RRUserDirectDlfParity());
 std::puts("PASS PT1 DLF A/B policy: RenoDX six-shader mode defaults ON and remains toggleable; specular clamp remains 60");
}
