#include "../src/performance_compare_state.h"
#include "../src/performance_compare_stats.h"
#include <cassert>
int main(){
 using namespace control_compare;Reset();Present(100,1);assert(count==0);
 phase=2;enabled=true;Present(200,2);enabled=false;Present(300,3);
 assert(count==1&&frames[0].ready&&frames[0].phase==2&&frames[0].ticks==200);
 CPU(2,1,123);assert(cpuCalls[2][1]==1&&cpuTicks[2][1]==123);
 Reset();assert(!frames[0].ready&&cpuCalls[2][1]==0);
 assert(phaseCount==9&&!conditions[0].owned&&!conditions[8].owned);
 assert(conditions[6].relax&&conditions[6].character&&conditions[6].volume&&conditions[6].owned);
 for(const auto& p:conditions)if(p.owned)assert(p.rays==0);
 auto empty=Summarize({0,-1,NAN});assert(empty.count==0);
 auto s=Summarize({10,10,10,20});assert(s.mean==12.5&&s.p50==10&&s.p99==20&&s.low1==50);
}
