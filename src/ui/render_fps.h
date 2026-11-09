#pragma once
#include "theme.h"
namespace native_lighting::render_fps {
inline auto& enabled=::control_visual::fpsEnabled;
inline HWND window=nullptr,owner=nullptr;
inline unsigned long long lastTick=0,lastFrames=0;
inline wchar_t label[64]=L"RENDER FPS: --";
inline LRESULT CALLBACK Proc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_NCHITTEST)return HTTRANSPARENT;
 if(m==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(m==WM_PAINT){PAINTSTRUCT p{};HDC dc=BeginPaint(h,&p);RECT r{};GetClientRect(h,&r);FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(246,246,246));HFONT font=control_ui::theme::CreateFontForStyle(control_ui::theme::kValueFont);auto old=SelectObject(dc,font);DrawTextW(dc,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,old);DeleteObject(font);EndPaint(h,&p);return 0;}
 return DefWindowProcW(h,m,w,l);
}
inline void Tick(HWND game,unsigned long long now) noexcept {
 if(window&&owner!=game){DestroyWindow(window);window=nullptr;}owner=game;
 if(!enabled||!game||GetForegroundWindow()!=game){if(window)ShowWindow(window,SW_HIDE);lastTick=0;return;}
 if(lastTick&&now-lastTick<500)return;
 if(!window){WNDCLASSW c{};c.hInstance=selfModule;c.lpfnWndProc=Proc;c.lpszClassName=L"ControlTOHRenderFPS";RegisterClassW(&c);window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,c.lpszClassName,L"Control render FPS",WS_POPUP,0,0,210,34,game,nullptr,selfModule,nullptr);}
 const auto frames=presentCount.load(std::memory_order_relaxed);
 if(!lastTick){lastTick=now;lastFrames=frames;}
 else if(now-lastTick>=500){const double fps=1000.*double(frames-lastFrames)/double(now-lastTick);swprintf_s(label,L"RENDER FPS: %.0f",fps);lastTick=now;lastFrames=frames;if(window)InvalidateRect(window,nullptr,FALSE);}
 if(window){POINT p{18,18};ClientToScreen(game,&p);SetWindowPos(window,nullptr,p.x,p.y,210,34,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);}
}
}
