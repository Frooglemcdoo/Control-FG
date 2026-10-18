#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../src/rr_contact_shadow_policy.h"
int main(){
 for(bool full:{false,true})for(bool enabled:{false,true})
 for(bool temporal:{false,true})for(int size:{-1,0,1,3,5,9,11})
 for(int step:{0,1,2,4,8}){
  const auto p=control_rr::ContactShadowFilterPolicy(full,enabled,temporal,size,step);
  assert(!p.neutralized);
  assert(p.temporal==temporal&&p.spatialSize==size&&p.spatialStep==step);
 }
 puts("PASS R13 contact shadow filter preserves every native argument in RR/off/fallback modes");
}
