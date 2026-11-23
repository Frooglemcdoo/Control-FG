#pragma once
// R489: approved rendering controls and the existing explicitly armed motion capture.
#include "theme.h"
#include "render_fps.h"
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")
#include "gi_auto_capture_box.h"
#include "lighting_panel_window.h"
namespace native_lighting::controls {
inline constexpr wchar_t ClassName[]=L"ControlTOHRRControlsR489";
inline std::atomic<bool> started{false};
inline bool open=false;
inline int cursorAdds=0;
inline void Close(HWND panel) noexcept {
 open=false;while(cursorAdds>0){ShowCursor(FALSE);--cursorAdds;}
 ShowWindow(panel,SW_HIDE);
}
inline void Refresh(HWND panel) noexcept {
 SetWindowTextW(GetDlgItem(panel,1),control_rr::RRUserRequested()?L"RR: ON":L"RR: OFF");
 SetWindowTextW(GetDlgItem(panel,2),gi_evaluator::Enabled()?gi_evaluator::Label():gi_live_toggle::Label());
 const auto clamp=control_rr_clamp::requestedMode.load();
 SetWindowTextW(GetDlgItem(panel,3),clamp==control_rr_clamp::Mode::Native?L"REF CLAMP: NATIVE":clamp==control_rr_clamp::Mode::Bypass?L"REF CLAMP: BYPASS":L"REF CLAMP: 60%");
 wchar_t text[64]{};const unsigned fixedRays=gi_auto_dispatch::LaunchRays();
 const unsigned rays=fixedRays?fixedRays:control_rr::GISampleRequested();
 if(fixedRays)swprintf_s(text,L"GI RAYS: %u",rays);
 else if(rays)swprintf_s(text,L"GI RAYS: %u",rays);else wcscpy_s(text,L"GI RAYS: NATIVE");SetWindowTextW(GetDlgItem(panel,4),text);
 SetWindowTextW(GetDlgItem(panel,5),control_rr_capture_ui::Busy()?L"CAPTURE: RECORDING":L"CAPTURE MOTION");
 SetWindowTextW(GetDlgItem(panel,6),L"PACKAGE CAPTURE");
 SetWindowTextW(GetDlgItem(panel,9),control_rr::RRUserLabel());
 SetWindowTextW(GetDlgItem(panel,10),gi_scene_capture::Motion()?control_rr_capture_ui::detail.load():L"Capture requires Start-GI-Motion-R489.cmd.");
#ifdef CONTROL_CHARACTER_GI_BLEND
 const bool enabled=rrCharacterBlendRequested.load();
 SetWindowTextW(GetDlgItem(panel,7),enabled?L"CHARACTER BLEND: ON":L"CHARACTER BLEND: OFF");
 SetWindowTextW(GetDlgItem(panel,11),enabled?rrCharacterBlendStatus.load():L"Disabled");
#endif
 for(int i=1;i<=5;++i)EnableWindow(GetDlgItem(panel,i),!control_rr_capture_ui::Busy()&&(i!=4||!fixedRays));
}
inline void Package(HWND panel) noexcept {
 wchar_t path[32768]{};if(!GetModuleFileNameW(selfModule,path,32768))return;
 auto slash=wcsrchr(path,L'\\');if(!slash)return;*slash=0;
 if(wcscat_s(path,L"\\ControlTOH\\Diagnostics\\Package-GI-Motion.cmd"))return;
 if(reinterpret_cast<INT_PTR>(ShellExecuteW(panel,L"open",path,nullptr,nullptr,SW_SHOWNORMAL))<=32)
  MessageBoxW(panel,L"Could not open Package-GI-Motion.cmd. Check the installed ControlTOH/Diagnostics folder.",L"Capture packaging",MB_OK|MB_ICONERROR);
}
inline void Capture(HWND panel) noexcept {
 if(!gi_auto_dispatch::Enabled()||!gi_scene_capture::Motion()){
  MessageBoxW(panel,L"Close Control and launch ControlTOH/Diagnostics/Start-GI-Motion-R489.cmd. Then use this button or Alt+Insert. Normal launches do not prepare the full-scene capture resources.",L"Motion capture",MB_OK);return;
 }
 if(!control_rr::RRUserRuntimeWorkRequested()||GetFGUserMultiplier()!=0||!gi_auto_capture::dimensions.load()||!gi_reuse::ownedRequested.load()){
  MessageBoxW(panel,L"Load gameplay with RR ON, FG OFF and GI FIXED before recording.",L"Motion capture",MB_OK);return;
 }
 if(!control_rr_capture_ui::Request()){
  MessageBoxW(panel,L"Capture is busy or this launch already recorded its sequence. Package the existing capture first.",L"Motion capture",MB_OK);return;
 }
 ++gi_auto_capture::request;gi_auto_capture::completed=0;gi_auto_capture::recorded=0;
 gi_auto_capture::phase=gi_auto_capture::Collecting;
 control_diagnostics::PublishCaptureActivity(false,true,true);
 Close(panel);
}
inline LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wp,LPARAM lp) noexcept {
 if(message==WM_CREATE){
  CreateWindowW(L"STATIC",L"CONTROLTOH R496 - RR CONTROLS",WS_CHILD|WS_VISIBLE,12,10,396,22,window,nullptr,selfModule,nullptr);
  for(int i=1;i<=6;++i){auto b=CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,12+((i-1)%2)*200,40+((i-1)/2)*42,192,34,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(i)),selfModule,nullptr);SendMessageW(b,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);}
  CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,12,172,396,24,window,reinterpret_cast<HMENU>(9),selfModule,nullptr);
  CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,12,200,396,36,window,reinterpret_cast<HMENU>(10),selfModule,nullptr);
#ifdef CONTROL_CHARACTER_GI_BLEND
  CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP,12,237,192,34,window,reinterpret_cast<HMENU>(7),selfModule,nullptr);
  CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,213,245,195,28,window,reinterpret_cast<HMENU>(11),selfModule,nullptr);
#endif
  Refresh(window);return 0;
 }
 if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(message==WM_CLOSE){Close(window);return 0;}
 if(message==WM_COMMAND && HIWORD(wp)==BN_CLICKED){
  const unsigned id=LOWORD(wp);
  if(id!=6&&control_rr_capture_ui::Busy())return 0;
  if(id==1){std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);if(control_settings::Write(control_settings::Definition(control_settings::Key::RayReconstruction),!control_rr::RRUserRequested()))FGSettingsMarkSettingsDirty();}
  else if(id==2){if(gi_evaluator::Enabled())gi_evaluator::Toggle();else if(gi_live_toggle::ready.load())gi_live_toggle::Toggle();}
  else if(id==3)control_rr_clamp::requestedMode.store(control_rr_clamp::Next(control_rr_clamp::requestedMode.load()));
  else if(id==4&&!gi_auto_dispatch::LaunchRays()){std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);if(control_settings::Write(control_settings::Definition(control_settings::Key::GlobalIlluminationSamples),control_rr::NextGISampleRequest(control_rr::GISampleRequested())))FGSettingsMarkSettingsDirty();}
  else if(id==5)Capture(window);
  else if(id==6)Package(window);
#ifdef CONTROL_CHARACTER_GI_BLEND
  else if(id==7)rrCharacterBlendRequested.store(!rrCharacterBlendRequested.load());
#endif
  Refresh(window);return 0;
 }
 if(message==WM_CTLCOLORSTATIC){SetTextColor(reinterpret_cast<HDC>(wp),RGB(240,240,240));SetBkColor(reinterpret_cast<HDC>(wp),RGB(24,24,24));return reinterpret_cast<LRESULT>(GetStockObject(DKGRAY_BRUSH));}
 return DefWindowProcW(window,message,wp,lp);
}
inline DWORD WINAPI ThreadProc(LPVOID) noexcept {
 WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=WindowProc;wc.lpszClassName=ClassName;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
 if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){started=false;return 0;}
 const UINT_PTR timer=SetTimer(nullptr,0,100,nullptr);if(!timer){started=false;return 0;}
 constexpr int hotkey=0x5367;bool registered=false;HWND game=nullptr,panel=nullptr;ULONGLONG nextFind=0;
 MSG msg{};
 while(GetMessageW(&msg,nullptr,0,0)>0){
  if(msg.message==WM_HOTKEY && msg.wParam==hotkey){
   if(panel&&lighting_panel::Foreground(game,panel,GetForegroundWindow())){
    if(open)Close(panel);else {open=true;Refresh(panel);int count;do{count=ShowCursor(TRUE);++cursorAdds;}while(count<0);}
   }
   continue;
  }
  if(msg.hwnd){TranslateMessage(&msg);DispatchMessageW(&msg);}
  if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
  const auto now=GetTickCount64();
  if(now>=nextFind){auto found=FindFGGameWindow();nextFind=now+1000;
   if(found!=game){if(panel){Close(panel);DestroyWindow(panel);panel=nullptr;}game=found;}
  }
  if(game&&!panel)panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,ClassName,L"ControlTOH rendering controls",WS_POPUP|WS_CLIPCHILDREN,0,0,416,280,game,nullptr,selfModule,nullptr);
  const bool foreground=panel&&lighting_panel::Foreground(game,panel,GetForegroundWindow());
  if(foreground&&!registered)registered=RegisterHotKey(nullptr,hotkey,MOD_NOREPEAT,VK_F10)!=FALSE;
  if(!foreground&&registered){UnregisterHotKey(nullptr,hotkey);registered=false;}
  const bool capturing=control_rr_capture_ui::Busy()||control_rr_capture_ui::requested.load();
  control_diagnostics::PublishCaptureActivity(false,capturing,gi_scene_capture::Enabled());
  if(capturing){gi_phase_audit::Poll();DrainDiagnosticLog();}

  render_fps::Tick(game,now);
  if(!panel)continue;
  if(!foreground){Close(panel);continue;}
  if(open){Refresh(panel);RECT r{};GetClientRect(game,&r);POINT p{r.right>432?r.right-432:0,16};ClientToScreen(game,&p);RECT previous{};GetWindowRect(panel,&previous);const bool visible=IsWindowVisible(panel)!=FALSE;if(!visible||previous.left!=p.x||previous.top!=p.y)SetWindowPos(panel,HWND_TOP,p.x,p.y,416,280,SWP_NOACTIVATE|SWP_SHOWWINDOW|(visible?SWP_NOZORDER:0));if(GetForegroundWindow()==game||GetForegroundWindow()==panel)ClipCursor(nullptr);}
 }
 gi_auto_box::Close();if(registered)UnregisterHotKey(nullptr,hotkey);KillTimer(nullptr,timer);if(panel){Close(panel);DestroyWindow(panel);}started=false;return 0;
}
}
namespace native_lighting {
inline void StartOverlay() noexcept {if(controls::started.exchange(true))return;HANDLE h=CreateThread(nullptr,0,controls::ThreadProc,nullptr,0,nullptr);if(h)CloseHandle(h);else controls::started=false;}
}
