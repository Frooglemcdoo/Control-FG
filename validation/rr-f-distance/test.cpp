#include "../../src/rr_distance_binding_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 for(unsigned ready=0;ready<2;++ready)for(unsigned projection=0;projection<2;++projection)
 for(unsigned preset=0;preset<14;++preset)
  assert(control_rr::DistanceEligible(ready!=0,projection!=0,preset,6)==(ready&&projection&&preset==6));
 control_rr::DistanceBindingHistory h;
 assert(!h.Update(false));assert(h.Update(true));
 for(unsigned i=0;i<10000;++i)assert(!h.Update(true));
 assert(h.Update(false));assert(!h.Update(false));assert(h.Update(true));
 puts("PASS D1 F-only admission and resets only when binding availability changes");
}
