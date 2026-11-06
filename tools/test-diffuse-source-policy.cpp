#include "../src/rr_broad_diffuse_policy.h"
#include "../src/rr_diffuse_source_policy.h"
#include <cassert>
#include <iostream>
int main(){
 assert(!control_rr_diffuse_source::legacyBypass.load());
 unsigned cases=0;
 for(unsigned effects=0;effects<64;++effects)
 for(bool full:{false,true})for(bool partial:{false,true})
 for(bool diagnostic:{false,true})for(bool legacy:{false,true}){
  const bool preserve=control_rr_diffuse_source::Preserve(diagnostic,legacy);
  const auto policy=control_rr::BroadDiffusePolicy(full,partial,effects,preserve);
  const bool oldBypass=!diagnostic&&full&&!partial&&(effects&(4u|16u));
  assert(policy.bypass==(legacy&&oldBypass));
  // In gameplay all combinations retain the original call; legacy comparison
  // retains the old behavior, including explicit diagnostic preserve priority.
  if(!legacy)assert(!policy.bypass);
  ++cases;
 }
 control_rr_diffuse_source::legacyBypass.store(true);
 assert(!control_rr_diffuse_source::Preserve(false));
 assert(control_rr_diffuse_source::Preserve(true));
 control_rr_diffuse_source::legacyBypass.store(false);
 assert(control_rr_diffuse_source::Preserve(false));
 std::cout<<"PASS "<<cases<<" diffuse-source policy cases: default native filtering, bounded legacy comparison, diagnostic priority, restore\n";
}
