#pragma once
#include "theme.h"

namespace native_lighting::ui {
inline constexpr wchar_t kClass[] = L"ControlFGLightingDiagnosticR54";
inline constexpr int kWidth = 490, kButtonFirst = 5400;
inline constexpr int kModeRows = (kModeCount + 2) / 3;
inline constexpr int kStatusY = 40 + kModeRows * 46 - 2;
inline constexpr int kActionsY = kStatusY + 28;
inline constexpr int kFooterY = kActionsY + 52;
inline constexpr int kHeight = kFooterY + 72;
inline constexpr unsigned kButtonCount = kModeCount + 3;
inline const wchar_t* ButtonLabel(unsigned index) noexcept {
    if (index < kModeCount) return kModeChoices[index].label;
    constexpr const wchar_t* actions[] = {L"ISSUE REMAINS", L"ISSUE GONE", L"SAVE LOG"};
    return actions[index - kModeCount];
}
inline HFONT headingFont = nullptr, bodyFont = nullptr, detailFont = nullptr;
inline std::atomic<bool> started{false};
inline std::atomic<unsigned> savedNotice{0};
inline bool selectionRejected = false;
inline bool panelOpen = false; // Owned exclusively by the diagnostic UI thread.
inline int cursorVisibilityIncrements = 0;

inline bool PanelHasForeground(HWND panel) noexcept {
    const HWND foreground = GetForegroundWindow();
    return foreground == panel || IsChild(panel, foreground);
}
inline void RestoreCursorVisibility() noexcept {
    while (cursorVisibilityIncrements > 0) {
        ShowCursor(FALSE);
        --cursorVisibilityIncrements;
    }
}
inline void ClosePanel(HWND panel) noexcept {
    const bool returnToGame = PanelHasForeground(panel);
    panelOpen = false;
    RestoreCursorVisibility();
    ShowWindow(panel, SW_HIDE);
    if (returnToGame) SetForegroundWindow(GetWindow(panel, GW_OWNER));
    Trace("overlay_closed return_to_game=%u", returnToGame ? 1u : 0u);
}
inline void OpenPanel(HWND panel) noexcept {
    panelOpen = true;
    ShowWindow(panel, SW_SHOWNOACTIVATE);
    // Only called for an explicit F10 press while the game has foreground.
    const BOOL activated = SetForegroundWindow(panel);
    if (activated) SetFocus(GetDlgItem(panel, kButtonFirst));
    Trace("overlay_open activation=%u", activated ? 1u : 0u);
}

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
    const auto mode = LiveSelection::GetMode(state);
    const unsigned required = RequiredBinding(mode);
    wchar_t status[160]{};
    swprintf_s(status, L"Selected mode: %s   %s", ModeAvailable(mode,ready) ? L"ready" : L"waiting",
        (observed & required) == required && (mode != Mode::SpotResolution2x || spot_resolution_runtime::Observed(state)) ? (IsRtSpotMode(mode) ? L"RT pair recorded" : L"native bindings recorded") : L"Waiting: return to game");
    DrawLabel(dc, detailFont, status, {14,kStatusY,kWidth-12,kStatusY+24}, theme::kMutedText);
    wchar_t shortcuts[128]{};
    swprintf_s(shortcuts, L"F10: return to game    Ctrl+Shift+1..%u: live mode", kModeCount);
    DrawLabel(dc, detailFont, shortcuts, {14,kFooterY,kWidth-12,kFooterY+24}, theme::kMutedText);
    DrawLabel(dc, detailFont, selectionRejected ? L"Mode unavailable: shader or sample hook is not ready. Save Log." : savedNotice.load() == 2 ? L"Log write failed. Save Log shows the Windows error." : savedNotice.load() == 1 ? L"Log saved. See the folder shown in the confirmation." : L"XYZ: blue=negative; orange=near center plane; red=positive. RR bypassed.", {14,kFooterY+26,kWidth-12,kFooterY+56}, theme::kText);
    EndPaint(window, &ps);
}
inline void SaveReport(HWND window) noexcept {
    ReportRRPaths("save_log");
    rt_shadow_capture::Summary();
    const auto state = selection.Read();
    Trace("save_log seq=%llu mode=%s recorded_mask=%u", LiveSelection::Sequence(state),
        ModeName(LiveSelection::GetMode(state)), LiveSelection::Observed(state));
    Trace("coverage graphics_pso=%llu compute_pso=%llu bindings=%llu same_size_mismatches=%u ready_mask=%u", graphicsSeen.load(), computeSeen.load(), bindingsSeen.load(), candidateMismatches.load(), readyMask.load());
    Trace("library_coverage graphics_loads=%llu compute_loads=%llu identities=%u", libraryGraphicsSeen.load(), libraryComputeSeen.load(), identityReports.load());
    DrainDiagnosticLog(true);
    wchar_t message[1024]{};
    AcquireSRWLockShared(&reportLock);
    const bool saved = reportHealthy.load();
    if (saved) swprintf_s(message, L"Log saved and flushed to disk:\n\n%s\n\nSend this file. Earlier session logs are kept.", report.Path());
    else swprintf_s(message, L"The log could not be saved. Windows error: %lu\n\nAttempted path: %s\n\nPlease send a screenshot of this message.", report.Error(), *report.Path() ? report.Path() : L"LocalAppData and temporary diagnostics folders");
    ReleaseSRWLockShared(&reportLock);
    savedNotice = saved ? 1u : 2u;
    MessageBoxW(window, message, saved ? L"Lighting log saved" : L"Lighting log failed", MB_OK | (saved ? MB_ICONINFORMATION : MB_ICONERROR));
}
inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    namespace theme = control_ui::theme;
    switch (message) {
    case WM_MOUSEACTIVATE: return MA_ACTIVATE;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) RestoreCursorVisibility();
        else {
            ClipCursor(nullptr);
            // Balance every ShowCursor call when focus leaves this panel.
            if (!cursorVisibilityIncrements) {
                int count;
                do { count = ShowCursor(TRUE); ++cursorVisibilityIncrements; } while (count < 0);
            }
        }
        break;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR: SetCursor(LoadCursorW(nullptr, IDC_ARROW)); return TRUE;
    case WM_PAINT: Paint(window); return 0;
    case WM_CREATE:
        for (unsigned i=0;i<kButtonCount;++i) {
            const int row=i<kModeCount ? i/3 : kModeRows, col=i<kModeCount ? i%3 : i-kModeCount;
            HWND button=CreateWindowExW(0,L"BUTTON",ButtonLabel(i),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
                14+col*156,i<kModeCount ? 40+row*46 : kActionsY,148,36,window,reinterpret_cast<HMENU>(INT_PTR(kButtonFirst+i)),selfModule,nullptr);
            SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(bodyFont),TRUE);
        }
        return 0;
    case WM_DRAWITEM: {
        const auto* item=reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        const unsigned index=item->CtlID-kButtonFirst;
        if(index>=kButtonCount)return FALSE;
        const bool active=index<kModeCount && LiveSelection::GetMode(selection.Read())==kModeChoices[index].mode;
        HBRUSH brush=CreateSolidBrush(active?theme::kAccent:RGB(35,35,35)); FillRect(item->hDC,&item->rcItem,brush); DeleteObject(brush);
        HBRUSH border=CreateSolidBrush(theme::kBorder); FrameRect(item->hDC,&item->rcItem,border); DeleteObject(border);
        const auto previous=SelectObject(item->hDC,bodyFont); SetBkMode(item->hDC,TRANSPARENT); SetTextColor(item->hDC,theme::kText);
        RECT rect=item->rcItem; DrawTextW(item->hDC,ButtonLabel(index),-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE); SelectObject(item->hDC,previous);
        return TRUE;
    }
    case WM_COMMAND:
        if(HIWORD(wParam)==BN_CLICKED) {
            const unsigned index=LOWORD(wParam)-kButtonFirst;
            if(index<kModeCount){selectionRejected=!SelectMode(kModeChoices[index].mode);savedNotice=0;}
            else if(index==kModeCount||index==kModeCount+1) {
                if (!ReportVisual(index==kModeCount))
                    MessageBoxW(window, L"This mode has not applied yet. Press F10 to return to the game, wait for the image to settle, then reopen the panel and record your result.", L"Wait for mode to apply", MB_OK);
            }
            else if(index==kModeCount+2) SaveReport(window);
            RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
            return 0;
        }
        break;
    case WM_CLOSE: ClosePanel(window); return 0;
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
    HWND panel=nullptr,owner=nullptr;bool held[kModeCount+1]{};
    ULONGLONG nextRefresh=0, nextWindowSearch=0, nextLogDrain=0, nextRRReport=0;
    HWND cachedGame=nullptr;
    POINT lastOrigin{-1,-1};
    Trace("session_start build=R128-native-gpu-hit-capture pid=%lu default=native log=user-diagnostics-folder",GetCurrentProcessId());
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
        if (!panel || !IsDialogMessageW(panel, &msg)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
        const ULONGLONG now=GetTickCount64();
        if(now>=nextRRReport){ReportRRPaths("heartbeat");nextRRReport=now+5000;}
        if(now>=nextLogDrain){DrainDiagnosticLog();nextLogDrain=now+500;}
        if(cachedGame&&!IsWindow(cachedGame))cachedGame=nullptr;
        if(!cachedGame&&now>=nextWindowSearch){cachedGame=FindFGGameWindow();nextWindowSearch=now+1000;}
        HWND game=cachedGame;
        if(panel&&owner!=game){ClosePanel(panel);DestroyWindow(panel);panel=nullptr;}
        if(game&&!panel){panel=CreateWindowExW(WS_EX_TOOLWINDOW,kClass,L"Control lighting diagnostic",WS_POPUP,
            0,0,kWidth,kHeight,game,nullptr,selfModule,nullptr);owner=game;}
        if(!panel)continue;
        const bool foreground=GetForegroundWindow()==game || PanelHasForeground(panel);
        const bool modifier=(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_SHIFT)&0x8000);
        static_assert(kModeCount <= 9, "Numeric shortcuts support nine choices");
        for(int i=0;i<int(kModeCount+1);++i){const bool down=(GetAsyncKeyState(i == 0 ? VK_F10 : '0' + i)&0x8000)!=0;
            if(foreground&&down&&!held[i]&&(i==0||modifier)){
                if(i==0){if(panelOpen)ClosePanel(panel);else OpenPanel(panel);}else {selectionRejected=!SelectMode(kModeChoices[i-1].mode);savedNotice=0;}
            }held[i]=down;
        }
        if(foreground&&panelOpen){
            RECT client{};GetClientRect(game,&client);POINT origin{client.right-kWidth-18,18};if(origin.x<0)origin.x=0;
            ClientToScreen(game,&origin);
            if(!IsWindowVisible(panel)||origin.x!=lastOrigin.x||origin.y!=lastOrigin.y){
                SetWindowPos(panel,HWND_TOP,origin.x,origin.y,kWidth,kHeight,SWP_NOACTIVATE|SWP_SHOWWINDOW);
                lastOrigin=origin;
            }
            // Release confinement only while this panel owns input, never over other apps.
            if (PanelHasForeground(panel)) ClipCursor(nullptr);
            if(GetTickCount64()>=nextRefresh){RedrawWindow(panel,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);nextRefresh=GetTickCount64()+250;}
        }else {RestoreCursorVisibility();if(IsWindowVisible(panel))ShowWindow(panel,SW_HIDE);if(!foreground)panelOpen=false;}
    }
    DrainDiagnosticLog(true);
    KillTimer(nullptr,timer);RestoreCursorVisibility();if(panel)DestroyWindow(panel);
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
