#include "../src/pt_scaling_metrics.h"
#include <cassert>
#include <cstdio>
int main(){
    PTScalingMetrics::Window w;const std::array<double,6> c{8,4,1,1,1,1};
    for(unsigned i=0;i<119;++i)assert(!w.Add(3840,2160,4,c));
    assert(w.Add(3840,2160,4,c));assert(w.samples==120&&w.gpu==480&&w.cpu[0]==960);
    w.ClearSamples();assert(w.samples==0&&w.width==3840);
    assert(!w.Add(3840,2160,4,c));
    assert(!w.Add(2560,1440,2,c));assert(w.samples==1&&w.gpu==2&&w.cpu[0]==8&&w.sequence==2);
    assert(!w.Add(0,1440,2,c));assert(w.samples==1);
    for(unsigned i=0;i<119;++i){const bool ready=w.Add(2560,1440,2,c);assert(ready==(i==118));}
    assert(w.gpu==240&&w.cpu[1]==480);
    puts("PASS: resolution windows never mix DLAA/Quality costs and emit only complete 120-sample groups.");
}
