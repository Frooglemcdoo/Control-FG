#include "../src/gi_reuse/output_resolve.h"
#include <cassert>
#include <limits>
#include <cstdio>
#include <initializer_list>
using namespace gi_reuse;
int main(){
    GIInitialResult result{};result.valid=1;result.frameTag=7;
    result.surfaceContribution={1,2,3};result.fallbackContribution={.1f,.2f,.3f};
    for(uint count:{1u,2u,63u,64u}){
        auto state=GIBeginOutput(result,7,count,2);assert(state.valid&&!GIOutputComplete(state));
        GIShadedProducerSample sample{};sample.hitDistance=4;sample.shading.material=65535;
        sample.shading.configuredCount=count;sample.shading.frameTag=7;sample.shading.depthScale=2;
        for(uint ray=0;ray<count;++ray){sample.shading.rayIndex=ray;assert(GIAddOutputDistance(state,sample));}
        assert(GIOutputComplete(state)&&std::abs(state.distance-4)<1e-5f&&std::abs(state.color.x-1.1f)<1e-5f);
        auto duplicate=state;assert(!GIAddOutputDistance(duplicate,sample)&&!GIOutputComplete(duplicate));
        for(uint bad=0;bad<5;++bad){auto input=sample;input.shading.rayIndex=0;auto rejected=GIBeginOutput(result,7,count,2);
            if(bad==0)input.hitDistance=std::numeric_limits<float>::quiet_NaN();
            if(bad==1)input.hitDistance=-1;
            if(bad==2)input.shading.frameTag=6;
            if(bad==3)input.shading.depthScale=1;
            if(bad==4)input.reserved1=1;
            assert(!GIAddOutputDistance(rejected,input)&&!GIOutputComplete(rejected));
        }
    }
    assert(!GIBeginOutput(result,6,2,2).valid&&!GIBeginOutput(result,7,0,2).valid&&!GIBeginOutput(result,7,65,2).valid);
    // Independent analytical batch: unequal PDFs, colored hits, miss and black.
    // The displayed estimate must not depend on the reservoir's selected color.
    for(float selected:{1.f,1000.f}){
        result.surfaceContribution={selected,0,0};
        auto state=GIBeginBatchOutput(result,7,4,2);
        for(uint i=0;i<4;++i){
            GIShadedProducerSample v{};v.hitDistance=float(i+1);
            auto& s=v.shading;s.frameTag=7;s.configuredCount=4;s.depthScale=2;s.rayIndex=i;
            s.material=0;s.verified=7;s.sourcePdf=1;s.primaryBrdfCos={1,1,1};
            if(i==0){s.secondaryRadiance={4,0,0};s.primaryBrdfCos={.5f,.5f,.5f};s.sourcePdf=.25f;}
            if(i==1){s.secondaryRadiance={0,6,0};s.primaryBrdfCos={.25f,.25f,.25f};s.sourcePdf=.5f;}
            if(i==2){s.material=65535;s.fallbackContribution={0,0,8};}
            assert(GIAddOutputDistance(state,v));
            if(i<3)assert(!GIOutputComplete(state));
        }
        assert(GIOutputComplete(state));
        assert(state.color.x==4&&state.color.y==1.5f&&state.color.z==4&&state.distance==2.5f);
    }
    result.surfaceContribution.x=std::numeric_limits<float>::max();result.fallbackContribution.x=std::numeric_limits<float>::max();
    assert(!GIBeginOutput(result,7,2,2).valid);
    std::puts("PASS native GI output contract: complete 1/2/63/64-ray coverage, all-ray distance mean, no double colour normalization, stale/duplicate/nonfinite rejection");
}
