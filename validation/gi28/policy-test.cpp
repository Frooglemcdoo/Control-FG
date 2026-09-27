#include "../../src/rr_user_control.h"
#include "../../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr;
 assert(RRUserDiffuseClampRenoDX());
 assert(!RRUserContactShadowRenoDX());
 assert(control_rr_clamp::Requested()==60);
 assert(control_rr_clamp::Normalize(60)==60);
 RRUserSetDiffuseClampRenoDX(false); assert(!RRUserDiffuseClampRenoDX());
 RRUserSetDiffuseClampRenoDX(true); assert(RRUserDiffuseClampRenoDX());
 RRUserSetContactShadowRenoDX(true); assert(RRUserContactShadowRenoDX());
 RRUserSetContactShadowRenoDX(false); assert(!RRUserContactShadowRenoDX());
 std::puts("PASS GI28 parity controls: diffuse RenoDX default ON, contact-shadow Current Mod default, specular clamp unchanged at 60");
}
