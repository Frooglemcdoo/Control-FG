#pragma once
#include "../gi_path_capture.h"
namespace native_lighting::ui::path_capture {
inline bool active=false,waiting=false;
inline HANDLE packager=nullptr;
inline unsigned condition=0,savedRequest=0;
inline uint64_t deadline=0,selectionAtStart=0;
inline rt_surface_source::Snapshot start{};
inline HANDLE manifest=INVALID_HANDLE_VALUE;
inline void Note(const char* text) noexcept {
 DWORD written=0;if(manifest!=INVALID_HANDLE_VALUE)WriteFile(manifest,text,DWORD(strlen(text)),&written,nullptr);
}
inline void Finish(HWND game,const char* reason) noexcept {
 rt_surface_source::request.store(savedRequest);
 // Slots snapshot their export directory; queued work may safely finish after cancellation.
 gi_path_capture::enabled.store(false);
 Note(reason);Note("\r\n");
 if(manifest!=INVALID_HANDLE_VALUE){FlushFileBuffers(manifest);CloseHandle(manifest);manifest=INVALID_HANDLE_VALUE;}
 active=false;
 if(control_rr_capture_ui::Busy()||control_rr_capture_ui::state.load()==control_rr_capture_ui::State::Failed){
  ShellExecuteW(game,L"open",gi_path_capture::directory,nullptr,nullptr,SW_SHOWNORMAL);
  MessageBoxW(game,L"Capture stopped before completion. Settings restored. Pending GPU copies are retained safely; files may still be saving. See manifest.txt.",L"GI path capture incomplete",MB_OK);return;
 }
 // tar runs asynchronously; the UI and render threads never wait for packaging.
 wchar_t sys[MAX_PATH]{},exe[MAX_PATH]{},args[3*MAX_PATH]{};
 GetSystemDirectoryW(sys,MAX_PATH);swprintf_s(exe,L"%s\\tar.exe",sys);
 swprintf_s(args,L"-a -cf \"%s.zip\" -C \"%s\" .",gi_path_capture::directory,gi_path_capture::directory);
 SHELLEXECUTEINFOW run{sizeof(run)};run.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;run.lpFile=exe;run.lpParameters=args;run.nShow=SW_HIDE;
 if(ShellExecuteExW(&run)&&run.hProcess)packager=run.hProcess;
 else MessageBoxW(game,L"Could not package. The capture folder is available under LOCALAPPDATA/ControlFGProbe.",L"GI path capture",MB_OK);
}
inline bool Start(uint64_t now) noexcept {
 if(active||packager||control_rr_capture_ui::Busy()||control_rr_capture_ui::state.load()==control_rr_capture_ui::State::Failed||
    control_rr_capture_ui::completedPairs.load()>control_rr_capture_ui::PairLimit()-2||
    capture_suite::active||performance_capture::Active()||control_gi_button::Active()||gi_stack_capture::enabled.load()||
    free_camera::commands.Pending()||!IsNativeBlend(LiveSelection::GetMode(selection.Read()))||
    !control_rr::RRUserRuntimeWorkRequested()||GetFGUserMultiplier()!=kSLSelectionOff)return false;
 wchar_t env[MAX_PATH]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",env,MAX_PATH))return false;
 swprintf_s(gi_path_capture::directory,L"%s\\ControlFGProbe\\GI-Path-R379-%lu-%llu",env,GetCurrentProcessId(),now);
 if(!CreateDirectoryW(gi_path_capture::directory,nullptr))return false;
 wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\manifest.txt",gi_path_capture::directory);
 manifest=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);if(manifest==INVALID_HANDLE_VALUE)return false;
 Note("R379 GI path comparison. Two consecutive evaluated frames per condition: RAW then Stable.\r\nFour frames total. Full-screen stride-4 point samples; not full-resolution image-quality proof. Native GPU readback remains full size. No performance measurement.\r\nRaw GI before filter / GI at RR input / HDR scene before RR / RR output / guides share frame IDs. No intermediate temporal-output image is captured.\r\nCounters prove CPU dispatch recording; successful RR output copies retire through GPU fences. Requested state alone is not coverage.\r\nHold still throughout; one beep starts each condition; two beeps after successful packaging. Settings restore on completion/failure.\r\n");
 savedRequest=rt_surface_source::request.load();selectionAtStart=selection.Read();
 for(unsigned i=0;i<3;++i){gi_path_capture::validFrames[i]=0;gi_path_capture::invalidFrames[i]=0;}
 condition=1;waiting=false;deadline=now+5000;active=true;
 gi_path_capture::condition.store(1);gi_path_capture::enabled.store(true);control_diagnostics::captureActive=true;
 rt_surface_source::request.store(8);start=rt_surface_source::Read(8);return true;
}
inline void Tick(HWND game,uint64_t now) noexcept {
 if(packager){
  DWORD code=STILL_ACTIVE;
  if(GetExitCodeProcess(packager,&code)&&code!=STILL_ACTIVE){
   CloseHandle(packager);packager=nullptr;
   wchar_t args[3*MAX_PATH]{};swprintf_s(args,L"/select,\"%s.zip\"",gi_path_capture::directory);
   if(code==0){Beep(1000,120);Beep(1000,120);ShellExecuteW(game,L"open",L"explorer.exe",args,nullptr,SW_SHOWNORMAL);}
   else MessageBoxW(game,L"ZIP packaging failed; raw capture files are retained under LOCALAPPDATA/ControlFGProbe.",L"GI capture",MB_OK);
  }
 }
 if(!active)return;
 if(GetForegroundWindow()!=game||!game||IsIconic(game)){Finish(game,"incomplete: focus lost");return;}
 if(selection.Read()!=selectionAtStart||!control_rr::RRUserRuntimeWorkRequested()||GetFGUserMultiplier()!=kSLSelectionOff){Finish(game,"incomplete: rendering settings changed");return;}
 if(!waiting){if(now<deadline)return;Beep(800,120);
  if(!control_rr_capture_ui::Request()){Finish(game,"incomplete: capture request rejected");return;}
  waiting=true;deadline=now+130000;return;
 }
 const auto state=control_rr_capture_ui::state.load();
 if(state==control_rr_capture_ui::State::Failed){Finish(game,"incomplete: buffer capture failed");return;}
 if(now>deadline){Finish(game,"incomplete: capture timeout");return;}
 if(state!=control_rr_capture_ui::State::Complete)return;
 const unsigned phase=condition==1?8:3;auto delta=rt_surface_source::Read(phase);
 char line[256]{};for(unsigned i=0;i<rt_surface_source::count;++i){delta[i]-=start[i];sprintf_s(line,"condition=%u %s=%llu\r\n",condition,rt_surface_source::names[i],static_cast<unsigned long long>(delta[i]));Note(line);}
 sprintf_s(line,"condition=%u verified_frames=%u invalid_frames=%u\r\n",condition,gi_path_capture::validFrames[condition].load(),gi_path_capture::invalidFrames[condition].load());Note(line);
 if(gi_path_capture::validFrames[condition].load()!=2||gi_path_capture::invalidFrames[condition].load()){Finish(game,"incomplete: frame alignment, filter boundary or full-resolution RAW equality failed; inspect metadata");return;}
 if(!rt_surface_source::Covered(phase,delta)){Finish(game,"incomplete: requested shader path did not record");return;}
 if(condition==2){Finish(game,"complete: both conditions captured; assess video separately for temporal quality");return;}
 condition=2;waiting=false;deadline=now+5000;
 gi_path_capture::condition.store(2);start=rt_surface_source::Read(3);rt_surface_source::request.store(3);
}
}
