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
    // Complete primary enumeration: a single contributing light must never
    // disappear, regardless of its position in a sparse native light list.
    for(uint total: {1u,2u,37u,64u})for(uint bright=0;bright<total;++bright){
        for(uint trial=0;trial<20;++trial){auto q=empty;float chosen=0;
            assert(PTLightCandidateCount(total,1)==total);
            for(uint c=0;c<PTLightCandidateCount(total,1);++c){
                uint selected=uint(PTLightCandidateChoose(total,1,c,Random())*total);
                assert(selected==c);float target=selected==bright?10.f:0.f;
                q=PTAddLightCandidate(q,target,1.f/float(total),Random(),c);
                if(q.target>0&&q.selected==c)chosen=target;
            }
            assert(std::abs(chosen/PTLightDenominator(q)-10.f)<1e-4f);
        }
    }
    assert(PTLightCandidateCount(0,1)==4&&PTLightCandidateCount(65,1)==4);
    assert(PTLightCandidateCount(37,0)==4);
    assert(PTLightCandidateChoose(65,1,2,.37f)==.37f);
    // Enumeration preserves colored, shadowed energy. Bright occluded light
    // remains eligible; never discard/reselect it after the shadow test.
    double rgb[3]={};const float emission[4][3]={{9,1,2},{1,7,3},{0,0,0},{3,2,8}};
    for(unsigned trial=0;trial<runs;++trial){auto q=empty;
        for(uint c=0;c<4;++c){float target=emission[c][0]*.2126f+emission[c][1]*.7152f+emission[c][2]*.0722f;
            q=PTAddLightCandidate(q,target,.25f,Random(),c);}
        float denominator=PTLightDenominator(q);
        if(q.selected!=0)for(uint c=0;c<3;++c)rgb[c]+=emission[q.selected][c]/denominator;
    }
    for(uint c=0;c<3;++c){rgb[c]/=runs;assert(std::abs(rgb[c]-(emission[1][c]+emission[3][c]))<.05);}
    puts("PASS: primary enumeration has zero sparse-light dropouts for 1..64 lights, preserves colored/occluded energy, bounds fallback cost");
    // Equal-brightness lights, one aligned with a narrow specular lobe.
    // Old incident*cos weights ignore that lobe. Use identical proposals and
    // reservoir random numbers for both estimators; compare means to exact sum.
    PTSurfaceMaterial glossy;glossy.kd=PTV(.15f,.08f,.03f);glossy.f0=PTV(.8f,.5f,.2f);
    glossy.roughness=.2f;glossy.lobes=3;
    const PTVec3 wo=PTV(0,0,1),incident=PTV(1,1,1);
    PTVec3 directions[16];float targets[16];double exact=0;
    for(unsigned i=0;i<16;++i){
        float x=i==0?0.f:.55f+.025f*float(i);
        directions[i]=PTV(x,0,std::sqrt(1-x*x));
        targets[i]=PTLightTarget(glossy,wo,directions[i],incident);exact+=targets[i];
    }
    assert(PTLightTarget(glossy,wo,PTV(0,0,-1),incident)==0);
    assert(PTLightTarget(glossy,wo,wo,PTV(0,0,0))==0);
    assert(PTLightTarget(glossy,wo,wo,PTV(-1,0,0))<0);
    double sums[2]={},squares[2]={};
    for(unsigned trial=0;trial<runs;++trial){
        PTLightReservoir q[2]={empty,empty};float chosen[2]={};
        for(uint c=0;c<4;++c){uint light=uint(Random()*16);float random=Random();
            for(unsigned mode=0;mode<2;++mode){
                float target=mode?targets[light]:directions[light].z;
                q[mode]=PTAddLightCandidate(q[mode],target,1.f/16,random,c);
                if(q[mode].target>0&&q[mode].selected==c)chosen[mode]=targets[light];
            }
        }
        for(unsigned mode=0;mode<2;++mode){float denominator=PTLightDenominator(q[mode]);
            double value=denominator>0?chosen[mode]/denominator:0;
            sums[mode]+=value;squares[mode]+=value*value;
        }
    }
    double variances[2];
    for(unsigned mode=0;mode<2;++mode){sums[mode]/=runs;variances[mode]=squares[mode]/runs-sums[mode]*sums[mode];
        assert(std::abs(sums[mode]-exact)<exact*.02);
    }
    assert(variances[1]<variances[0]*.4);
    std::printf("PASS: material target exact=%.6f old_mean=%.6f new_mean=%.6f old_variance=%.6f new_variance=%.6f\n",
                exact,sums[0],sums[1],variances[0],variances[1]);
    std::printf("PASS: shared RIS zero/invalid cases, sparse 38-light mean=%.6f variance=%.6f (uniform=3700), occluded mean=%.6f\n",mean,variance,visibleMean);
}
