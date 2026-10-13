#include "../src/pt_hotpath_timing.h"
#include <cstdio>
#include <initializer_list>
int main(){
    using namespace PTHotpathTiming;
    for(unsigned i=0;i<128;++i){
        Sample disabled(Kind::glass_capture,false);disabled.Next(Kind::primary_metrics);
        Sample profile(Kind::build_wait);profile.Next(Kind::build_inputs);profile.Next(Kind::build_record);
    }
    for(auto kind:{Kind::build_wait,Kind::build_inputs,Kind::build_record}){
        const auto& counter=counters[static_cast<unsigned>(kind)];
        if(counter.samples.load()!=2||counter.maximum.load()>counter.ticks.load())return 1;
    }
    if(counters[static_cast<unsigned>(Kind::glass_capture)].samples.load()||
       counters[static_cast<unsigned>(Kind::primary_metrics)].samples.load())return 2;
    // Early return/destruction must finish only the last entered stage.
    {Sample profile(Kind::reset_lookup);profile.Next(Kind::reset_wait);}
    if(counters[static_cast<unsigned>(Kind::reset_lookup)].samples.load()!=1||
       counters[static_cast<unsigned>(Kind::reset_wait)].samples.load()!=1||
       counters[static_cast<unsigned>(Kind::reset_native)].samples.load()!=0)return 3;
    std::puts("PASS: same sampled operation across stages, disabled path, early-exit accounting");
}
