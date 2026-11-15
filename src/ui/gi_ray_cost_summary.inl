// Included inside Summary(); R440 ray-cost capture only.
std::vector<double> intervals[phaseCount];
std::ostringstream cadence,raw,table,stages,out;
cadence<<"phase,present,interval_ms\n";
uint64_t lastTicks=0,lastPresent=0;unsigned lastPhase=~0u;
const unsigned n=(std::min)(control_compare::count.load(),capacity);
for(unsigned i=0;i<n;++i){auto& f=frames[i];if(!f.ready.load(std::memory_order_acquire))continue;
 if(f.phase==lastPhase&&f.present==lastPresent+1&&f.ticks>lastTicks&&f.phase<phaseCount){
  const double ms=1000.*(f.ticks-lastTicks)/qpcFrequency;
  intervals[f.phase].push_back(ms);cadence<<conditions[f.phase].name<<','<<f.present<<','<<ms<<'\n';
 }
 lastTicks=f.ticks;lastPresent=f.present;lastPhase=f.phase;
}
Statistics stats[phaseCount];for(unsigned p=0;p<phaseCount;++p)stats[p]=Summarize(intervals[p]);
if(outcome.rfind("COMPLETE",0)==0){
 bool stable=RepeatStable(stats[0].mean,stats[3].mean)&&RepeatStable(stats[1].mean,stats[2].mean)&&
     RepeatStable(stats[2].mean,stats[4].mean)&&RepeatStable(stats[4].mean,stats[6].mean);
 for(const auto& s:stats)stable=stable&&s.count>100;
 if(!stable)outcome="INCOMPLETE: repeat drift exceeds 5% or insufficient frame intervals";
}
out<<"R441 GI RAY COST: R434/R439 OUTPUT, MULTI-RAY GUIDE BYPASS\nOutcome: "<<outcome<<"\n\n"
   <<"Only GI sample request changes: Native versus 10. All lighting, guide, character, volume, reflection and GI-stage settings are retained.\n"
   <<"The R434 fixed guide applies only to the one-ray path; its toggle is held fixed. This measures the real selected paths, not pure TraceRay instruction cost.\n"
   <<"Seven phases, each 5-second warmup and 12-second measurement. GPU queries every 64 Presents; rays10_no_queries disables GPU queries and CPU span timers.\n"
   <<"Cadence and coverage collection remain active in the no-query phase. Do not interpret it as completely uninstrumented gameplay.\n"
   <<"Selected GPU spans are not total GPU frame time. GI ray timing includes traversal and hit-data writes; GI evaluation includes hit-data reads and lighting.\n"
   <<"This capture does not independently separate memory traffic from execution within either span. RR is timed separately.\n"
   <<"Keep the camera still. Dynamic lights/objects and clock drift can invalidate comparisons. Incomplete results must not be ranked as optimization evidence.\n\n";
table<<"phase,requested_rays,effective_sample_mask,intervals,mean_frame_ms,p50_ms,p95_ms,p99_ms,average_fps,coverage_ok,skipped_gpu_samples\n";
for(unsigned p=0;p<phaseCount;++p){const auto& s=stats[p];
 table<<conditions[p].name<<','<<conditions[p].rays<<','<<results[p].sampleMask<<','<<s.count<<','<<s.mean<<','<<s.p50<<','<<s.p95<<','<<s.p99<<','<<(s.mean?1000/s.mean:0)<<','<<results[p].observed<<','<<results[p].skipped<<'\n';
 out<<conditions[p].name<<": "<<s.mean<<" ms, "<<(s.mean?1000/s.mean:0)<<" FPS; p99="<<s.p99<<" ms; intervals="<<s.count<<"; coverage="<<results[p].observed<<"; effective_ray_mask="<<results[p].sampleMask<<'\n';
}
if(stats[0].mean&&stats[3].mean&&stats[1].mean&&stats[2].mean&&stats[4].mean&&stats[6].mean)
 out<<"\nObserved 10-ray minus native mean frame time: "<<(stats[1].mean+stats[2].mean+stats[4].mean+stats[6].mean)/4-(stats[0].mean+stats[3].mean)/2<<" ms (valid only when repeats/coverage pass).\n";
if(stats[4].mean&&stats[5].mean&&stats[6].mean)
 out<<"10-ray profiled bracket minus no-query frame time: "<<(stats[4].mean+stats[6].mean)/2-stats[5].mean<<" ms.\n";
std::map<std::tuple<unsigned,unsigned,uint64_t>,double> grouped;
raw<<"phase,present,stage,gpu_ms\n";
for(const auto& s:gpu){raw<<conditions[s.phase].name<<','<<s.present<<','<<performance::Name(performance::Stage(s.stage))<<','<<s.ms<<'\n';grouped[{s.phase,s.stage,s.present}]+=s.ms;}
stages<<"phase,stage,sampled_presents,mean_gpu_ms_per_sampled_present,p95_gpu_ms_per_sampled_present\n";
for(unsigned p=0;p<phaseCount;++p){
 for(unsigned stage=1;stage<performance::kStageCount;++stage){std::vector<double> v;
  for(const auto& item:grouped)if(std::get<0>(item.first)==p&&std::get<1>(item.first)==stage)v.push_back(item.second);
  const auto s=Summarize(v);if(s.count)stages<<conditions[p].name<<','<<performance::Name(performance::Stage(stage))<<','<<s.count<<','<<s.mean<<','<<s.p95<<'\n';
 }
 out<<"\n"<<conditions[p].name<<" settings/evidence:\n"<<results[p].settings<<'\n';
 for(unsigned e=0;e<settings_check::Count;++e)out<<settings_check::names[e]<<'='<<results[p].evidence[e]<<'\n';
}
out<<"\nrender_geometry="<<geometryAtStart<<" profiler_fault="<<performance::fault.load()<<" dropped_rows="<<control_compare::dropped.load()<<'\n';
Write(L"frame-times.csv",cadence.str());Write(L"comparison.csv",table.str());
Write(L"gpu-timings.csv",raw.str());Write(L"gpu-stage-summary.csv",stages.str());
Write(L"SUMMARY.txt",out.str());
