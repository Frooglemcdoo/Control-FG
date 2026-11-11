#pragma once
// Included inside native_lighting::ui after DrawLabel. Never takes focus or input.
namespace gi_trace_status {
inline HWND window=nullptr,owner=nullptr,frameWindow=nullptr;
inline bool referenceAttempted=false;
inline LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) noexcept {
 if(message==WM_NCHITTEST)return HTTRANSPARENT;
 if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(message==WM_ERASEBKGND)return 1;
 if(message==WM_PAINT){
  PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
  HBRUSH brush=CreateSolidBrush(control_ui::theme::kBackground);FillRect(dc,&r,brush);DeleteObject(brush);
  DrawLabel(dc,headingFont,L"GI LIGHTING CAPTURE",{14,6,r.right-14,30},control_ui::theme::kAccent);
  DrawLabel(dc,detailFont,gi_spot_trace::Status(),{14,32,r.right-14,68},control_ui::theme::kText);
  EndPaint(hwnd,&ps);return 0;
 }
 return DefWindowProcW(hwnd,message,wp,lp);
}
inline LRESULT CALLBACK FrameProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) noexcept {
 if(message==WM_NCHITTEST)return HTTRANSPARENT;
 if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(message==WM_ERASEBKGND)return 1;
 if(message==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
  FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  HBRUSH b=CreateSolidBrush(RGB(255,80,20));FrameRect(dc,&r,b);DeleteObject(b);
EndPaint(hwnd,&ps);return 0;}
 return DefWindowProcW(hwnd,message,wp,lp);
}
// Best-effort SDR desktop framing reference, NOT a synchronized GPU/HDR capture.
inline void SaveReference(HWND game) noexcept {
 RECT r{};if(!GetClientRect(game,&r)||r.right<=0||r.bottom<=0||r.right>16384||r.bottom>16384||static_cast<UINT64>(r.right)*r.bottom>16777216)return;
 POINT origin{};ClientToScreen(game,&origin);HDC screen=GetDC(nullptr),memory=CreateCompatibleDC(screen);
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);bool ok=false;
 if(memory&&bitmap){auto old=SelectObject(memory,bitmap);
  if(BitBlt(memory,0,0,r.right,r.bottom,screen,origin.x,origin.y,SRCCOPY|CAPTUREBLT)){
   GdiFlush();wchar_t path[MAX_PATH]{};AcquireSRWLockExclusive(&reportLock);const bool opened=report.OpenDefault();if(opened)wcscpy_s(path,report.Path());ReleaseSRWLockExclusive(&reportLock);
   if(opened){auto* slash=wcsrchr(path,static_cast<wchar_t>(92));if(slash){*(slash+1)=0;wchar_t name[80]{};swprintf_s(name,L"GI-Joint-R285-%lu-reference.bmp",GetCurrentProcessId());wcscat_s(path,name);
    HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f!=INVALID_HANDLE_VALUE){BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(BITMAPINFOHEADER);const DWORD size=static_cast<DWORD>(r.right*r.bottom*4);h.bfSize=h.bfOffBits+size;DWORD n=0;
     ok=WriteFile(f,&h,sizeof(h),&n,nullptr)&&n==sizeof(h)&&WriteFile(f,&info.bmiHeader,sizeof(BITMAPINFOHEADER),&n,nullptr)&&n==sizeof(BITMAPINFOHEADER)&&WriteFile(f,pixels,size,&n,nullptr)&&n==size;CloseHandle(f);
    }
   }}
  }SelectObject(memory,old);
 }
 if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);if(screen)ReleaseDC(nullptr,screen);
 Trace("GI_JOINT_R285_REFERENCE saved=%u width=%ld height=%ld present=%llu framing_only=1 synchronized=0 hdr=0",unsigned(ok),r.right,r.bottom,presentCount.load());
}
inline void FrameTick(HWND game,const RECT& client) noexcept {
 if(!gi_spot_trace::JointTrace()&&!gi_spot_trace::OwnedTrace())return;
 const UINT gx=gi_spot_trace::gridWidth.load(),gy=gi_spot_trace::gridHeight.load();if(!gx||!gy)return;
 if(!frameWindow){WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=FrameProc;wc.lpszClassName=L"ControlTOHGIFrameR284";
  if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
  frameWindow=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_LAYERED,wc.lpszClassName,L"GI sampled region",WS_POPUP,0,0,1,1,game,nullptr,selfModule,nullptr);
  if(!frameWindow)return;SetLayeredWindowAttributes(frameWindow,RGB(0,0,0),255,LWA_COLORKEY);
 }
 const auto phase=gi_spot_trace::phase.load();const bool visible=phase==gi_spot_trace::Phase::Idle||phase==gi_spot_trace::Phase::Countdown;
 if(!visible){ShowWindow(frameWindow,SW_HIDE);return;}
 int width=client.right,height=client.bottom;int offsetX=0,offsetY=0;
 if(gi_spot_trace::OwnedTrace()){
  UINT roi[4]{};gi_spot_trace::OwnedROI(gx,gy,roi);
  offsetX=int(UINT64(roi[0])*width/gx);offsetY=int(UINT64(roi[1])*height/gy);
  width=int(UINT64(roi[2])*width/gx);height=int(UINT64(roi[3])*height/gy);
 }
 POINT at{};ClientToScreen(game,&at);
 SetWindowPos(frameWindow,HWND_TOPMOST,at.x+offsetX,at.y+offsetY,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);InvalidateRect(frameWindow,nullptr,FALSE);
 if(!gi_spot_trace::OwnedTrace()&&!referenceAttempted&&phase==gi_spot_trace::Phase::Countdown&&GetTickCount64()-gi_spot_trace::armedAt>=4000){referenceAttempted=true;SaveReference(game);}
}
inline void Tick(HWND game) noexcept {
 if(!gi_spot_trace::Enabled())return;
 if(window&&owner!=game){DestroyWindow(window);if(frameWindow)DestroyWindow(frameWindow);frameWindow=nullptr;window=nullptr;owner=nullptr;}
 if(!game)return;
 if(!window){
  WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=Proc;wc.lpszClassName=L"ControlTOHGITraceStatusR275";
  if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
  window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_LAYERED,wc.lpszClassName,L"GI capture status",WS_POPUP,0,0,740,76,game,nullptr,selfModule,nullptr);
  if(!window){Trace("GI_SPOT_R275_STATUS failed=%lu",GetLastError());return;}
  owner=game;SetLayeredWindowAttributes(window,0,235,LWA_ALPHA);
 }
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 if(pid!=GetCurrentProcessId()||IsIconic(game)){ShowWindow(window,SW_HIDE);if(frameWindow)ShowWindow(frameWindow,SW_HIDE);return;}
 RECT client{};GetClientRect(game,&client);FrameTick(game,client);const int width=(std::min)(740L,(std::max)(1L,client.right-36));
 POINT origin{(std::max)(0L,client.right-width-18),(std::max)(0L,client.bottom-94)};ClientToScreen(game,&origin);
 SetWindowPos(window,HWND_TOPMOST,origin.x,origin.y,width,76,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 static ULONGLONG next=0;if(GetTickCount64()>=next){InvalidateRect(window,nullptr,FALSE);next=GetTickCount64()+100;}
}
inline void Close() noexcept {if(window)DestroyWindow(window);if(frameWindow)DestroyWindow(frameWindow);window=owner=frameWindow=nullptr;}
}
