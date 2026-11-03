#include "../src/gi_reuse/initial_reservoir.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace gi_reuse;
static void Check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
static GIInitialInput Sample(uint ray,uint count=4){
    GIInitialInput v{};v.secondaryPosition={0,0,1};v.sourcePdf=.25f;
    v.secondaryRadiance={2,0,0};v.primaryBrdfCos={.5f,.5f,.5f};v.rayIndex=ray;
    v.frameTag=19;v.verified=7;v.configuredCount=count;v.depthScale=2;return v;
}
int main()try{
    const GIPoint normal{0,0,-1};double mean[3]{};
    constexpr unsigned trials=65536;
    for(unsigned i=0;i<trials;++i){
        auto r=GIBeginInitialReservoir(19,4,2);auto a=Sample(0),b=Sample(1),black=Sample(2),miss=Sample(3);
        b.secondaryRadiance={0,3,0};b.primaryBrdfCos={.25f,.25f,.25f};b.sourcePdf=.75f;
        black.secondaryRadiance={0,0,0};miss.material=65535;miss.fallbackContribution={.1f,.2f,.3f};
        Check(GIStreamInitialReservoir(r,a,normal,1,.5f),"First sample rejected");
        Check(GIStreamInitialReservoir(r,b,normal,1,(i+.5f)/trials),"Second sample rejected");
        Check(GIStreamInitialReservoir(r,black,normal,1,.5f),"Black sample rejected");
        Check(GIStreamInitialReservoir(r,miss,{},0,.5f),"Miss sample rejected");
        const auto result=GIResolveInitialReservoir(r);Check(result.valid&&r.processed==4,"Complete batch rejected");
        mean[0]+=result.surfaceContribution.x+result.fallbackContribution.x;
        mean[1]+=result.surfaceContribution.y+result.fallbackContribution.y;
        mean[2]+=result.surfaceContribution.z+result.fallbackContribution.z;
        Check(result.inverseSourcePdf==0,"RIS weight mislabeled as source PDF");
    }
    Check(std::abs(mean[0]/trials-2.05)<.0002&&std::abs(mean[1]/trials-.6)<.0002&&std::abs(mean[2]/trials-.15)<.0002,"Initial RIS expectation or fallback denominator changed");
    auto r=GIBeginInitialReservoir(19,64,2);
    for(uint i=0;i<64;++i){auto v=Sample(i,64);Check(GIStreamInitialReservoir(r,v,normal,1,.5f),"64-ray boundary rejected");}
    auto result=GIResolveInitialReservoir(r);Check(result.valid&&std::abs(result.surfaceContribution.x-8)<1e-5f,"64-ray source energy changed");
    Check(r.seenLow==~0u&&r.seenHigh==~0u,"64-ray coverage incomplete");
    r=GIBeginInitialReservoir(19,1,2);auto black=Sample(0,1);black.secondaryRadiance={0,0,0};
    Check(GIStreamInitialReservoir(r,black,normal,1,.5f)&&GIResolveInitialReservoir(r).valid&&!r.hasSelection,"All-black batch invalidated");
    r=GIBeginInitialReservoir(19,1,2);auto miss=Sample(0,1);miss.material=65535;miss.fallbackContribution={1,2,3};
    Check(GIStreamInitialReservoir(r,miss,{},0,.5f),"All-miss batch rejected");result=GIResolveInitialReservoir(r);Check(result.valid&&!result.isSurface&&result.fallbackContribution.z==6,"All-miss energy changed");
    r=GIBeginInitialReservoir(19,4,2);Check(GIStreamInitialReservoir(r,Sample(0),normal,1,.5f),"Setup failed");
    Check(!GIResolveInitialReservoir(r).valid,"Incomplete batch published");
    Check(!GIStreamInitialReservoir(r,Sample(0),normal,1,.5f)&&!GIResolveInitialReservoir(r).valid,"Duplicate admitted");
    for(unsigned test=0;test<7;++test){
        r=GIBeginInitialReservoir(19,4,2);auto v=Sample(0);auto n=normal;uint verified=1;float random=.5f;
        if(test==0)v.frameTag=18;
        if(test==1)v.configuredCount=3;
        if(test==2)v.depthScale=1;
        if(test==3)random=1;
        if(test==4)random=std::numeric_limits<float>::quiet_NaN();
        if(test==5)n={};
        if(test==6)verified=0;
        Check(!GIStreamInitialReservoir(r,v,n,verified,random)&&!GIResolveInitialReservoir(r).valid,"Invalid input published");
    }
    Check(!GIBeginInitialReservoir(19,65,1).valid&&!GIBeginInitialReservoir(19,0,1).valid,"Invalid ray count admitted");
    std::cout<<"PASS initial RIS: unequal PDFs/colors, full-ray normalization, miss fallback, black hits, 64-ray coverage and malformed/incomplete rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
