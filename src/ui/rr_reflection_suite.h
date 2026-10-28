#pragma once
#ifdef CONTROL_RR_F9_SUITE
namespace native_lighting::reflection_suite {
inline bool active=false,waiting=false,cancelling=false;
inline unsigned step=0,priorPairs=0;
inline uint64_t began=0;
inline Mode restore=Mode::Native;
inline uint64_t selectionToken=0;
inline const char* stopReason="cancelled";
inline void Package() noexcept {
 wchar_t path[MAX_PATH]{};if(!GetModuleFileNameW(selfModule,path,MAX_PATH))return;
 auto* slash=wcsrchr(path,L'\\');if(!slash)return;*slash=0;
 wchar_t command[2048]{};swprintf_s(command,L"powershell.exe -NoExit -NoProfile -ExecutionPolicy Bypass -File \"%s\\Capture-RR-Suite.ps1\" -GameProcessId %lu -Session %u",path,GetCurrentProcessId(),rr_suite::session.load());
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
 if(CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,path,&startup,&process)){CloseHandle(process.hThread);CloseHandle(process.hProcess);}
 else Trace("rr_suite_package_failed error=%lu",GetLastError());
}
inline void Finish(const char* reason) noexcept {
 rr_suite::requested.store(0);SelectMode(restore);active=false;waiting=false;
 Trace("rr_suite_end session=%u reason=%s completed_steps=%u restored=%s",rr_suite::session.load(),reason,step,ModeName(restore));
 control_rr_capture_ui::detail.store(!strcmp(reason,"complete")?L"F9 sequence saved. Packaging. Move to the next location afterward.":L"F9 sequence stopped. Settings restored; collecting diagnostics.");
 DrainDiagnosticLog(true);Package();
}
inline bool Start(uint64_t now) noexcept {
 using S=control_rr_capture_ui::State;
 const auto state=control_rr_capture_ui::state.load();
 if(active||capture_suite::active||performance_capture::Active()||rr_suite::session.load()>=2||rr_suite::cancelled.load()||
    control_rr_capture_ui::requested.load()||(state!=S::Idle&&state!=S::Complete)||control_rr_capture_ui::completedPairs.load()!=rr_suite::session.load()*rr_suite::count||
    !rr_suite::ready.load()||!specularSamplesReady.load()||!control_rr::RRUserRequested()||slFgEnabledByApi.load())return false;
 restore=LiveSelection::GetMode(selection.Read());
 if(!SelectMode(kNativeBlendDefaultMode))return false;
 selectionToken=LiveSelection::Sequence(selection.Read());++rr_suite::session;step=0;active=true;waiting=false;cancelling=false;began=now;
 rr_suite::requested.store(1);
 Trace("rr_suite_start session=%u steps=%u frames_per_step=2 gain=1 restore=%s",rr_suite::session.load(),rr_suite::count,ModeName(restore));return true;
}
inline void Cancel(const char* reason) noexcept {if(active){cancelling=true;stopReason=reason;}}
inline void Tick(HWND game,uint64_t now) noexcept {
 if(!active)return;
 using S=control_rr_capture_ui::State;
 auto state=control_rr_capture_ui::state.load();
 if(!game||GetForegroundWindow()!=game||IsIconic(game))Cancel("focus_lost");
 if(LiveSelection::Sequence(selection.Read())!=selectionToken)Cancel("selection_changed");
 if(!control_rr::RRUserRequested()||slFgEnabledByApi.load())Cancel("rr_or_fg_changed");
 if(state==S::Failed){rr_suite::cancelled.store(true);Finish("capture_failed");return;}
 // Cancellation stops future recording. Submitted GPU copies still retire normally.
 if(cancelling){rr_suite::cancelled.store(true);Finish(stopReason);return;}
 if(now-began>180000){rr_suite::cancelled.store(true);Finish("step_timeout");return;}
 if(!waiting){
  if(rr_suite::latched.load()!=step+1||rr_suite::dispatchStep.load()!=step+1||now-began<1000)return;
  priorPairs=control_rr_capture_ui::completedPairs.load();
  if(control_rr_capture_ui::Request()){
   waiting=true;Trace("rr_suite_step session=%u step=%u name=%s spec=%u raw_rr_clamp=%u dispatches=%llu",rr_suite::session.load(),step+1,rr_suite::steps[step].name,rr_suite::steps[step].samples,unsigned(rr_suite::steps[step].raw),rr_suite::dispatches.load());
  }
  return;
 }
 if(state==S::Complete&&control_rr_capture_ui::completedPairs.load()==priorPairs+1){
  ++step;waiting=false;
  if(step==rr_suite::count){Finish("complete");return;}
  rr_suite::requested.store(step+1);began=now;
 }
}
}
#endif
