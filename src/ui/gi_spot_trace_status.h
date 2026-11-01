#pragma once
// Included inside native_lighting::ui after DrawLabel. Never takes focus or input.
namespace gi_trace_status {
inline HWND window=nullptr,owner=nullptr;
inline LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) noexcept {
 if(message==WM_NCHITTEST)return HTTRANSPARENT;
 if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(message==WM_ERASEBKGND)return 1;
 if(message==WM_PAINT){
  PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
  HBRUSH brush=CreateSolidBrush(control_ui::theme::kBackground);FillRect(dc,&r,brush);DeleteObject(brush);
  DrawLabel(dc,headingFont,L"GI SPOTLIGHT CAPTURE",{14,6,r.right-14,30},control_ui::theme::kAccent);
  DrawLabel(dc,detailFont,gi_spot_trace::Status(),{14,32,r.right-14,68},control_ui::theme::kText);
  EndPaint(hwnd,&ps);return 0;
 }
 return DefWindowProcW(hwnd,message,wp,lp);
}
inline void Tick(HWND game) noexcept {
 if(!gi_spot_trace::Enabled())return;
 if(window&&owner!=game){DestroyWindow(window);window=nullptr;owner=nullptr;}
 if(!game)return;
 if(!window){
  WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=Proc;wc.lpszClassName=L"ControlTOHGITraceStatusR275";
  if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
  window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_LAYERED,wc.lpszClassName,L"GI capture status",WS_POPUP,0,0,740,76,game,nullptr,selfModule,nullptr);
  if(!window){Trace("GI_SPOT_R275_STATUS failed=%lu",GetLastError());return;}
  owner=game;SetLayeredWindowAttributes(window,0,235,LWA_ALPHA);
 }
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 if(pid!=GetCurrentProcessId()||IsIconic(game)){ShowWindow(window,SW_HIDE);return;}
 RECT client{};GetClientRect(game,&client);const int width=(std::min)(740L,(std::max)(1L,client.right-36));
 POINT origin{(std::max)(0L,client.right-width-18),(std::max)(0L,client.bottom-94)};ClientToScreen(game,&origin);
 SetWindowPos(window,HWND_TOPMOST,origin.x,origin.y,width,76,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 static ULONGLONG next=0;if(GetTickCount64()>=next){InvalidateRect(window,nullptr,FALSE);next=GetTickCount64()+100;}
}
inline void Close() noexcept {if(window)DestroyWindow(window);window=owner=nullptr;}
}
