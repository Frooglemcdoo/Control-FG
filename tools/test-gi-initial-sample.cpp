#include "../src/gi_reuse/initial_sample.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace gi_reuse;
void Require(bool b){if(!b)throw std::runtime_error("GI initial sample failed");}
void Near(float a,float b){Require(std::abs(a-b)<1e-6f);}
GIInitialInput Base(){GIInitialInput v{};v.secondaryPosition={1,2,3};v.sourcePdf=.25f;v.secondaryRadiance={2,3,4};v.primaryBrdfCos={.125f,.25f,.5f};v.material=7;v.frameTag=4;v.verified=7;v.configuredCount=6;v.depthScale=.75f;return v;}
int main()try{
 auto v=Base();auto r=GIMakeInitialSample(v,4);Require(r.valid&&r.isSurface);Near(r.surfaceContribution.x,.125f);Near(r.surfaceContribution.y,.375f);Near(r.surfaceContribution.z,1);Near(r.inverseSourcePdf,4);
 // Changing proposal density and BRDF*cos together preserves throughput.
 auto b=v;b.sourcePdf=.5f;b.primaryBrdfCos={.25f,.5f,1};auto other=GIMakeInitialSample(b,4);Near(r.surfaceContribution.z,other.surfaceContribution.z);
 // Black hit is a valid zero-weight candidate; do not erase its sample count.
 b=v;b.secondaryRadiance={0,0,0};other=GIMakeInitialSample(b,4);Require(other.valid&&other.isSurface);Near(other.targetDensity,0);
 // Miss has no position, PDF or surface radiance requirements.
 b=v;b.material=65535;b.verified=0;b.sourcePdf=0;b.secondaryPosition.x=std::numeric_limits<float>::quiet_NaN();b.fallbackContribution={1,2,3};other=GIMakeInitialSample(b,4);Require(other.valid&&!other.isSurface);Near(other.fallbackContribution.z,.375f);
 for(unsigned test=0;test<10;++test){b=v;
  if(test==0)b.verified=3;
  if(test==1)b.material=65534;
  if(test==2)b.sourcePdf=0;
  if(test==3)b.sourcePdf=std::numeric_limits<float>::quiet_NaN();
  if(test==4)b.frameTag=3;
  if(test==5)b.configuredCount=0;
  if(test==6)b.rayIndex=6;
  if(test==7)b.primaryBrdfCos.x=-1;
  if(test==8)b.secondaryRadiance.y=std::numeric_limits<float>::infinity();
  if(test==9)b.reserved=1;
  Require(!GIMakeInitialSample(b,4).valid);
 }
 std::cout<<"PASS initial GI: proposal cancellation, finite inputs, hit/miss split, black samples, native count and depth normalization.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
