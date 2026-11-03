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
    result.surfaceContribution.x=std::numeric_limits<float>::max();result.fallbackContribution.x=std::numeric_limits<float>::max();
    assert(!GIBeginOutput(result,7,2,2).valid);
    std::puts("PASS native GI output contract: complete 1/2/63/64-ray coverage, all-ray distance mean, no double colour normalization, stale/duplicate/nonfinite rejection");
}
