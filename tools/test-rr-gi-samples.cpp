#include <initializer_list>
#include "../src/rr_gi_sample_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using control_rr::GISampleComparison;
 for(unsigned n: {0u,1u,2u,4u,16u,0xffffffffu}){
  assert(GISampleComparison(n,0,true)==n);
  assert(GISampleComparison(n,2,false)==n);
  assert(GISampleComparison(n,3,true)==n);
  assert(GISampleComparison(n,2,true)==(n==1?2:n));
 }
 puts("PASS: GI sample comparison preserves native counts outside full-RR one-to-two case");
}
