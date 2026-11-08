#pragma once
#include "../performance_compare_stats.h"
#include <map>
#include <tuple>
namespace native_lighting::ui::compare_capture {
using namespace control_compare;
enum class State {Idle,Arm,Warm,Measure,Drain,Package};
inline State state=State::Idle;
inline unsigned step=0,savedInterval=128;
inline uint64_t until=0,session=0,skippedStart=0,geometryAtStart=0,lastRR=0,lastRRTime=0;
inline LONGLONG qpcFrequency=0;
inline std::wstring folder;
inline std::string expectedSignature,constantConfig;
inline HANDLE packager=nullptr;
inline bool ioFailed=false;
inline std::string outcome="incomplete";
struct Saved {Mode mode{};bool owned=false,lighting=false;unsigned rays=0,quality=0,volume=0,surface=0,request=0,manualGI=0,contact=0,spec=0;control_rr_gi::StageMode stage{};} saved;
struct Result {uint64_t evidence[settings_check::Count]{};uint64_t skipped=0;unsigned sampleMask=0;bool observed=false;std::string settings;};
inline Result results[phaseCount]{};
struct GPU {unsigned phase;uint64_t present;unsigned stage;double ms;};
inline std::vector<GPU> gpu;
inline bool Active() noexcept{return state!=State::Idle;}
inline bool Write(const wchar_t* name,const std::string& text){
 HANDLE f=CreateFileW((folder+L"\\"+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);
 if(f==INVALID_HANDLE_VALUE){ioFailed=true;return false;}DWORD n=0;
 const bool okay=WriteFile(f,text.data(),DWORD(text.size()),&n,nullptr)&&n==text.size();CloseHandle(f);if(!okay)ioFailed=true;return okay;
}
inline std::string Constants(){
 std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);std::ostringstream out;
 for(const auto& o:control_settings::kOptions){using K=control_settings::Key;
  if(o.key!=K::GlobalIlluminationSamples&&o.key!=K::EnhancedCharacterLighting&&o.key!=K::EnhancedLighting)out<<o.id<<'='<<control_settings::Read(o.key)<<';';}
 out<<"clamp="<<unsigned(control_rr_clamp::requestedMode.load())<<" hdr="<<IsHdr10BridgeActive()<<" preset="<<control_rr::RRUserPresetValue();return out.str();
}
inline void Restore(){
 control_compare::enabled=false;control_compare::running=false;settings_check::End();performance::enabled=false;
 gi_reuse::ownedRequested=saved.owned;control_rr::GISampleSelect(saved.rays);control_visual::characterQuality=saved.quality;
 control_visual::lightingEnabled=saved.lighting;volume_test::phase=saved.volume;character_surface::phase=saved.surface;
 rt_surface_source::request=saved.request;rt_surface_source::manualGI=saved.manualGI;rt_surface_source::manualContact=saved.contact;
 specularSampleChoice=saved.spec;control_rr_gi::stageMode=saved.stage;SelectMode(saved.mode);
 performance::sampleInterval=savedInterval;
}
inline bool Apply(){
 const auto& p=conditions[step];
 gi_reuse::ownedRequested=p.owned;control_rr::GISampleSelect(p.rays);control_visual::characterQuality=p.character?3:0;
 control_visual::lightingEnabled=p.lighting;volume_test::phase=p.volume?0:1;character_surface::phase=0;
 rt_surface_source::request=p.relax?7:8;rt_surface_source::manualGI=0;rt_surface_source::manualContact=0;
 specularSampleChoice=0;control_rr_gi::stageMode=control_rr_gi::StageMode::RawBypass;
 if(!SelectMode(p.lighting?kNativeBlendDefaultMode:Mode::Native))return false;
 expectedSignature=settings_report::Signature();return true;
}
inline void Emit(const performance::Sample& s,double ms){gpu.push_back({step,s.present,unsigned(s.stage),ms});}
inline void Summary(){
 std::vector<double> intervals[phaseCount];std::ostringstream cadence;
 cadence<<"phase,present,interval_ms\n";uint64_t lastTicks=0,lastPresent=0;unsigned lastPhase=~0u;
 const unsigned n=(std::min)(control_compare::count.load(),capacity);
 for(unsigned i=0;i<n;++i){auto& f=frames[i];if(!f.ready.load(std::memory_order_acquire))continue;
  if(f.phase==lastPhase&&f.present==lastPresent+1&&f.ticks>lastTicks&&f.phase<phaseCount){const double ms=1000.*(f.ticks-lastTicks)/qpcFrequency;intervals[f.phase].push_back(ms);cadence<<conditions[f.phase].name<<','<<f.present<<','<<ms<<'\n';}
  lastTicks=f.ticks;lastPresent=f.present;lastPhase=f.phase;
 }
 Write(L"frame-times.csv",cadence.str());
 std::ostringstream raw;raw<<"phase,present,stage,gpu_ms\n";
 std::map<std::tuple<unsigned,unsigned,uint64_t>,double> perPresent;
 for(const auto& s:gpu){raw<<conditions[s.phase].name<<','<<s.present<<','<<performance::Name(performance::Stage(s.stage))<<','<<s.ms<<'\n';perPresent[{s.phase,s.stage,s.present}]+=s.ms;}
 Write(L"gpu-timings.csv",raw.str());
 Statistics stats[phaseCount];std::ostringstream out,table,stage;
 out<<"R388 RR PERFORMANCE COMPARISON\nOutcome: "<<outcome<<"\n\nNative = native GI producer/sample policy and character/volume/lighting replacements off, with this mod's RR integration still ON.\nFull = Owned GI, 4 GI samples, enhanced lighting, RELAX, character/near-volume and volume filter.\nRR, clamp, reflections sample policy (Native), FG OFF and unrelated Visual settings held fixed.\nThe same proxy/hooks remain loaded in every phase. This is not an unmodified executable baseline.\nStand still throughout; moving objects/lights can still invalidate scene matching.\nEach phase: 5 seconds warmup, 12 seconds measurement; GPU queries every 64 Presents; unrelated compute/rays excluded.\n\n";
 table<<"phase,intervals,mean_frame_ms,p50_ms,p95_ms,p99_ms,average_fps,one_percent_low_fps,coverage_ok,skipped_gpu_samples\n";
 for(unsigned p=0;p<phaseCount;++p){stats[p]=Summarize(intervals[p]);auto& s=stats[p];
  table<<conditions[p].name<<','<<s.count<<','<<s.mean<<','<<s.p50<<','<<s.p95<<','<<s.p99<<','<<(s.mean?1000/s.mean:0)<<','<<s.low1<<','<<results[p].observed<<','<<results[p].skipped<<'\n';
  out<<conditions[p].name<<": "<<s.mean<<" ms, "<<(s.mean?1000/s.mean:0)<<" FPS; p99="<<s.p99<<" ms; intervals="<<s.count<<"; required-path coverage="<<results[p].observed<<'\n';
 }
 Write(L"comparison.csv",table.str());
 if(stats[0].mean&&stats[8].mean)out<<"\nBaseline repeat drift: "<<100*(stats[8].mean/stats[0].mean-1)<<"% in mean frame time.\n";
 if(stats[6].mean&&stats[7].mean)out<<"Full repeat drift: "<<100*(stats[7].mean/stats[6].mean-1)<<"% in mean frame time.\n";
 if(stats[0].mean&&stats[6].mean)out<<"Full minus initial native: "<<stats[6].mean-stats[0].mean<<" ms (raw observation, not a causal allocation).\n";
 out<<"Input/output dimensions (packed 16-bit W,H,W,H): "<<geometryAtStart<<"\n";
 out<<"\nStage isolation is incremental: sample policy -> Owned producer/reuse -> lighting policy -> RELAX -> character -> near-volume and main volume filters.\nDo not sum these deltas as independent costs: dependencies, GPU occupancy, CPU bottlenecks and clock drift can interact.\nMore than 5% repeat drift warrants rejecting the comparison. No effect is ranked if required-path coverage is missing.\n\n";
 stage<<"phase,stage,sampled_presents,mean_gpu_ms_per_sampled_present,p95_gpu_ms_per_sampled_present\n";
 for(unsigned p=0;p<phaseCount;++p){
  out<<"\n"<<conditions[p].name<<" evidence:\n";
  out<<"effective_GI_sample_mask="<<results[p].sampleMask<<" (bit N means N samples; bit 31 means 31 or more)\n";
  for(unsigned e=0;e<settings_check::Count;++e)out<<settings_check::names[e]<<'='<<results[p].evidence[e]<<'\n';
  for(unsigned s=1;s<performance::kStageCount;++s){std::vector<double> v;for(const auto& entry:perPresent)if(std::get<0>(entry.first)==p&&std::get<1>(entry.first)==s)v.push_back(entry.second);
   auto x=Summarize(v);if(x.count)stage<<conditions[p].name<<','<<performance::Name(performance::Stage(s))<<','<<x.count<<','<<x.mean<<','<<x.p95<<'\n';}
  const char* cpuNames[]={"receiver_recording","relax_recording_including_buffer_allocations","ngx_evaluation_call"};
  for(unsigned c=0;c<3;++c){auto calls=cpuCalls[p][c].load();out<<cpuNames[c]<<" mean_cpu_ms="<<(calls?1000.*cpuTicks[p][c].load()/qpcFrequency/calls:0)<<" calls="<<calls<<'\n';}
  out<<results[p].settings<<'\n';
 }
 Write(L"gpu-stage-summary.csv",stage.str());
 out<<"\nGPU stages are selected spans, not total frame GPU time. GI rays INCLUDE temporal visibility; do not add the nested visibility span again. RELAX includes prepare/finish conversion. Other compute may overlap explicit scopes.\nCPU spans measure command recording/API work, not whole-frame CPU utilization. Frame intervals are application Present cadence with FG off; they include waits and are not GPU durations.\nNo textures, shader dumps or continuous shadow inventory. Counter/fence boundary differences are normal; failures and unobserved paths are not silently treated as successful measurements.\nProfiler fault="<<performance::fault.load()<<" dropped_frame_rows="<<control_compare::dropped.load()<<"\n";
 Write(L"SUMMARY.txt",out.str());
}
inline void Finish(HWND game,const char* reason){
 outcome=reason;Restore();performance::watch=false;Summary();
 Write(L"settings-restored.txt",settings_report::Snapshot());
 if(ioFailed){state=State::Idle;MessageBoxW(game,L"Some report files could not be saved. Partial files remain under LOCALAPPDATA/ControlFGProbe.",L"Performance comparison incomplete",MB_OK);return;}
 wchar_t sys[MAX_PATH]{},exe[MAX_PATH]{};GetSystemDirectoryW(sys,MAX_PATH);swprintf_s(exe,L"%s\\tar.exe",sys);
 const auto args=L"-a -cf \""+folder+L".zip\" -C \""+folder+L"\" .";
 SHELLEXECUTEINFOW run{sizeof(run)};run.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;run.lpFile=exe;run.lpParameters=args.c_str();run.nShow=SW_HIDE;
 if(ShellExecuteExW(&run)&&run.hProcess){packager=run.hProcess;state=State::Package;until=GetTickCount64()+60000;}
 else {state=State::Idle;MessageBoxW(game,L"Packaging could not start. Settings restored; report folder retained under LOCALAPPDATA/ControlFGProbe.",L"Performance comparison",MB_OK);}
}
inline bool Start(uint64_t now){
 if(Active()||settings_report::Busy()||path_capture::active||path_capture::packager||control_diagnostics::Active()||control_rr_capture_ui::Busy()||capture_suite::active||performance_capture::Active()||control_gi_button::Active()||
    !control_rr::RRUserRequested()||GetFGUserMultiplier()!=kSLSelectionOff||!gi_reuse::SingleProducerEnabled()||!ModeAvailable(kNativeBlendDefaultMode,readyMask.load())||!performance::Prepare())return false;
 if(performance::count+8192>performance::kCapacity)return false;
 LARGE_INTEGER frequency{};if(!QueryPerformanceFrequency(&frequency)||!frequency.QuadPart)return false;qpcFrequency=frequency.QuadPart;
 wchar_t env[MAX_PATH]{};const DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",env,MAX_PATH);if(!len||len>=MAX_PATH)return false;
 const auto root=std::wstring(env)+L"\\ControlFGProbe";CreateDirectoryW(root.c_str(),nullptr);
 folder=root+L"\\Performance-R388-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(now);if(!CreateDirectoryW(folder.c_str(),nullptr))return false;
 saved={LiveSelection::GetMode(selection.Read()),gi_reuse::ownedRequested.load(),control_visual::lightingEnabled.load(),control_rr::GISampleRequested(),control_visual::characterQuality.load(),volume_test::phase.load(),character_surface::phase.load(),rt_surface_source::request.load(),rt_surface_source::manualGI.load(),rt_surface_source::manualContact.load(),specularSampleChoice.load(),control_rr_gi::stageMode.load()};
 ioFailed=false;gpu.clear();gpu.reserve(8192);control_compare::Reset();for(auto& r:results)r=Result{};
 constantConfig=Constants();Write(L"settings-original.txt",settings_report::Snapshot());Write(L"SUMMARY.txt","INCOMPLETE: performance comparison started. Keep game open until two beeps.\n");
 {std::lock_guard<std::recursive_mutex> guard(fgSettingsMutex);CopyFileW(fgSettingsPath.c_str(),(folder+L"\\mod-settings.ini").c_str(),FALSE);}
 wchar_t module[32768]{};GetModuleFileNameW(nullptr,module,32768);if(auto* slash=wcsrchr(module,L'\\')){*slash=0;CopyFileW((std::wstring(module)+L"\\renderer.ini").c_str(),(folder+L"\\renderer.ini").c_str(),FALSE);}
 if(ioFailed)return false;
 AcquireSRWLockExclusive(&performance::lock);for(unsigned i=0;i<performance::count;++i){performance::samples[i].current=false;performance::samples[i].state=performance::State::Discarded;}ReleaseSRWLockExclusive(&performance::lock);
 savedInterval=performance::sampleInterval.load();performance::sampleInterval=64;performance::skipped=0;
 geometryAtStart=0;control_compare::geometry=0;control_compare::running=true;
 step=0;state=State::Arm;until=now+5000;return true;
}
inline void Tick(HWND game,uint64_t now){
 if(!Active())return;
 if(state==State::Package){DWORD code=STILL_ACTIVE;if(GetExitCodeProcess(packager,&code)&&code!=STILL_ACTIVE){CloseHandle(packager);packager=nullptr;state=State::Idle;
   if(code==0){Beep(1000,120);Beep(1000,120);const auto args=L"/select,\""+folder+L".zip\"";ShellExecuteW(game,L"open",L"explorer.exe",args.c_str(),nullptr,SW_SHOWNORMAL);}
   else MessageBoxW(game,L"Packaging failed. Settings restored and report folder retained.",L"Performance comparison",MB_OK);
  }else if(now>until){TerminateProcess(packager,1);CloseHandle(packager);packager=nullptr;state=State::Idle;MessageBoxW(game,L"Packaging timed out. Settings restored and report folder retained.",L"Performance comparison",MB_OK);}return;}
 if(state==State::Arm&&now<until)return;
 if(!game||IsIconic(game)||GetForegroundWindow()!=game){Finish(game,"INCOMPLETE: game lost focus");return;}
 if(Constants()!=constantConfig||!control_rr::RRUserRequested()||GetFGUserMultiplier()!=kSLSelectionOff){Finish(game,"INCOMPLETE: fixed settings changed");return;}
 if(state==State::Arm){if(!Apply()){Finish(game,"INCOMPLETE: feature unavailable");return;}state=State::Warm;until=now+5000;return;}
 if(settings_report::Signature()!=expectedSignature){Finish(game,"INCOMPLETE: compared settings changed");return;}
 if(performance::fault.load()){Finish(game,"INCOMPLETE: GPU timestamp tracking fault");return;}
 if(geometryAtStart&&control_compare::geometry.load()!=geometryAtStart){Finish(game,"INCOMPLETE: rendering resolution changed");return;}
 if(state==State::Warm&&now>=until){
  if(!geometryAtStart)geometryAtStart=control_compare::geometry.load();
  if(!geometryAtStart){Finish(game,"INCOMPLETE: live render dimensions unavailable");return;}
  lastRR=0;lastRRTime=now;
  AcquireSRWLockExclusive(&performance::lock);performance::scanFirst=performance::count;ReleaseSRWLockExclusive(&performance::lock);
  session=performance::capture.fetch_add(1)+1;skippedStart=performance::skipped.load();settings_check::Begin();
  control_compare::phase=step;control_compare::enabled=true;performance::enabled=true;
  state=State::Measure;until=now+12000;Beep(800,80);return;
 }
 if(state==State::Measure){
  const auto rr=settings_check::counts[settings_check::RRSuccess].load();if(rr!=lastRR){lastRR=rr;lastRRTime=now;}
  if(now-lastRRTime>1500){Finish(game,"INCOMPLETE: RR stopped evaluating or game paused");return;}
  performance::Poll(session,Emit);if(now<until)return;
  control_compare::enabled=false;performance::enabled=false;settings_check::End();
  for(unsigned e=0;e<settings_check::Count;++e)results[step].evidence[e]=settings_check::counts[e].load();
  results[step].sampleMask=settings_check::giSampleMask.load();
  results[step].settings=settings_report::Snapshot();results[step].skipped=performance::skipped.load()-skippedStart;
  using namespace settings_check;const auto* e=results[step].evidence;
  results[step].observed=e[RRSuccess]>100&&!e[RRFailure]&&!e[RelaxFallback]&&
    (conditions[step].owned?e[OwnedGI]>100&&!e[NativeGI]:e[NativeGI]>100&&!e[OwnedGI])&&
    (conditions[step].relax?e[RelaxCompleted]>100:e[RelaxRecorded]==0)&&
    (conditions[step].character?e[CharacterEnhanced]>0:e[CharacterEnhanced]==0)&&
    (!conditions[step].volume||e[VolumeEnhanced]>0)&&
    (!conditions[step].lighting||e[EnhancedShadowRays]>0)&&
    (!conditions[step].rays||results[step].sampleMask==(1u<<conditions[step].rays));
  state=State::Drain;until=now+5000;
 }
 if(state==State::Drain){const auto pending=performance::Poll(session,Emit);if(pending){if(now>until)Finish(game,"INCOMPLETE: GPU timings did not retire");return;}
  performance::watch=false;
  if(step+1==phaseCount){
   bool complete=true;for(unsigned p=0;p<phaseCount;++p){bool timed=false;for(const auto& s:gpu)if(s.phase==p&&s.stage==unsigned(performance::Stage::RREvaluation))timed=true;complete=complete&&timed&&results[p].observed;}
   Finish(game,complete?"COMPLETE: inspect repeat drift and timing data":"INCOMPLETE: missing GPU timings or feature coverage");return;}
  ++step;if(!Apply()){Finish(game,"INCOMPLETE: feature unavailable");return;}state=State::Warm;until=now+5000;
 }
}
}
