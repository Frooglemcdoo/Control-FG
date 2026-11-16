#include "../src/gi_reuse/pilot_guide_resolve.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace gi_reuse;
int main(){
 // Exhaustive distribution, not sampled expected output: pilot p=1/2, guided
 // q=.9*p+.1*(.9,.1). Includes black visibility and strongly unequal RGB.
 for(unsigned n:{2u,4u,10u})for(float alpha:{0.f,.05f,.1f}){
  if(alpha>1.f/n)continue;
  const double q=.5+.4*alpha;double expected[3]{};double probability=0;
  for(unsigned bits=0;bits<(1u<<n);++bits){
   auto color=[](bool high){return high?GIPoint{100,4,0}:GIPoint{0,.2f,2};};
   auto s=GIPilotBegin(color(bits&1),n,alpha,19);double p=.5;
   for(unsigned i=1;i<n;++i){bool high=(bits>>i)&1;double qi=high?q:1-q;p*=qi;s=GIPilotAdd(s,color(high),float(.5/qi),19);}
   auto r=GIPilotFinish(s);assert(r.valid&&r.rgb.x>=0&&r.rgb.y>=0&&r.rgb.z>=0);
   expected[0]+=p*r.rgb.x;expected[1]+=p*r.rgb.y;expected[2]+=p*r.rgb.z;probability+=p;
  }
  assert(std::abs(probability-1)<1e-10);assert(std::abs(expected[0]-50)<.00003);assert(std::abs(expected[1]-2.1)<.000003);assert(std::abs(expected[2]-1)<.000003);
 }
 // Constant illumination survives adversarial mixtures of zero and maximal weights.
 for(unsigned n:{2u,4u,10u,64u})for(unsigned pattern=0;pattern<3;++pattern){
  auto s=GIPilotBegin({.25f,2,100},n,1.f/n,4);
  for(unsigned i=1;i<n;++i)s=GIPilotAdd(s,{.25f,2,100},pattern==0?0:pattern==1?s.maxWeight:(i%2?s.maxWeight:0),4);
  auto r=GIPilotFinish(s);assert(r.valid);assert(std::abs(r.rgb.x-.25f)<1e-6&&std::abs(r.rgb.y-2)<1e-5&&std::abs(r.rgb.z-100)<.0002);
 }
 auto s=GIPilotBegin({1,1,1},10,.1f,7);
 assert(!GIPilotFinish(s).valid);assert(!GIPilotAdd(s,{1,1,1},1,8).valid);
 assert(!GIPilotAdd(s,{1,1,1},2,7).valid);assert(!GIPilotAdd(s,{-1,1,1},1,7).valid);
 assert(!GIPilotAdd(s,{1,1,1},std::numeric_limits<float>::infinity(),7).valid);
 assert(!GIPilotBegin({1,1,1},10,.2f,7).valid);
 std::puts("PASS exhaustive RGB expectation, nonnegative resolve, constant field, version/count/weight rejection. Research-only; no game integration.");
}
