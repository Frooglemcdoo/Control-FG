#pragma once
// Rendering controls only. No capture, profiling, report, or diagnostic dependencies.
#include "lighting_panel_window.h"
namespace native_lighting::controls {
inline constexpr wchar_t ClassName[]=L"ControlTOHRenderingControlsR367";
inline std::atomic<bool> started{false};
inline bool open=false;
inline int cursorAdds=0;
inline void Close(HWND panel) noexcept {
 open=false;while(cursorAdds>0){ShowCursor(FALSE);--cursorAdds;}
 ShowWindow(panel,SW_HIDE);
}
inline void Refresh(HWND panel) noexcept {
 const auto mode=LiveSelection::GetMode(selection.Read());
 SetWindowTextW(GetDlgItem(panel,1),IsNativeBlend(mode)?L"LIGHTING: NATIVE BLEND":L"LIGHTING: NATIVE");
 SetWindowTextW(GetDlgItem(panel,2),gi_evaluator::Enabled()?gi_evaluator::Label():gi_live_toggle::Label());
 SetWindowTextW(GetDlgItem(panel,3),!control_rr_gi::ownedReady.load()?L"GI FILTER: WAIT":rt_surface_source::ownedFilterEnabled.load()?L"GI FILTER: OWNED":L"GI FILTER: RAW");
 wchar_t text[64]{};const unsigned rays=control_rr::GISampleRequested();
 if(rays)swprintf_s(text,L"GI RAYS: %u",rays);else wcscpy_s(text,L"GI RAYS: NATIVE");SetWindowTextW(GetDlgItem(panel,4),text);
 const auto clamp=control_rr_clamp::requestedMode.load();
 SetWindowTextW(GetDlgItem(panel,5),clamp==control_rr_clamp::Mode::Native?L"CLAMP: NATIVE":clamp==control_rr_clamp::Mode::Bypass?L"CLAMP: BYPASS":L"CLAMP: 60%");
 const unsigned spec=specularSampleChoice.load();if(spec)swprintf_s(text,L"SPEC: %u",spec);else wcscpy_s(text,L"SPEC: NATIVE");SetWindowTextW(GetDlgItem(panel,6),text);
 SetWindowTextW(GetDlgItem(panel,7),UsesNativeBlendProjection(mode)?L"RAYS: PROJECTION":L"RAYS: EMITTER");
 SetWindowTextW(GetDlgItem(panel,8),NativeBlendMapGuard(mode)?L"MAP GUARD: ON":L"MAP GUARD: OFF");
}
inline LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wp,LPARAM lp) noexcept {
 if(message==WM_CREATE){
  CreateWindowW(L"STATIC",L"CONTROLTOH R367 - RENDERING CONTROLS",WS_CHILD|WS_VISIBLE,12,10,396,22,window,nullptr,selfModule,nullptr);
  for(int i=1;i<=8;++i){auto b=CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,12+((i-1)%2)*200,40+((i-1)/2)*42,192,34,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(i)),selfModule,nullptr);SendMessageW(b,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);}
  CreateWindowW(L"STATIC",L"F10 closes. No capture or monitoring is running.",WS_CHILD|WS_VISIBLE,12,216,396,24,window,nullptr,selfModule,nullptr);
  Refresh(window);return 0;
 }
 if(message==WM_CLOSE){Close(window);return 0;}
 if(message==WM_COMMAND && HIWORD(wp)==BN_CLICKED){
  const unsigned id=LOWORD(wp);const auto mode=LiveSelection::GetMode(selection.Read());
  if(id==1)SelectMode(IsNativeBlend(mode)?Mode::Native:kNativeBlendDefaultMode);
  else if(id==2){if(gi_evaluator::Enabled())gi_evaluator::Toggle();else if(gi_live_toggle::ready.load())gi_live_toggle::Toggle();}
  else if(id==3 && control_rr_gi::ownedReady.load())rt_surface_source::ownedFilterEnabled.store(!rt_surface_source::ownedFilterEnabled.load());
  else if(id==4){std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);const auto& option=control_settings::Definition(control_settings::Key::GlobalIlluminationSamples);if(control_settings::Write(option,control_rr::NextGISampleRequest(control_rr::GISampleRequested())))FGSettingsMarkSettingsDirty();}
  else if(id==5)control_rr_clamp::requestedMode.store(control_rr_clamp::Next(control_rr_clamp::requestedMode.load()));
  else if(id==6){const auto next=NextSpecularSampleChoice(specularSampleChoice.load());if(!next||specularSamplesReady.load())SetSpecularSampleChoice(next);}
  else if(id==7)SelectMode(NativeBlendMode(IsNativeBlend(mode)?NativeBlendMapGuard(mode):false,!UsesNativeBlendProjection(mode)));
  else if(id==8)SelectMode(NativeBlendMode(!NativeBlendMapGuard(mode),IsNativeBlend(mode)?UsesNativeBlendProjection(mode):true));
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
  if(game&&!panel)panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,ClassName,L"ControlTOH rendering controls",WS_POPUP|WS_CLIPCHILDREN,0,0,416,250,game,nullptr,selfModule,nullptr);
  const bool foreground=panel&&lighting_panel::Foreground(game,panel,GetForegroundWindow());
  if(foreground&&!registered)registered=RegisterHotKey(nullptr,hotkey,MOD_NOREPEAT,VK_F10)!=FALSE;
  if(!foreground&&registered){UnregisterHotKey(nullptr,hotkey);registered=false;}
  if(!panel)continue;
  if(!foreground){Close(panel);continue;}
  if(open){RECT r{};GetClientRect(game,&r);POINT p{r.right>432?r.right-432:0,16};ClientToScreen(game,&p);SetWindowPos(panel,HWND_TOP,p.x,p.y,416,250,SWP_NOACTIVATE|SWP_SHOWWINDOW);ClipCursor(nullptr);}
 }
 if(registered)UnregisterHotKey(nullptr,hotkey);KillTimer(nullptr,timer);if(panel){Close(panel);DestroyWindow(panel);}started=false;return 0;
}
}
namespace native_lighting {
inline void StartOverlay() noexcept {if(controls::started.exchange(true))return;HANDLE h=CreateThread(nullptr,0,controls::ThreadProc,nullptr,0,nullptr);if(h)CloseHandle(h);else controls::started=false;}
}
