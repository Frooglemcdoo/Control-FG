#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../src/rr_contact_shadow_policy.h"
#include "../src/rr_broad_diffuse_policy.h"
int main(){
 for(bool full:{false,true})for(bool enabled:{false,true})
 for(bool temporal:{false,true})for(int size:{-1,0,1,3,5,9,11})
 for(int step:{0,1,2,4,8}){
  const auto p=control_rr::ContactShadowFilterPolicy(full,enabled,temporal,size,step);
  assert(!p.neutralized&&p.temporal==temporal&&p.spatialSize==size&&p.spatialStep==step);
 }
 for(bool full:{false,true})for(bool partial:{false,true})
 for(unsigned effects=0;effects<=127;++effects)
  assert(!control_rr::BroadDiffusePolicy(full,partial,effects).bypass);
 puts("PASS R50 preserves native contact-shadow arguments and broad DLF across all RT combinations");
}
