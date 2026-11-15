#include "../src/performance_compare_state.h"
#include "../src/performance_compare_stats.h"
#include <cassert>
#ifdef CONTROL_GI_RAY_COST_CAPTURE
#define CONTROL_NRD_GI
#include "../src/rt_surface_source_capture.h"
#endif
int main(){
 using namespace control_compare;Reset();Present(100,1);assert(count==0);
 phase=2;enabled=true;Present(200,2);enabled=false;Present(300,3);
 assert(count==1&&frames[0].ready&&frames[0].phase==2&&frames[0].ticks==200);
 CPU(2,1,123);assert(cpuCalls[2][1]==1&&cpuTicks[2][1]==123);
 Reset();assert(!frames[0].ready&&cpuCalls[2][1]==0);
#ifdef CONTROL_GI_RAY_COST_CAPTURE
 // Reproduce the shipped default: RELAX OFF is manualGI=4 and must start.
 assert(rt_surface_source::manualGI.load()==4);
 assert(rt_surface_source::ReadyForRayCostCapture());
 for(unsigned mode=0;mode<5;++mode){rt_surface_source::manualGI=mode;assert(rt_surface_source::ReadyForRayCostCapture()==(mode==4));}
 rt_surface_source::request=8;assert(!rt_surface_source::ReadyForRayCostCapture());rt_surface_source::request=0;
 rt_surface_source::manualContact=2;assert(!rt_surface_source::ReadyForRayCostCapture());rt_surface_source::manualContact=0;
 control_rr_gi::stageMode=control_rr_gi::StageMode::Both;assert(!rt_surface_source::ReadyForRayCostCapture());
 control_rr_gi::stageMode=control_rr_gi::StageMode::RawBypass;assert(rt_surface_source::ReadyForRayCostCapture());
 assert(phaseCount==7&&conditions[0].rays==0&&conditions[3].rays==0);
 for(unsigned i=0;i<phaseCount;++i){assert(conditions[i].rr&&!conditions[i].relax);if(i!=0&&i!=3)assert(conditions[i].rays==10);}
 assert(!conditions[5].profile&&conditions[6].profile);
#else
 assert(phaseCount==14&&!conditions[0].owned&&!conditions[8].owned);
 assert(conditions[6].relax&&conditions[6].character&&conditions[6].volume&&conditions[6].owned);
 for(const auto& p:conditions)assert(p.rays==0);
 assert(!conditions[9].rr&&!conditions[10].rr&&conditions[11].rr);
 assert(!conditions[12].profile&&conditions[12].relax&&conditions[13].profile);
#endif
 assert(RepeatStable(10,10.4)&&!RepeatStable(10,11)&&!RepeatStable(0,10)&&!RepeatStable(10,NAN));
 auto empty=Summarize({0,-1,NAN});assert(empty.count==0);
 auto s=Summarize({10,10,10,20});assert(s.mean==12.5&&s.p50==10&&s.p99==20&&s.low1==50);
}
