#include "../src/gi_reuse/reuse_candidate.h"
#include <iostream>
#include <stdexcept>
using namespace gi_reuse;
void Require(bool b){if(!b)throw std::runtime_error("Reuse candidate failed");}
void Near(float a,float b){Require(std::abs(a-b)<2e-5f);}
int main()try{
 GIConnection c{};c.source={0,0,0};c.target={0,0,-.1f};c.secondary={0,0,1};
 c.secondaryGeometricNormal={0,0,-1};c.material=7;c.frameTag=5;c.connectionId=17;c.verified=7;
 GIInitialInput v{};v.secondaryRadiance={2,3,4};v.primaryBrdfCos={.318309886f,.318309886f,.318309886f};
 v.material=7;v.frameTag=5;v.verified=7;v.configuredCount=6;v.depthScale=.75f;
 GIConnectionVisibility vis{5,17,8,1,1,1,0,0};
 auto shade=[&](GIConnection cc,GIInitialInput vv,GIConnectionVisibility visibility,uint producer=1,uint radiance=1){return GIShadeReuseCandidate(cc,vv,1,1,1,1,producer,radiance,8,visibility,5);};
 auto r=shade(c,v,vis);Require(r.valid);
 // q_receiver/q_source=.25/.5; geometric J=1/(1.1^2).
 Near(r.surfaceContribution.x,2*.5f/(1.1f*1.1f)*.75f/6);
 c.target=c.source;r=shade(c,v,vis);Require(r.valid);Near(r.surfaceContribution.x,.25f);
 for(unsigned i=0;i<11;++i){auto bad=vis;auto input=v;auto geometry=c;
  if(i==0)bad.visible=0; // opaque/alpha-tested blocker
  if(i==1)bad.completed=0;
  if(i==2)bad.nativeAlphaPolicy=0;
  if(i==3)bad.frameTag=4;
  if(i==4)bad.sceneRevision=7;
  if(i==5)bad.connectionId=18;
  if(i==6)bad.reserved0=1;
  if(i==7)input.material=8;
  if(i==8)geometry.verified=3;
  if(i==9)input.verified=3;
  if(i==10)geometry.target={0,0,-1};
  Require(!shade(geometry,input,bad).valid);
 }
 Require(!shade(c,v,vis,0).valid);Require(!shade(c,v,vis,1,0).valid);
 std::cout<<"PASS receiver candidate: source identity, finite reach/Jacobian, fresh visibility contract, stale/blocked/unverified rejection. Synthetic contract tests; no native trace executed.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
