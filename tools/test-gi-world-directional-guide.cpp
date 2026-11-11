#include "../src/gi_reuse/world_directional_guide.h"
#include <cassert>
#include <cstdio>
#include <random>
using namespace gi_reuse;
int main(){
 float energy[64]{};energy[63]=100;energy[2]=3;auto guide=GIBuildDirectionalGuide(energy,.5f);
 GIPoint normals[]={{0,0,1},{0,0,-1},{1,0,0},{0,1,0},{.6f,0,.8f}};
 std::mt19937 rng(424);auto random=[](std::mt19937& generator){return float(generator()>>8)*0.000000059604644775390625f;};
 for(auto n:normals)for(float fraction:{0.f,.125f,.5f}){
  constexpr unsigned count=500000;double sum[4]{},square[4]{};unsigned skipped=0;
  for(unsigned i=0;i<count;++i){float u=random(rng),v=random(rng);auto r=GISampleWorldGuide(guide,n,fraction,u,v);if(!r.valid){std::fprintf(stderr,"INVALID n %.9g %.9g %.9g fraction %.9g u %.9g v %.9g pdf %.9g draw %u\n",n.x,n.y,n.z,fraction,u,v,r.pdf,i);return 2;}assert(std::abs(GIDot(r.direction,r.direction)-1)<1e-5);assert(r.throughputWeight<=1/(1-fraction)+1e-5);if(!r.trace)++skipped;
   // Integral under cosine sampling: E[direction] = (2/3)*normal.
   double values[]={r.throughputWeight,r.throughputWeight*(r.direction.x+1),r.throughputWeight*(r.direction.y+1),r.throughputWeight*(r.direction.z+1)};
   for(unsigned j=0;j<4;++j){sum[j]+=values[j];square[j]+=values[j]*values[j];}
  }
  double expected[]={1,1+2.*n.x/3,1+2.*n.y/3,1+2.*n.z/3};
  for(unsigned j=0;j<4;++j){double mean=sum[j]/count,var=square[j]/count-mean*mean;double se=std::sqrt(std::max(0.,var)/count);assert(std::abs(mean-expected[j])<7*se+1e-5);}
  std::printf("PASS n=(%.1f,%.1f,%.1f) fraction=%.3f constant mean=%.6f zero-support fraction=%.6f\n",n.x,n.y,n.z,fraction,sum[0]/count,double(skipped)/count);
 }
 for(auto n:normals)for(float u:{0.f,.125f,.5f,.99999994f})for(float fraction:{0.f,.125f,.5f}){
  auto r=GISampleWorldGuide(guide,n,fraction,u,0);assert(r.valid&&std::isfinite(r.pdf)&&r.pdf>0);}
 std::puts("PASS exact zero vertical coordinate across mixture boundaries and rotated receivers");
 GIPoint bad{0,0,2};assert(!GISampleWorldGuide(guide,bad,.5f,.2f,.2f).valid);
 std::puts("PASS 7.5M draws: rotated/flipped receiver normals, world-fixed proposal, cosine expectation, miss-equivalent constant field, below-hemisphere zero terms, PDF support bound");
 std::puts("Scope: estimator math only. No live bindings, learned illumination, material identity, visibility reuse, RR output or FPS claim.");
}
