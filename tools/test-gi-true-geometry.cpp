#include "../src/gi_reuse/producer_shading.h"
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace gi_reuse;
static_assert(offsetof(GIProducerHit, nativeValue)==0);
static_assert(offsetof(GIProducerHit, geometricNormal)==32);
static_assert(offsetof(GIProducerHit, shadingNormal)==64);
static_assert(offsetof(GIProducerHit, texcoordV)==80);
void Require(bool v){if(!v)throw std::runtime_error("true geometry failed");}
void Near(float a,float b){Require(std::abs(a-b)<1e-6f);}
int main()try{
 GIPoint o{.25f,.25f,0},d{0,0,1},a{0,0,1},b{1,0,1},c{0,1,1};
 auto hit=GIRecoverTrueHit(o,d,1,a,b,c);Require(hit.valid==1);Near(hit.position.z,1);Near(hit.geometricNormal.z,1);
 // The old stored hit is at .99, but true reconstruction must stay at 1.
 Require(hit.position.z!=.99f);
 hit=GIRecoverTrueHit(o,d,1,a,c,b);Require(hit.valid==1);Near(hit.geometricNormal.z,-1);
 // Non-uniformly transformed triangle: cross world edges, not the untransformed normal.
 a={0,0,0};b={2,0,0};c={0,3,4};o={.5f,.75f,0};
 hit=GIRecoverTrueHit(o,d,1,a,b,c);Require(hit.valid==1);Near(hit.geometricNormal.y,-.8f);Near(hit.geometricNormal.z,.6f);
 // Translation and uniform scale do not affect normal direction.
 for(float scale : {1e-15f,1.f,1e15f}){
  a={0,0,scale};b={scale,0,scale};c={0,scale,scale};
  hit=GIRecoverTrueHit({0,0,0},d,scale,a,b,c);Require(hit.valid==1);Near(hit.geometricNormal.z,1);
 }
 Require(GIRecoverTrueHit(o,d,1,a,a,c).valid==0);
 Require(GIRecoverTrueHit(o,{0,0,2},1,a,b,c).valid==0);
 Require(GIRecoverTrueHit(o,d,-1,a,b,c).valid==0);
 Require(GIRecoverTrueHit(o,d,std::numeric_limits<float>::quiet_NaN(),a,b,c).valid==0);
 std::cout<<"PASS true geometry: unbiased point, winding, transformed edges, scale extremes, degenerate/invalid rejection. Caller triangle identity remains unverified.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
