#include <cassert>
#include "../../src/rr_broad_diffuse_policy.h"
int main(){
  using control_rr::BroadDiffusePolicy;
  assert(!BroadDiffusePolicy(false,false,4u|16u).bypass);
  assert(!BroadDiffusePolicy(true,true,4u|16u).bypass);
  assert(!BroadDiffusePolicy(true,false,0u).bypass);
  assert(BroadDiffusePolicy(true,false,4u).bypass);
  assert(BroadDiffusePolicy(true,false,16u).bypass);
  assert(BroadDiffusePolicy(true,false,4u|16u).bypass);
  return 0;
}
