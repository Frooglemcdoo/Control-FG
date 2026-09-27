#include "../../src/rr_specular_mv_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr_specmv;
 assert(!RequestedForPreset(false,6,6));
 assert(!RequestedForPreset(true,5,6));
 assert(RequestedForPreset(true,6,6));
 assert(!BindingChanged(false,false));
 assert(BindingChanged(false,true));
 assert(BindingChanged(true,false));
 std::puts("PASS GI27 specular-MV policy: Model F only, explicit A/B request, binding transitions reset history");
}
