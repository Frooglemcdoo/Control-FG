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
 std::puts("PASS GI32 DLF A/B policy: direct RenoDX DLF defaults ON; mode remains toggleable; specular clamp remains 60");
}
