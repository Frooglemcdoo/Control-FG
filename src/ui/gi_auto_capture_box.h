#pragma once
namespace gi_auto_box {
inline HWND box=nullptr,label=nullptr,owner=nullptr;
inline bool f7Held=false;inline ULONGLONG armedAt=0;
inline LRESULT CALLBACK BoxProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)noexcept {
 if(msg==WM_NCHITTEST)return HTTRANSPARENT;if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){PAINTSTRUCT ps{};auto dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
  FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  auto brush=CreateSolidBrush(RGB(255,120,20));for(int i=0;i<3;++i){FrameRect(dc,&r,brush);InflateRect(&r,-1,-1);}DeleteObject(brush);EndPaint(hwnd,&ps);return 0;}
 return DefWindowProcW(hwnd,msg,wp,lp);
}
inline LRESULT CALLBACK LabelProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)noexcept {
 if(msg==WM_NCHITTEST)return HTTRANSPARENT;if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){PAINTSTRUCT ps{};auto dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
  auto b=CreateSolidBrush(control_ui::theme::kBackground);FillRect(dc,&r,b);DeleteObject(b);
  const auto phase=gi_auto_capture::phase.load();wchar_t text[240]{};
  if(!gi_auto_capture::dimensions.load())wcscpy_s(text,L"GI capture: load gameplay with 10 rays and GI FIXED.");
  else if(phase==gi_auto_capture::Preview)wcscpy_s(text,L"Alt + arrows: move orange box | F7: start capture");
  else if(phase==gi_auto_capture::Countdown)swprintf_s(text,L"Box locked. Capture starts in %llu seconds.",(5000-(std::min)(5000ull,GetTickCount64()-armedAt)+999)/1000);
  else if(phase==gi_auto_capture::Collecting)swprintf_s(text,L"Capturing: %u / 10 snapshots saved. Box locked.",gi_auto_capture::completed.load());
  else wcscpy_s(text,L"Capture complete. Collect compact logs. Alt + arrows / F7 to repeat.");
  DrawLabel(dc,detailFont,text,{12,8,r.right-12,r.bottom-8},control_ui::theme::kText);EndPaint(hwnd,&ps);return 0;}
 return DefWindowProcW(hwnd,msg,wp,lp);
}
inline void Close()noexcept {if(box)DestroyWindow(box);if(label)DestroyWindow(label);box=label=owner=nullptr;}
inline void Tick(HWND game)noexcept {
 if(!gi_auto_dispatch::Enabled())return;
 if(owner!=game)Close();if(!game)return;
 if(!box||!label){Close();WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=BoxProc;wc.lpszClassName=L"ControlGIAutoBoxR458";
  if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
  wc.lpfnWndProc=LabelProc;wc.lpszClassName=L"ControlGIAutoLabelR458";if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
  const DWORD ex=WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_LAYERED;
  box=CreateWindowExW(ex,L"ControlGIAutoBoxR458",L"GI capture region",WS_POPUP,0,0,1,1,game,nullptr,selfModule,nullptr);
  label=CreateWindowExW(ex,L"ControlGIAutoLabelR458",L"GI capture controls",WS_POPUP,0,0,700,48,game,nullptr,selfModule,nullptr);
  if(!box||!label){Close();return;}owner=game;SetLayeredWindowAttributes(box,RGB(0,0,0),255,LWA_COLORKEY);SetLayeredWindowAttributes(label,0,235,LWA_ALPHA);
 }
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 const bool focused=pid==GetCurrentProcessId()&&!IsIconic(game);
 const bool f7=(GetAsyncKeyState(VK_F7)&0x8000)!=0;
 if(!focused){ShowWindow(box,SW_HIDE);ShowWindow(label,SW_HIDE);f7Held=f7;return;}
 const auto dimensions=gi_auto_capture::dimensions.load();const UINT w=UINT(dimensions),h=UINT(dimensions>>32);
 auto phase=gi_auto_capture::phase.load();const bool editable=phase==gi_auto_capture::Preview||phase==gi_auto_capture::Complete;
 if(editable&&(GetAsyncKeyState(VK_MENU)&0x8000)){
  auto c=gi_auto_capture::center.load();int x=int(c&65535u),y=int(c>>16);
  if(GetAsyncKeyState(VK_LEFT)&0x8000)x-=5;if(GetAsyncKeyState(VK_RIGHT)&0x8000)x+=5;
  if(GetAsyncKeyState(VK_UP)&0x8000)y-=5;if(GetAsyncKeyState(VK_DOWN)&0x8000)y+=5;
  x=(std::max)(0,(std::min)(1000,x));y=(std::max)(0,(std::min)(1000,y));gi_auto_capture::center.store(UINT(x)|(UINT(y)<<16));
 }
 if(f7&&!f7Held&&editable&&w&&h){
  gi_auto_capture::frozenCenter.store(gi_auto_capture::center.load());gi_auto_capture::completed=0;gi_auto_capture::recorded=0;
  ++gi_auto_capture::request;armedAt=GetTickCount64();gi_auto_capture::phase=gi_auto_capture::Countdown;
  Log("GI_AUTO_R458 box_armed=1 request=%llu",static_cast<unsigned long long>(gi_auto_capture::request.load()));
 }
 f7Held=f7;
 if(gi_auto_capture::phase.load()==gi_auto_capture::Countdown&&GetTickCount64()-armedAt>=5000)gi_auto_capture::phase=gi_auto_capture::Collecting;
 RECT client{};if(!GetClientRect(game,&client)||client.right<=0||client.bottom<=0)return;POINT at{};ClientToScreen(game,&at);
 if(w&&h){const auto roi=gi_auto_capture::Region(w,h);
  const int left=int(UINT64(roi.x)*client.right/w),top=int(UINT64(roi.y)*client.bottom/h);
  const int right=int(UINT64(roi.x+roi.w)*client.right/w),bottom=int(UINT64(roi.y+roi.h)*client.bottom/h);
  SetWindowPos(box,HWND_TOPMOST,at.x+left,at.y+top,right-left,bottom-top,SWP_NOACTIVATE|SWP_SHOWWINDOW);InvalidateRect(box,nullptr,FALSE);
 }else ShowWindow(box,SW_HIDE);
 const int labelWidth=(std::min)(740L,client.right);
 SetWindowPos(label,HWND_TOPMOST,at.x+(client.right-labelWidth)/2,at.y+(std::max)(0L,client.bottom-64),labelWidth,48,SWP_NOACTIVATE|SWP_SHOWWINDOW);InvalidateRect(label,nullptr,FALSE);
}
}
