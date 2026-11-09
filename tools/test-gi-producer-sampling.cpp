#include "../src/gi_reuse/producer_sampling.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace gi_reuse;
static void Require(bool v){if(!v)throw std::runtime_error("proposal sampling contract");}
int main()try{
 // Independently implement the published R307 32-bit permutation for comparison.
 auto reference=[](uint x,uint y,uint f,uint b,uint ray,uint salt){
  std::uint32_t z=(x*73856093u)^(y*19349663u)^(f*83492791u)^(b*2654435761u)^(ray*1597334677u)^salt;
  z=(z^(z>>16))*0x7feb352du;z=(z^(z>>15))*0x846ca68bu;z^=z>>16;
  return float(z/256u)/16777216.0f;
 };
 for(uint count:{1u,2u,3u,6u,16u,63u,64u}){
  double u=0,v=0,w=0,s=0,vw=0,ws=0;std::uint64_t n=0;
  for(uint frame=0;frame<4096;++frame)for(uint ray=0;ray<count;++ray){
   auto r=GIProducerSample(479,281,frame,7,ray,count);
   Require(r.valid&&r.u>=0&&r.u<1&&r.v>=0&&r.v<1&&r.range>=0&&r.range<1&&r.selection>=0&&r.selection<1);
   const uint cols=(count&1)?1:2,rows=count/cols;
   Require(r.u>=float(ray%cols)/float(cols)&&r.u<=float(ray%cols+1)/float(cols));
   Require(r.v>=float(ray/cols)/float(rows)&&r.v<=float(ray/cols+1)/float(rows));
   Require(r.range==reference(479,281,frame,7,ray,0x967a889bu));
   Require(r.selection==reference(479,281,frame,7,ray,0x368cc8b7u));
   float expectedU=(float(ray%cols)+reference(479,281,frame,7,ray,0x68bc21ebu))/float(cols);
   float expectedV=(float(ray/cols)+reference(479,281,frame,7,ray,0x02e5be93u))/float(rows);
   Require(std::abs(r.u-expectedU)<1e-7f&&std::abs(r.v-expectedV)<1e-7f);
   u+=r.u;v+=r.v;w+=r.range;s+=r.selection;vw+=double(r.v)*r.range;ws+=double(r.range)*r.selection;++n;
  }
  const double total=double(n);
  Require(std::abs(u/total-.5)<.02&&std::abs(v/total-.5)<.02&&std::abs(w/total-.5)<.02&&std::abs(s/total-.5)<.02);
  Require(std::abs(vw/total-v*w/(total*total))<.01&&std::abs(ws/total-w*s/(total*total))<.01);
 }
 Require(!GIProducerSample(0,0,0,0,0,0).valid&&!GIProducerSample(0,0,0,0,0,65).valid&&!GIProducerSample(0,0,0,0,6,6).valid);
 const auto a=GIProducerSample(479,281,1,7,0,6),b=GIProducerSample(479,281,257,7,0,6);
 Require(a.u!=b.u||a.v!=b.v); // no 256-frame repeat
 std::cout<<"PASS complete 1..64-ray strata, R307 coordinate identity, range/selection dimensions and frame progression\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
