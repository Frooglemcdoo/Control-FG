#include "../src/gi_reuse/frozen_world_guide.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <random>
using namespace gi_reuse;
int main(){
 float energy[64]{};energy[63]=1e30f;energy[2]=3e28f;
 auto guide=GIFreezeWorldGuide(energy,512,100,7);assert(GIFrozenValid(guide,100,7));
 assert(!GIFrozenValid(guide,101,7)&&!GIFrozenValid(guide,100,8));
 auto insufficient=GIFreezeWorldGuide(energy,255,100,7);assert(!insufficient.valid);
 energy[0]=std::numeric_limits<float>::infinity();auto invalid=GIFreezeWorldGuide(energy,512,100,7);assert(!invalid.valid);
 GIPoint normals[]={{0,0,1},{0,0,-1},{1,0,0},{.6f,0,.8f}};
 std::mt19937 rng(425);std::uniform_real_distribution<float> random(0,1);
 for(auto n:normals)for(float fraction:{0.f,.125f,.5f}){
  double sum[4]{},sq[4]{};unsigned zero=0;constexpr unsigned count=250000;
  for(unsigned i=0;i<count;++i){
   float u=random(rng),v=random(rng);auto r=GISampleFrozenWorld(guide,n,100,7,fraction,u,v);
   assert(r.valid&&r.frame==100&&r.epoch==7&&std::abs(GIDot(r.direction,r.direction)-1)<1e-5);
   assert(r.weight>=0&&r.weight<=1/(1-fraction)+1e-5);
   if(!r.trace){++zero;assert(r.weight==0);}
   double values[]={r.weight,r.weight*(1+r.direction.x),r.weight*(1+r.direction.y),r.weight*(1+r.direction.z)};
   for(unsigned j=0;j<4;++j){sum[j]+=values[j];sq[j]+=values[j]*values[j];}
   // Stale frame/epoch, insufficient training and invalid input training must
   // reproduce the ORIGINAL native random sample, not a redrawn proposal.
   if(i<100){
    auto native=GISampleFrozenWorld(guide,n,100,7,0,u,v);
    for(auto bad:{insufficient,invalid}){
     auto rbad=GISampleFrozenWorld(bad,n,100,7,fraction,u,v);
     assert(rbad.valid&&rbad.weight==native.weight);
     assert(rbad.direction.x==native.direction.x&&rbad.direction.y==native.direction.y&&rbad.direction.z==native.direction.z);
    }
    auto stale=GISampleFrozenWorld(guide,n,101,7,fraction,u,v);
    assert(stale.valid&&stale.weight==native.weight&&stale.direction.x==native.direction.x&&stale.direction.y==native.direction.y&&stale.direction.z==native.direction.z);
   }
  }
  double expected[]={1,1+2.*n.x/3,1+2.*n.y/3,1+2.*n.z/3};
  for(unsigned j=0;j<4;++j){double m=sum[j]/count,var=sq[j]/count-m*m;double se=std::sqrt(std::max(0.,var)/count);assert(std::abs(m-expected[j])<7*se+1e-5);}
  std::printf("PASS normal %.1f %.1f %.1f mixture %.3f constant %.6f zero %u\n",n.x,n.y,n.z,fraction,sum[0]/count,zero);
 }
 // A guide concentrated below the receiver still has full cosine support.
 // Below-hemisphere proposals are counted as zeros, never retried.
 float boundary[]={0.f,std::nextafter(.125f,0.f),.125f,std::nextafter(1.f,0.f)};
 for(float u:boundary)for(float v:boundary){auto r=GISampleFrozenWorld(guide,normals[0],100,7,.125f,u,v);assert(r.valid&&std::isfinite(r.weight));}
 auto zero=GISampleFrozenWorld(guide,normals[0],100,7,0,.3f,0);assert(zero.valid&&zero.frame==100&&zero.epoch==7);
 assert(!GISampleFrozenWorld(guide,normals[0],100,7,.6f,.5f,.5f).valid);
 std::puts("PASS 3M draws: stored-CDF density, analytic energy, flipped normals, stale/invalid pre-sample fallback and boundary samples. No runtime or FPS claim.");
}
