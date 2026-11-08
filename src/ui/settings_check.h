#pragma once
#include "../settings_check_state.h"
#include <sstream>
namespace native_lighting::ui::settings_report {
inline bool active=false,recording=false,changed=false,focusLost=false,ioFailed=false;
inline HANDLE packager=nullptr;
inline uint64_t deadline=0,started=0,presentStart=0,packagingStarted=0,fgStateStart=0;
inline std::wstring directory;
inline std::string startSignature;
inline unsigned rrRequest=0,ownedRequest=0,filterRequest=0,characterRequest=0,lightingRequest=0,clampRequest=0,fgRequest=0;
inline bool Busy() noexcept {return active||packager;}
inline bool Write(const wchar_t* name,const std::string& text) noexcept {
 const auto path=directory+L"\\"+name;
 HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);
 if(f==INVALID_HANDLE_VALUE){ioFailed=true;return false;}
 DWORD n=0;const bool ok=WriteFile(f,text.data(),DWORD(text.size()),&n,nullptr)&&n==text.size();
 CloseHandle(f);if(!ok)ioFailed=true;return ok;
}
inline std::string Signature() {
 std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
 std::ostringstream out;
 for(auto v:control_settings::Snapshot())out<<v<<',';
 out<<unsigned(LiveSelection::GetMode(selection.Read()))<<','<<gi_reuse::ownedRequested.load()<<','
    <<rt_surface_source::request.load()<<','<<rt_surface_source::manualGI.load()<<','<<rt_surface_source::manualContact.load()<<','
    <<unsigned(control_rr_clamp::requestedMode.load())<<','<<specularSampleChoice.load();
 return out.str();
}
inline std::string Snapshot() {
 std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
 std::ostringstream out;
 // Names in the exact schema order, including hidden/legacy settings.
 constexpr const char* names[]={"Frame Generation","Dynamic Target Mode","Dynamic Target FPS","Ray Reconstruction","Indirect Lighting Samples","Glass Reflections","DirectStorage","Texture Streaming Budget","VRAM Monitor","Experimental RTX40 Multi-FG","Enhanced Character Lighting","Enhanced Lighting and Shadows","FPS Counter"};
 static_assert(sizeof(names)/sizeof(names[0])==control_settings::kOptionCount,"Update settings report names when schema changes");
 unsigned i=0;for(const auto& option:control_settings::kOptions){
  out<<names[i++]<<" [id="<<option.id<<"] requested="<<control_settings::Read(option.key)
     <<" default="<<control_settings::Default(option)<<" available="<<control_settings::Enabled(option)<<'\n';
 }
 out<<"lighting_mode="<<unsigned(LiveSelection::GetMode(selection.Read()))<<"\nowned_requested="<<gi_reuse::ownedRequested.load()
    <<"\nGI_capture_override="<<rt_surface_source::request.load()<<"\nGI_manual_override="<<rt_surface_source::manualGI.load()
    <<"\ncontact_manual_override="<<rt_surface_source::manualContact.load()<<"\nreflection_sample_policy="<<specularSampleChoice.load()<<" (0=Native)"
    <<"\nclamp_requested_percent="<<control_rr_clamp::RuntimeRequested()<<"\nclamp_last_effective_percent="<<control_rr_clamp::effective.load()
    <<"\nrr_status="<<unsigned(control_rr::rrUserStatus.load())<<" (0=Off,1=Waiting,2=Active,3=Stopped,4=Recovering,5=Resizing)"
    <<"\nrr_wait_reason="<<unsigned(control_rr::rrUserWaitReason.load())
    <<"\nrr_runtime_pause_mask="<<control_rr::RRUserRuntimePauseReasons()
    <<"\nFG_selection_applied="<<IsFGSelectionAppliedForOverlay()<<"\nFG_last_sample_multiplier="<<GetFGEffectiveMultiplierForOverlay()
    <<"\nFG_last_sample_present="<<GetFGLastStatePresentForOverlay()
    <<"\nDirectStorage_supported="<<IsFGDirectStorageSupported()
    <<"\nFPS_window_visible="<<(render_fps::window&&IsWindowVisible(render_fps::window))
    <<"\nbroad_diagnostics_active="<<control_diagnostics::Active()<<'\n';
 return out.str();
}
inline DWORD CopySaved(const std::wstring& source,const wchar_t* name) noexcept {
 if(source.empty())return ERROR_FILE_NOT_FOUND;
 if(CopyFileW(source.c_str(),(directory+L"\\"+name).c_str(),FALSE))return ERROR_SUCCESS;
 return GetLastError();
}
inline void CaptureRequests() noexcept {
 rrRequest=control_rr::RRUserRequested();ownedRequest=gi_reuse::ownedRequested.load();
 const auto filter=rt_surface_source::EffectiveRequest();
 filterRequest=rrRequest&&(filter==7||filter==3||(!filter&&control_rr_gi::stageMode.load()==control_rr_gi::StageMode::BothSteadySpatial));
 characterRequest=control_visual::characterQuality.load()==3;
 lightingRequest=control_visual::lightingEnabled.load();clampRequest=control_rr_clamp::RuntimeRequested();fgRequest=GetFGUserMultiplier();
}
inline void Finish(HWND game,const char* outcome,uint64_t now) {
 settings_check::End();active=false;
 std::ostringstream out;
 out<<"ControlTOH R386 - CHECK SETTINGS\nOutcome: "<<outcome<<"\nCollection time ms: "<<(recording?now-started:0)
    <<"\nSettings changed: "<<changed<<"\nFocus lost: "<<focusLost<<"\nRender presents: "<<(recording?presentCount.load()-presentStart:0)<<"\n\n";
 using namespace settings_check;
 const auto count=[](Event e){return counts[e].load();};
 out<<"RR: "<<Verdict(rrRequest!=0,count(RRSuccess),count(RRFailure))<<" (NGX evaluation return status)\n"
    <<"Owned GI: "<<Verdict(ownedRequest!=0,count(OwnedGI))<<" (dispatch recording)\n"
    <<"RELAX: "<<Verdict(filterRequest!=0,count(RelaxCompleted),count(RelaxFallback))<<" (submitted work reached GPU fence)\n"
    <<"Enhanced character: "<<Verdict(characterRequest!=0,count(CharacterEnhanced))<<" (binding recording; keep Jesse visible)\n"
    <<"Enhanced lighting: "<<Verdict(lightingRequest!=0,count(LightingEnhanced))<<" (binding recording)\n"
    <<"Near-volume enhanced bindings: "<<count(VolumeEnhanced)<<"; native bindings: "<<count(VolumeNative)<<" (scene-dependent)\n"
    <<"Clamp60: "<<Verdict(clampRequest==60,count(Clamp60))<<" (scoped replacement invocation)\n"
    <<"FG: "<<(fgRequest==0?"OFF (see applied state below)":IsFGSelectionAppliedForOverlay()&&GetFGLastStatePresentForOverlay()>fgStateStart?"FRESH_RUNTIME_STATE_OBSERVED":"NOT_CONFIRMED")<<" (runtime report; not a display-latency measurement)\n\n";
 out<<"GI effective sample counts observed at native allocation/shader query:";
 for(unsigned i=0;i<32;++i)if(giSampleMask.load()&(1u<<i))out<<' '<<i;
 out<<" (31 means 31 or higher; empty means NOT_OBSERVED)\n";
 for(unsigned i=0;i<Count;++i)out<<names[i]<<'='<<counts[i].load()<<'\n';
 out<<"\nDUPLICATE-WORK AUDIT\n";
 if(filterRequest&&count(RelaxCompleted)&&(count(GINativeTemporal)||count(GINativeSpatial)||count(GINativeFilterFallback)))
  out<<"REVIEW: native GI filter work and RELAX were both observed in this window. This can be fallback/transition activity; totals alone do not prove both processed the same frame.\n";
 else if(filterRequest&&count(RelaxCompleted))out<<"No native GI temporal/spatial filtering observed alongside RELAX in this window.\n";
 else out<<"GI replacement coverage insufficient to assess overlap.\n";
 out<<"Owned ray generation replaces native ray generation; temporal visibility is a separate additional pass.\nRELAX replaces the GI filter, but receiver export, prepare/finish and NRD internal passes are additional work.\nNative shadow filtering is retained where it filters a separate shadow signal; it is not a second GI producer.\nNative specular/broad diffuse filters with RR should normally be zero in the default Native Blend mode. Diagnostic modes can intentionally preserve them.\nNo GPU duration is inferred from counts.\n";
 out<<"last_RELAX_rejection="<<reason.load()<<"\n\nEND SNAPSHOT\n"<<Snapshot();
 out<<"\nInterpretation:\nOBSERVED verifies only the evidence described, not image quality or every frame.\nNOT_OBSERVED means coverage missing, not a proven defect. OFF does not prove native fallback works.\nCHECK_FAILED means the window included an error or rejected input; successful frames may also be present.\nSettings changes, focus loss, zero render presents or an incomplete outcome invalidate a whole-window comparison.\nCounts at the collection boundaries are approximate; RELAX completion excludes earlier generations.\nNative renderer.ini is saved configuration, not proof of live native graphics values. Missing files are reported in saved-files.txt.\nReflection sample values, DLSS/HDR/native quality options, glass, DirectStorage transfers and VRAM monitoring are configuration-only unless runtime evidence is explicitly listed.\nNo universal ALL PASS is issued. This is not a performance benchmark. No textures, images, ETW/WPR or shadow inventory are captured.\n";
 Write(L"SUMMARY.txt",out.str());Write(L"settings-end.json",control_native_menu::Configuration());
 if(ioFailed){MessageBoxW(game,L"Some report files could not be written. The partial folder is retained under LOCALAPPDATA/ControlFGProbe.",L"Settings check incomplete",MB_OK);return;}
 wchar_t sys[MAX_PATH]{},exe[MAX_PATH]{};GetSystemDirectoryW(sys,MAX_PATH);swprintf_s(exe,L"%s\\tar.exe",sys);
 const std::wstring args=L"-a -cf \""+directory+L".zip\" -C \""+directory+L"\" .";
 SHELLEXECUTEINFOW run{sizeof(run)};run.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;run.lpFile=exe;run.lpParameters=args.c_str();run.nShow=SW_HIDE;
 if(ShellExecuteExW(&run)&&run.hProcess){packager=run.hProcess;packagingStarted=now;}
 else MessageBoxW(game,L"ZIP packaging could not start. The report folder is retained under LOCALAPPDATA/ControlFGProbe.",L"Settings check",MB_OK);
}
inline bool Start(uint64_t now) {
 if(Busy()||path_capture::active||path_capture::packager||control_diagnostics::Active()||control_rr_capture_ui::Busy()||capture_suite::active||performance_capture::Active()||control_gi_button::Active())return false;
 wchar_t env[MAX_PATH]{};DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,MAX_PATH);if(!length||length>=MAX_PATH)return false;
 const auto root=std::wstring(env)+L"\\ControlFGProbe";
 if(!CreateDirectoryW(root.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
 directory=root+L"\\Settings-R386-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(now);
 if(!CreateDirectoryW(directory.c_str(),nullptr))return false;
 ioFailed=false;changed=false;focusLost=false;recording=false;
 settings_check::Reset();CaptureRequests();fgStateStart=GetFGLastStatePresentForOverlay();
 std::ostringstream files;
 {std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
  files<<"mod-settings.ini copy Win32 error="<<CopySaved(fgSettingsPath,L"mod-settings.ini")<<'\n';}
 wchar_t gamePath[32768]{};const DWORD n=GetModuleFileNameW(nullptr,gamePath,32768);
 std::wstring native;if(n&&n<32768){native=gamePath;const auto slash=native.find_last_of(L"\\/");native=slash==std::wstring::npos?L"":native.substr(0,slash+1)+L"renderer.ini";}
 files<<"renderer.ini copy Win32 error="<<CopySaved(native,L"renderer.ini")<<"\n0=success,2=file missing,5=access denied. Saved files are copied without changing them.\n";
 Write(L"saved-files.txt",files.str());
 Write(L"SUMMARY.txt","Settings check armed. INCOMPLETE until this file is replaced after the observation window. Keep the game open until two beeps.\n");
 Write(L"settings-start.json",control_native_menu::Configuration());Write(L"settings-start.txt",Snapshot());
 if(ioFailed)return false;
 active=true;deadline=now+5000;return true;
}
inline void Tick(HWND game,uint64_t now) {
 if(packager){
  DWORD code=STILL_ACTIVE;
  if(GetExitCodeProcess(packager,&code)&&code!=STILL_ACTIVE){
   CloseHandle(packager);packager=nullptr;
   if(code==0){Beep(1000,120);Beep(1000,120);const auto args=L"/select,\""+directory+L".zip\"";ShellExecuteW(game,L"open",L"explorer.exe",args.c_str(),nullptr,SW_SHOWNORMAL);}
   else MessageBoxW(game,L"ZIP packaging failed. Report files are retained under LOCALAPPDATA/ControlFGProbe.",L"Settings check",MB_OK);
  }else if(now-packagingStarted>60000){TerminateProcess(packager,1);CloseHandle(packager);packager=nullptr;MessageBoxW(game,L"ZIP packaging timed out. Report files are retained under LOCALAPPDATA/ControlFGProbe.",L"Settings check",MB_OK);}
 }
 if(!active)return;
 if(!recording){
  if(now<deadline)return;
  if(!game||IsIconic(game)||GetForegroundWindow()!=game){focusLost=true;Finish(game,"INCOMPLETE: game not foreground",now);return;}
  startSignature=Signature();Write(L"settings-start.json",control_native_menu::Configuration());Write(L"settings-start.txt",Snapshot());
  CaptureRequests();
  fgStateStart=GetFGLastStatePresentForOverlay();presentStart=presentCount.load();
  Beep(800,120);started=GetTickCount64();deadline=started+10000;recording=true;settings_check::Begin();return;
 }
 if(!game||IsIconic(game)||GetForegroundWindow()!=game){focusLost=true;Finish(game,"INCOMPLETE: focus lost",now);return;}
 if(Signature()!=startSignature){changed=true;Finish(game,"INCOMPLETE: settings changed",now);return;}
 if(now>=deadline)Finish(game,presentCount.load()>presentStart?"OBSERVATION FINISHED - inspect individual results":"INCOMPLETE: no render presents",now);
}
}
