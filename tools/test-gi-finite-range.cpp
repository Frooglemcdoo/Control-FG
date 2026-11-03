#include "../src/gi_reuse/finite_range_proposal.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace gi_reuse;
void Require(bool b){if(!b)throw std::runtime_error("Finite-range proposal failed");}
void Near(float a,float b){Require(std::abs(a-b)<2e-5f);}
int main()try{
 GIInitialInput in{};in.secondaryPosition={1,2,3};in.secondaryRadiance={2,3,4};
 in.material=7;in.frameTag=1;in.configuredCount=6;in.depthScale=.75f;in.verified=7;
 const float distances[]={0,.4f,.8f,.9f,1,1.1f,1.19f};
 for(float d:distances){
  auto p=GIMakeRangeProposal(.6f,d,1,1);Require(p.valid);
  in.sourcePdf=p.directionalPdf;in.primaryBrdfCos={p.directionalPdf,p.directionalPdf*2,p.directionalPdf*3};
  auto base=GIMakeInitialSample(in,1);
  auto adjusted=GIMakeInitialSample(GIApplyRangeProposal(in,p,p),1);
  Require(base.valid&&adjusted.valid);
  Near(base.surfaceContribution.x,adjusted.surfaceContribution.x);
  Near(base.surfaceContribution.y,adjusted.surfaceContribution.y);
  Near(base.surfaceContribution.z,adjusted.surfaceContribution.z);
  // Independently integrate the producer range variate, including misses.
  unsigned hit=0;constexpr unsigned n=100000;
  for(unsigned i=0;i<n;++i)if(.8f+.4f*((float(i)+.5f)/float(n))>=d)++hit;
  Near(p.reachProbability,float(hit)/float(n));
  // Constant surface and fallback: expected energy is q*L+(1-q)*F.
  Near(p.reachProbability*3+(1-p.reachProbability)*2,float(hit)/float(n)+2);
 }
 auto source=GIMakeRangeProposal(.6f,1,1,1),target=GIMakeRangeProposal(.6f,1.1f,1,1);
 Near(source.reachProbability,.5f);Near(target.reachProbability,.25f);
 in.sourcePdf=source.directionalPdf;in.primaryBrdfCos={source.directionalPdf,0,0};
 auto r=GIMakeInitialSample(GIApplyRangeProposal(in,source,target),1);
 Near(r.surfaceContribution.x,.125f); // 2 * (.25/.5) * .75 / 6
 for(auto p:{GIMakeRangeProposal(.6f,1.21f,1,1),GIMakeRangeProposal(0,1,1,1),
   GIMakeRangeProposal(1.01f,1,1,1),GIMakeRangeProposal(.6f,-1,1,1),
   GIMakeRangeProposal(.6f,1,0,1),GIMakeRangeProposal(.6f,1,1,0),
   GIMakeRangeProposal(.6f,std::numeric_limits<float>::infinity(),1,1)})Require(!p.valid);
 in.verified=0;Require(!GIMakeInitialSample(GIApplyRangeProposal(in,source,target),1).valid);
 std::cout<<"PASS finite-range model: source cancellation, target/source support, integrated reach/miss energy, invalid inputs. Not native-distribution verification.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
