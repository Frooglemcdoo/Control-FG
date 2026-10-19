#include <initializer_list>
#include "../src/rr_gi_sample_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using control_rr::GISampleComparison;
 for(unsigned requested: {0u,1u,2u,3u,4u,8u,16u,0xffffffffu}){
  const bool accepted=requested==2||requested==4;
  assert(control_rr::NormalizeGISampleRequest(requested)==(accepted?requested:0u));
  for(unsigned n: {0u,1u,2u,4u,16u,0xffffffffu}){
   assert(GISampleComparison(n,requested,false)==n);
   assert(GISampleComparison(n,requested,true)==(accepted&&n==1?requested:n));
  }
 }
 puts("PASS: GI comparison accepts only 2/4; preserves native counts and RR-off behavior");
}
