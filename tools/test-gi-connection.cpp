#include "../src/gi_reuse/connection.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace gi_reuse;
void Require(bool b){if(!b)throw std::runtime_error("connection contract failed");}
void Near(float a,float b){Require(std::abs(a-b)<1e-6f);}
GIConnection Base(){GIConnection c{};c.secondary={0,0,1};c.secondaryGeometricNormal={0,0,-1};c.sourceMaxDistance=c.targetMaxDistance=4;c.sourceSolidAnglePdf=.25f;c.material=7;c.frameTag=5;c.connectionId=9;c.verified=3;return c;}
int main()try{
 auto c=Base();auto r=GIEvaluateConnection(c,5);Require(r.valid==1);Near(r.jacobian,1);Near(r.mappedSolidAnglePdf,.25f);
 c.target={0,0,-1};r=GIEvaluateConnection(c,5);Require(r.valid==1);Near(r.jacobian,.25f);Near(r.mappedSolidAnglePdf,1);
 c=Base();c.target={1,0,0};r=GIEvaluateConnection(c,5);Require(r.valid==1);Near(r.jacobian,1/(2*std::sqrt(2.f)));
 // Reversing source and destination inverts the geometric Jacobian.
 auto reverse=c;reverse.source=c.target;reverse.target=c.source;
 Near(r.jacobian*GIEvaluateConnection(reverse,5).jacobian,1);
 for(unsigned test=0;test<10;++test){c=Base();
  if(test==0)c.verified=0;
  if(test==1)c.material=65535;
  if(test==2)c.material=65534;
  if(test==3)c.frameTag=4;
  if(test==4)c.targetMaxDistance=.5f;
  if(test==5)c.sourceMaxDistance=.5f;
  if(test==6)c.secondaryGeometricNormal={0,0,1};
  if(test==7)c.sourceSolidAnglePdf=0;
  if(test==8)c.secondary.x=std::numeric_limits<float>::quiet_NaN();
  if(test==9)c.target=c.secondary;
  Require(GIEvaluateConnection(c,5).valid==0);
 }
 c=Base();c.secondaryGeometricNormal={0,0,1};c.twoSided=1;Require(GIEvaluateConnection(c,5).valid==1);
 std::cout<<"PASS GI connection: identity, distance/angle PDF transforms, reciprocal Jacobian, support limits, sentinels, stale/unverified/invalid geometry, two-sided policy. No visibility or image-quality claim.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
