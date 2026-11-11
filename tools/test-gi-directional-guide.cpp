#include "../src/gi_reuse/directional_guide.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include <random>
using namespace gi_reuse;
struct Moments {double mean,variance,peak;};
Moments moments(GIDirectionalGuide g,const std::array<double,64>& signal,bool corrected=true){
 double sum=0,mean=0,second=0,peak=0;for(float p:g.mass)sum+=p;
 for(unsigned i=0;i<64;++i){double p=g.mass[i]/sum;double v=signal[i]/(corrected?64*p:1);mean+=p*v;second+=p*v*v;peak=std::max(peak,v);}
 return {mean,std::max(0.,second-mean*mean),peak};
}
double currentTwoRayStrataVariance(const std::array<double,64>& signal){
 // Current Owned count=2 has one ray in each x half and opposite y halves.
 // Average both diagonal permutations; conditional draws are independent.
 double means[4]{},seconds[4]{};
 for(unsigned i=0;i<64;++i){unsigned q=(i%8>=4?1:0)+(i/8>=4?2:0);means[q]+=signal[i]/16;seconds[q]+=signal[i]*signal[i]/16;}
 double e=0,e2=0;
 for(unsigned diagonal=0;diagonal<2;++diagonal){unsigned a=diagonal?2:0,b=diagonal?1:3;
  double m=(means[a]+means[b])/2,v=(seconds[a]-means[a]*means[a]+seconds[b]-means[b]*means[b])/4;
  e+=m/2;e2+=(v+m*m)/2;}
 return e2-e*e;
}
int main(){
 float energy[64]{};auto uniform=GIBuildDirectionalGuide(energy,.5f);
 for(float p:uniform.mass)assert(p==1.f/64);
 energy[7]=100;for(unsigned i=0;i<64;++i)if(i!=7)energy[i]=.05f;
 auto guided=GIBuildDirectionalGuide(energy,.5f);
 double total=0;for(float p:guided.mass)total+=p;
 for(float p:guided.mass)assert(64*p/total>=.499999);
 std::array<double,64> signal{};for(unsigned i=0;i<64;++i)signal[i]=energy[i];
 std::printf("CURRENT two-ray diagonal strata analytic variance %.9f (single-frame toy only)\n",currentTwoRayStrataVariance(signal));
 auto base=moments(uniform,signal),good=moments(guided,signal),bad=moments(guided,signal,false);
 assert(std::abs(good.mean-base.mean)<1e-12);assert(good.variance<base.variance*.03);
 assert(std::abs(bad.mean-base.mean)>10); // Catches an R416-style missing weight.
 std::printf("MATCHED analytic mean %.9f, two-ray variance baseline %.9f guided %.9f ratio %.6f, single contribution peak %.6f -> %.6f\n",base.mean,base.variance/2,good.variance/2,good.variance/base.variance,base.peak,good.peak);
 signal[7]=.05;signal[51]=100;
 auto stale=moments(guided,signal),movedBase=moments(uniform,signal);
 assert(std::abs(stale.mean-movedBase.mean)<1e-12);
 assert(stale.variance>movedBase.variance); // Stale guidance must NOT be advertised as always better.
 std::printf("MOVED analytic mean %.9f, two-ray variance baseline %.9f stale %.9f ratio %.6f\n",stale.mean,movedBase.variance/2,stale.variance/2,stale.variance/movedBase.variance);
 // Colored surfaces, black occlusion, and a large nonzero miss/volume baseline.
 for(unsigned channel=0;channel<3;++channel){
  for(unsigned i=0;i<64;++i)signal[i]=i%3==0?0:i%3==1?(.3+channel*.2):(i+1)*(.7-channel*.15);
  auto a=moments(uniform,signal),b=moments(guided,signal);assert(std::abs(a.mean-b.mean)<1e-12);
 }
 // Removing the light remains correct using stale direction probabilities.
 signal.fill(.2);auto removed=moments(guided,signal);assert(std::abs(removed.mean-.2)<1e-12);
 std::printf("REMOVED constant fallback mean %.9f, variance %.9f (not zero with stale guide)\n",removed.mean,removed.variance);
 // Real sampler: unbiased bin frequencies and weighted RGB expectation.
 signal.fill(.05);signal[7]=100;
 std::mt19937 gen(423);std::uniform_real_distribution<float> rand(0,1);
 constexpr unsigned count=1000000;std::array<unsigned,64> bins{};double sum=0,sum2=0;
 for(unsigned i=0;i<count;++i){auto r=GISampleDirectionalGuide(guided,rand(gen),rand(gen));assert(r.valid&&r.u>=0&&r.u<1&&r.v>=0&&r.v<1);assert(unsigned(r.u*8)+8*unsigned(r.v*8)==r.cell);assert(r.inverseRelativePdf<=2.00001f);++bins[r.cell];double value=signal[r.cell]*r.inverseRelativePdf;sum+=value;sum2+=value*value;}
 double mean=sum/count,se=std::sqrt((sum2/count-mean*mean)/count);assert(std::abs(mean-good.mean)<6*se);
 for(unsigned i=0;i<64;++i){double p=guided.mass[i]/total;assert(std::abs(double(bins[i])/count-p)<6*std::sqrt(p*(1-p)/count)+1./count);}
 for(float invalid:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  energy[7]=invalid;auto g=GIBuildDirectionalGuide(energy,.5f);for(float p:g.mass)assert(p==1.f/64);
 }
 for(float fraction:{-1.f,1.f,std::numeric_limits<float>::quiet_NaN()}){auto g=GIBuildDirectionalGuide(energy,fraction);for(float p:g.mass)assert(p==1.f/64);}
 energy[7]=std::numeric_limits<float>::max();auto huge=GIBuildDirectionalGuide(energy,.5f);assert(GISampleDirectionalGuide(huge,.99999994f,.99999994f).valid);
 assert(!GISampleDirectionalGuide(guided,1,0).valid);assert(!GISampleDirectionalGuide(guided,0,-1).valid);
 std::puts("PASS actual C++ sampler: 1M draws, histogram, weighted energy, support floor, HDR normalization, invalid fallback; analytic stale/light-removal/miss/color cases");
 std::puts("LIMIT: discrete toy integrands and independent draws, not Control rays, production shader binding, guide learning, or in-game image/FPS validation.");
}
