#include "../src/shaders/pt_light_reservoir.hlsli"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace control_pt;
static uint seed=91357;
static float Random(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(seed>>8)*(1.0f/16777216.0f);}
int main(){
    auto empty=PTEmptyLightReservoir();assert(PTLightDenominator(empty)==0);
    auto zero=PTAddLightCandidate(empty,0,.25f,.5f,0);assert(zero.count==1&&PTLightDenominator(zero)==0);
    assert(!PTAddLightCandidate(empty,1,0,.5f,0).valid);
    assert(!PTAddLightCandidate(empty,std::numeric_limits<float>::quiet_NaN(),1,.5f,0).valid);
    auto deterministic=empty;
    deterministic=PTAddLightCandidate(deterministic,2,.25f,0,0);
    deterministic=PTAddLightCandidate(deterministic,0,.25f,0,1);
    deterministic=PTAddLightCandidate(deterministic,6,.25f,0,2);
    deterministic=PTAddLightCandidate(deterministic,0,.25f,0,3);
    assert(deterministic.selected==2&&std::abs(PTLightDenominator(deterministic)-.75f)<1e-6f);
    double mean=0,square=0;const unsigned runs=500000;
    for(unsigned trial=0;trial<runs;++trial){auto q=empty;float selected=0;
        for(uint c=0;c<4;++c){uint light=uint(Random()*38);float target=light==0?10.f:0.f;
            q=PTAddLightCandidate(q,target,1.f/38,Random(),c);if(q.target>0&&q.selected==c)selected=target;}
        float d=PTLightDenominator(q);double value=d>0?selected/d:0;mean+=value;square+=value*value;
    }
    mean/=runs;double variance=square/runs-mean*mean;
    assert(std::abs(mean-10)<.15);assert(variance<3700*.30);
    // Visibility is evaluated AFTER selection; an occluded bright light must
    // not bias surviving contributions, and cannot simply be resampled away.
    double visibleMean=0;
    for(unsigned trial=0;trial<runs;++trial){auto q=empty;float selected=0;
        for(uint c=0;c<4;++c){uint light=uint(Random()*3);float target=float(1u<<light);
            q=PTAddLightCandidate(q,target,1.f/3,Random(),c);if(q.selected==c)selected=light==0?0:target;}
        visibleMean+=selected/PTLightDenominator(q);
    }
    visibleMean/=runs;assert(std::abs(visibleMean-6)<.04);
    std::printf("PASS: shared RIS zero/invalid cases, sparse 38-light mean=%.6f variance=%.6f (uniform=3700), occluded mean=%.6f\n",mean,variance,visibleMean);
}
