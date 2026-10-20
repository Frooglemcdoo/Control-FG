#pragma once
#include "theme.h"

namespace native_lighting::ui {
inline constexpr wchar_t kClass[] = L"ControlFGLightingDiagnosticR54";
inline constexpr int kWidth = 490, kHeight = 222, kButtonFirst = 5400;
inline constexpr const wchar_t* kButtons[] = {L"NATIVE", L"GI OFF", L"DIRECT OFF", L"SPECKLES REMAIN", L"SPECKLES GONE", L"SAVE LOG"};
inline HFONT headingFont = nullptr, bodyFont = nullptr, detailFont = nullptr;
inline std::atomic<bool> started{false};
inline std::atomic<unsigned> savedNotice{0};

inline void DrawLabel(HDC dc, HFONT font, const wchar_t* text, RECT bounds, COLORREF color) noexcept {
    const auto previous = SelectObject(dc, font);
    SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text, -1, &bounds, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, previous);
}
inline void Paint(HWND window) noexcept {
    namespace theme = control_ui::theme;
    PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps);
    RECT bounds{}; GetClientRect(window, &bounds);
    HBRUSH background = CreateSolidBrush(theme::kBackground); FillRect(dc, &bounds, background); DeleteObject(background);
    DrawLabel(dc, headingFont, L"LIGHTING DIAGNOSTIC", {14,8,kWidth-12,34}, theme::kAccent);
    const auto state = selection.Read();
    const unsigned ready = readyMask.load(), observed = LiveSelection::Observed(state);
    wchar_t status[160]{};
    swprintf_s(status, L"GI: %s   Direct: %s   %s", ready & kGiBit ? L"ready" : L"waiting", ready & kDirectBit ? L"ready" : L"waiting",
        observed == kAllBits ? L"bindings confirmed" : L"waiting for bindings");
    DrawLabel(dc, detailFont, status, {14,82,kWidth-12,108}, theme::kMutedText);
    DrawLabel(dc, detailFont, L"F10: panel    Ctrl+Shift+1 / 2 / 3: live mode", {14,155,kWidth-12,179}, theme::kMutedText);
    DrawLabel(dc, detailFont, savedNotice.load() == 2 ? L"Log write failed: check game-folder write access." : savedNotice.load() == 1 ? L"Saved: ControlFG-Lighting-R54.log" : L"Choose a mode, let it settle, then record what you see.", {14,181,kWidth-12,208}, theme::kText);
    EndPaint(window, &ps);
}
inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    namespace theme = control_ui::theme;
    switch (message) {
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR: SetCursor(LoadCursorW(nullptr, IDC_ARROW)); return TRUE;
    case WM_PAINT: Paint(window); return 0;
    case WM_CREATE:
        for (int i=0;i<6;++i) {
            const int row=i/3, col=i%3;
            HWND button=CreateWindowExW(WS_EX_NOACTIVATE,L"BUTTON",kButtons[i],WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                14+col*156,40+row*72,148,36,window,reinterpret_cast<HMENU>(INT_PTR(kButtonFirst+i)),selfModule,nullptr);
            SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(bodyFont),TRUE);
        }
        return 0;
    case WM_DRAWITEM: {
        const auto* item=reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        const unsigned index=item->CtlID-kButtonFirst;
        if(index>=6)return FALSE;
        const bool active=index<3 && unsigned(LiveSelection::GetMode(selection.Read()))==index;
        HBRUSH brush=CreateSolidBrush(active?theme::kAccent:RGB(35,35,35)); FillRect(item->hDC,&item->rcItem,brush); DeleteObject(brush);
        HBRUSH border=CreateSolidBrush(theme::kBorder); FrameRect(item->hDC,&item->rcItem,border); DeleteObject(border);
        const auto previous=SelectObject(item->hDC,bodyFont); SetBkMode(item->hDC,TRANSPARENT); SetTextColor(item->hDC,theme::kText);
        RECT rect=item->rcItem; DrawTextW(item->hDC,kButtons[index],-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE); SelectObject(item->hDC,previous);
        return TRUE;
    }
    case WM_COMMAND:
        if(HIWORD(wParam)==BN_CLICKED) {
            const unsigned index=LOWORD(wParam)-kButtonFirst;
            if(index<3){SelectMode(static_cast<Mode>(index));savedNotice=0;}
            else if(index==3||index==4)ReportVisual(index==3);
            else if(index==5){const auto state=selection.Read();Trace("save_log seq=%llu mode=%s recorded_mask=%u",LiveSelection::Sequence(state),ModeName(LiveSelection::GetMode(state)),LiveSelection::Observed(state));savedNotice=reportHealthy.load()?1u:2u;}
            RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
            return 0;
        }
        break;
    case WM_CLOSE: ShowWindow(window,SW_HIDE); return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
inline DWORD WINAPI ThreadProc(LPVOID) noexcept {
    namespace theme=control_ui::theme;
    headingFont=theme::CreateFontForStyle(theme::kHeadingFont);
    bodyFont=theme::CreateFontForStyle(theme::kHeadingFont);
    detailFont=theme::CreateFontForStyle(theme::kDetailFont);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=WindowProc;wc.lpszClassName=kClass;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){Trace("overlay_failed reason=class");return 0;}
    UINT_PTR timer=SetTimer(nullptr,0,50,nullptr);
    if(!timer){Trace("overlay_failed reason=timer");return 0;}
    HWND panel=nullptr,owner=nullptr;bool open=true,held[4]{};
    ULONGLONG nextRefresh=0;
    Trace("session_start build=R54 pid=%lu default=native log=ControlFG-Lighting-R54.log",GetCurrentProcessId());
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
        TranslateMessage(&msg);DispatchMessageW(&msg);
        if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
        HWND game=FindFGGameWindow();
        if(panel&&owner!=game){DestroyWindow(panel);panel=nullptr;}
        if(game&&!panel){panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,kClass,L"Control lighting diagnostic",WS_POPUP,
            0,0,kWidth,kHeight,game,nullptr,selfModule,nullptr);owner=game;}
        if(!panel)continue;
        const bool foreground=GetForegroundWindow()==game;
        const bool modifier=(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_SHIFT)&0x8000);
        const int keys[4]={VK_F10,'1','2','3'};
        for(int i=0;i<4;++i){const bool down=(GetAsyncKeyState(keys[i])&0x8000)!=0;
            if(foreground&&down&&!held[i]&&(i==0||modifier)){
                if(i==0)open=!open;else {SelectMode(static_cast<Mode>(i-1));savedNotice=0;}
            }held[i]=down;
        }
        if(foreground&&open){
            RECT client{};GetClientRect(game,&client);POINT origin{client.right-kWidth-18,18};if(origin.x<0)origin.x=0;
            ClientToScreen(game,&origin);SetWindowPos(panel,HWND_TOP,origin.x,origin.y,kWidth,kHeight,SWP_NOACTIVATE|SWP_SHOWWINDOW);
            // No focus switch, game window subclass, global input hook or topmost window.
            if(GetTickCount64()>=nextRefresh){RedrawWindow(panel,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);nextRefresh=GetTickCount64()+250;}
        }else ShowWindow(panel,SW_HIDE);
    }
    KillTimer(nullptr,timer);if(panel)DestroyWindow(panel);
    DeleteObject(headingFont);DeleteObject(bodyFont);DeleteObject(detailFont);return 0;
}
} // namespace native_lighting::ui
namespace native_lighting {
inline void StartOverlay() noexcept {
    if(ui::started.exchange(true))return;
    HANDLE thread=CreateThread(nullptr,0,ui::ThreadProc,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);else {ui::started=false;Trace("overlay_failed reason=thread");}
}
}
