#define NOMINMAX
#define CONTROL_GI_AUTO_TARGETS
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <cassert>
#include "../src/gi_auto_dispatch_bridge.h"
inline HMODULE selfModule=GetModuleHandleW(nullptr);
inline void Log(const char*,...){}
namespace control_ui::theme {inline constexpr COLORREF kBackground=RGB(20,20,20),kText=RGB(230,230,230);}
namespace native_lighting::ui {
inline HFONT detailFont=static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
inline void DrawLabel(HDC dc,HFONT font,const wchar_t* text,RECT rect,COLORREF color){
 auto old=SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text,-1,&rect,DT_LEFT);SelectObject(dc,old);
}
#include "../src/ui/gi_auto_capture_box.h"
}
int main(){using namespace native_lighting::ui;
 SetEnvironmentVariableA("CONTROLTOH_GI_AUTO","1");
 HWND game=CreateWindowExW(0,L"STATIC",L"Capture UI fixture",WS_OVERLAPPEDWINDOW,0,0,1280,720,nullptr,nullptr,selfModule,nullptr);assert(game);
 gi_auto_capture::dimensions=2560ull|(1440ull<<32);gi_auto_box::Tick(game);
 assert(gi_auto_box::box&&gi_auto_box::label);
 const auto ex=GetWindowLongPtrW(gi_auto_box::box,GWL_EXSTYLE);
 assert((ex&WS_EX_TRANSPARENT)&&(ex&WS_EX_NOACTIVATE)&&(ex&WS_EX_LAYERED));
 assert(GetWindow(gi_auto_box::box,GW_OWNER)==game);
 assert(SendMessageW(gi_auto_box::box,WM_NCHITTEST,0,0)==HTTRANSPARENT);
 assert(SendMessageW(gi_auto_box::label,WM_MOUSEACTIVATE,0,0)==MA_NOACTIVATE);
 gi_auto_box::Close();assert(!gi_auto_box::box&&!gi_auto_box::label);DestroyWindow(game);
 puts("PASS actual capture-window creation, click-through/no-activation styles, owner and cleanup. Live game display not tested.");
}
