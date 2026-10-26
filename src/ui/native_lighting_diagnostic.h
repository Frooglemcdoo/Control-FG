#pragma once
#include "theme.h"
#include "lighting_capture_suite.h"
#include "lighting_performance_capture.h"

namespace native_lighting::ui {
inline constexpr wchar_t kClass[] = L"ControlFGLightingDiagnosticR54";
inline constexpr int kWidth = 490, kButtonFirst = 5400;
inline constexpr int kModeRows = (kModeCount + 2) / 3;
inline constexpr int kStatusY = 40 + kModeRows * 46 - 2;
inline constexpr int kActionsY = kStatusY + 28;
inline constexpr int kCaptureY = kActionsY + 46;
inline constexpr int kPerformanceY = kCaptureY + 46;
inline constexpr int kCaptureStatusY = kPerformanceY + 46;
inline constexpr int kFooterY = kCaptureStatusY + 50;
inline constexpr int kHeight = kFooterY + 72;
inline constexpr unsigned kButtonCount = kModeCount + 8;
inline const wchar_t* ButtonLabel(unsigned index) noexcept {
    if (index < kModeCount) return kModeChoices[index].label;
    constexpr const wchar_t* actions[] = {L"ISSUE REMAINS", L"ISSUE GONE", L"SAVE LOG", L"CAPTURE LIGHT", L"PACKAGE LIGHT", L"TRACE SHADOW", L"PERF CAPTURE"};
    if(index==kModeCount+6 && performance_capture::Active())return L"CANCEL PERF";
    if(index==kModeCount+7) {
        static wchar_t label[32]{};
        const unsigned samples=specularSampleChoice.load();
        if(samples)swprintf_s(label,L"SPEC: %u",samples);
        else wcscpy_s(label,L"SPEC: NATIVE");
        return label;
    }
    return actions[index - kModeCount];
}
inline const wchar_t* ModeHint(Mode mode) noexcept {
    if(mode==Mode::Native)return L"Native lighting. SPEC is shared with other modes; NATIVE removes its override.";
    if(mode==Mode::RoomNativeBlend)return L"Native PCF-only shadow fade applied to RT. Uses your saved RR setting.";
    if(mode==Mode::RoomMapProbe)return L"Diagnostic colors. Click TRACE SHADOW; hold still. Restore DEFAULT afterward.";
    if(mode==Mode::RoomCorrectedMapPriority)return L"Test: rank RT lights using map-visible energy. RR off.";
    if(mode==Mode::RoomCorrectedOpen)return L"Test: selected RT spotlight unblocked; other shadows retained. RR off.";
    if(DisablesCorrectedBakedGi(mode))return L"Baked-brick samples removed from RT diffuse GI; shadows retained.";
    if(ShowsSpotLightIds(mode))return L"Colors=selected spotlight; magenta=no selected spotlight.";
    if(IsCorrectedSignal(mode))return L"Dark=shadow; white=lit; magenta=no selected spotlight.";
    return L"RR OFF comparisons preserve your saved RR preference.";
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
    // Keep Control active so opening this tool does not trigger its pause menu.
    ClipCursor(nullptr);
    if (!cursorVisibilityIncrements) {
        int count;
        do { count=ShowCursor(TRUE); ++cursorVisibilityIncrements; } while(count<0);
    }
    Trace("overlay_open activation=0 preserve_game_focus=1");
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
    wchar_t captureStatus[128]{};
    swprintf_s(captureStatus,L"Light capture: %s   Saved pairs: %u/2",control_rr_capture_ui::Label(control_rr_capture_ui::state.load()),control_rr_capture_ui::completedPairs.load());
    DrawLabel(dc,detailFont,captureStatus,{14,kCaptureStatusY,kWidth-12,kCaptureStatusY+22},theme::kText);
    DrawLabel(dc,detailFont,performance_capture::Active()?performance_capture::Status():control_rr_capture_ui::detail.load(),{14,kCaptureStatusY+22,kWidth-12,kCaptureStatusY+46},theme::kMutedText);
    wchar_t shortcuts[128]{};
    swprintf_s(shortcuts, L"F9: suite   Shift+F9: light capture   F10: close   Ctrl+Shift+1..%u", kModeCount);
    DrawLabel(dc, detailFont, shortcuts, {14,kFooterY,kWidth-12,kFooterY+24}, theme::kMutedText);
    DrawLabel(dc, detailFont, selectionRejected ? L"Mode unavailable. Save Log exports shader evidence for diagnosis." : savedNotice.load() == 2 ? L"Log write failed. Save Log shows the Windows error." : savedNotice.load() == 1 ? L"Log saved. See the folder shown in the confirmation." : ModeHint(mode), {14,kFooterY+26,kWidth-12,kFooterY+56}, theme::kText);
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
#if defined(CONTROL_CAPTURE_DIAGNOSTIC)
    // Export immutable CPU shader copies here only. Creation/binding callbacks
    // never write files or wait on this report lock.
    AcquireSRWLockExclusive(&reportLock);
    const bool snapshotSaved=reportHealthy.load() && shaderSnapshot.Export(
        [](const char* bytes,size_t length){return report.Write(bytes,static_cast<DWORD>(length));});
    reportHealthy.store(snapshotSaved && report.Flush());
    ReleaseSRWLockExclusive(&reportLock);
#endif
    wchar_t message[1024]{};
    AcquireSRWLockShared(&reportLock);
    const bool saved = reportHealthy.load();
    if (saved) swprintf_s(message, L"Log saved and flushed to disk:\n\n%s\n\nSend this file. Earlier session logs are kept.", report.Path());
    else swprintf_s(message, L"The log could not be saved. Windows error: %lu\n\nAttempted path: %s\n\nPlease send a screenshot of this message.", report.Error(), *report.Path() ? report.Path() : L"LocalAppData and temporary diagnostics folders");
    ReleaseSRWLockShared(&reportLock);
    savedNotice = saved ? 1u : 2u;
    MessageBoxW(window, message, saved ? L"Lighting log saved" : L"Lighting log failed", MB_OK | (saved ? MB_ICONINFORMATION : MB_ICONERROR));
}
inline void PackageRR(HWND window) noexcept {
    wchar_t module[MAX_PATH]{};
    if(!GetModuleFileNameW(selfModule,module,MAX_PATH)){MessageBoxW(window,L"Could not locate the capture helper.",L"Lighting packaging",MB_OK|MB_ICONERROR);return;}
    auto* slash=wcsrchr(module,L'\\');if(!slash)return;*slash=0;
    wchar_t command[2048]{};
    swprintf_s(command,L"powershell.exe -NoExit -NoProfile -ExecutionPolicy Bypass -File \"%s\\Capture-RR-Boundary.ps1\" -CollectExisting -GameProcessId %lu",module,GetCurrentProcessId());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,module,&startup,&process)){
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
    } else MessageBoxW(window,L"Could not start packaging. Check Capture-RR-Boundary.ps1 is installed beside dxgi.dll.",L"Lighting packaging",MB_OK|MB_ICONERROR);
}
inline void RequestLightCapture(HWND panel) noexcept {
    const auto mode=LiveSelection::GetMode(selection.Read());
    if(!performance_capture::Active() && !capture_suite::active && SupportsFocusedLightingCapture(mode) && control_rr_capture_ui::Request()) {
        Trace("native_lighting_capture requested=1 mode=%s",ModeName(mode));
        ClosePanel(panel);
    } else MessageBoxW(panel,L"Select RR OFF, RT SPOT OPEN, or NATIVE RR OFF before capturing. If busy, failed, or both pairs are saved, read the status; PACKAGE LIGHT collects diagnostics.",L"Lighting capture",MB_OK);
}
inline void RequestShadowTrace(HWND panel) noexcept {
    if(performance_capture::Active())return;
    using State=control_rr_capture_ui::State;
    const auto light=control_rr_capture_ui::state.load();
    if(control_rr_capture_ui::requested.load() ||
       (light!=State::Idle && light!=State::Complete)) {
        MessageBoxW(panel,L"Finish the light capture first. If it failed, start a fresh game session before tracing.",L"Shadow trace",MB_OK);return;
    }
    if(capture_suite::Start(capture_suite::Kind::Blocker))ClosePanel(panel);
    else MessageBoxW(panel,L"Shadow trace unavailable. Return to gameplay until corrected shadows are ready. If a previous trace failed, restart the game; otherwise Save Log for diagnosis.",L"Shadow trace",MB_OK);
}
inline void RequestPerformanceCapture(HWND panel) noexcept {
    if(performance_capture::Active()){
        if(performance_capture::phase!=performance_capture::Phase::Package)performance_capture::Save(GetWindow(panel,GW_OWNER),"cancelled");
        return;
    }
    using State=control_rr_capture_ui::State;
    const auto light=control_rr_capture_ui::state.load();
    if(control_rr_capture_ui::requested.load() || (light!=State::Idle && light!=State::Complete)){
        MessageBoxW(panel,L"Finish the light capture before starting a performance capture.",L"Performance capture",MB_OK);return;
    }
    if(performance_capture::Start())ClosePanel(panel);
    else MessageBoxW(panel,L"Performance capture unavailable. Return to gameplay first and finish any active capture. If timing resources failed or their bounded capacity is exhausted, restart the game.",L"Performance capture",MB_OK);
}
inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    namespace theme = control_ui::theme;
    switch (message) {
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
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
            const int row=i<kModeCount ? i/3 : kModeRows, col=i<kModeCount ? i%3 : (i-kModeCount)%3;
            HWND button=CreateWindowExW(0,L"STATIC",ButtonLabel(i),WS_CHILD|WS_VISIBLE|SS_NOTIFY|SS_OWNERDRAW,
                14+col*156,i<kModeCount ? 40+row*46 : i<kModeCount+3 ? kActionsY : i<kModeCount+6 ? kCaptureY : kPerformanceY,148,36,window,reinterpret_cast<HMENU>(INT_PTR(kButtonFirst+i)),selfModule,nullptr);
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
            if(performance_capture::Active() && index!=kModeCount+6)return 0;
            if(index<kModeCount){selectionRejected=!SelectMode(kModeChoices[index].mode);savedNotice=0;}
            else if(index==kModeCount||index==kModeCount+1) {
                if (!ReportVisual(index==kModeCount))
                    MessageBoxW(window, L"This mode has not applied yet. Press F10 to return to the game, wait for the image to settle, then reopen the panel and record your result.", L"Wait for mode to apply", MB_OK);
            }
            else if(index==kModeCount+2) SaveReport(window);
            else if(index==kModeCount+3) RequestLightCapture(window);
            else if(index==kModeCount+4) PackageRR(window);
            else if(index==kModeCount+5) RequestShadowTrace(window);
            else if(index==kModeCount+6) RequestPerformanceCapture(window);
            else if(index==kModeCount+7 && !capture_suite::active) {
                const unsigned next=NextSpecularSampleChoice(specularSampleChoice.load());
                if(next && !specularSamplesReady.load()) {
                    MessageBoxW(window,L"Specular sampling is not ready. Return to gameplay and try again.",L"Specular quality",MB_OK);
                } else {
                    SetSpecularSampleChoice(next);
                    Trace("specular_quality user_request=%u independent_of_lighting=1",next);
                }
            }
            RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
            return 0;
        }
        break;
    case WM_CLOSE: ClosePanel(window); return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
inline DWORD WINAPI ThreadProc(LPVOID) noexcept {
    const HRESULT comResult=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
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
    Trace("session_start build=R167-overlay-rr-capture pid=%lu default=authored_clip_surface_normals_current_spot_when_ready log=user-diagnostics-folder",GetCurrentProcessId());
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
        if (!panel || !IsDialogMessageW(panel, &msg)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
        const ULONGLONG now=GetTickCount64();
        if(now>=nextRRReport){ReportRRPaths("heartbeat");nextRRReport=now+5000;}
        if(now>=nextLogDrain){DrainDiagnosticLog();nextLogDrain=now+((capture_suite::active||performance_capture::Active())?100:500);}
        if(cachedGame&&!IsWindow(cachedGame))cachedGame=nullptr;
        if(!cachedGame&&now>=nextWindowSearch){cachedGame=FindFGGameWindow();nextWindowSearch=now+1000;}
        HWND game=cachedGame;
        if(panel&&owner!=game){ClosePanel(panel);DestroyWindow(panel);panel=nullptr;}
        if(game&&!panel){panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,kClass,L"Control lighting diagnostic",WS_POPUP,
            0,0,kWidth,kHeight,game,nullptr,selfModule,nullptr);owner=game;}
        if(!panel){if(performance_capture::Active())performance_capture::Tick(nullptr,now);if(capture_suite::active)capture_suite::Finish(nullptr,"game_window_lost");continue;}
        if(LiveSelection::Sequence(selection.Read())==0 && ModeAvailable(kDefaultMode,readyMask.load()))SelectMode(kDefaultMode);
        static bool f9Held=false;
        const bool f9Down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
        const bool capturePress=f9Down&&!f9Held && !(GetAsyncKeyState(VK_CONTROL)&0x8000) && !(GetAsyncKeyState(VK_MENU)&0x8000);
        f9Held=f9Down;
        if(!performance_capture::Active() && capturePress && (GetForegroundWindow()==game || PanelHasForeground(panel))) {
            if(GetAsyncKeyState(VK_SHIFT)&0x8000) RequestLightCapture(panel);
            else if(capture_suite::active)capture_suite::Finish(game,"cancelled");
            else {if(panelOpen)ClosePanel(panel);
                if(!capture_suite::Start())MessageBoxW(game,L"Could not start capture. Return to gameplay and wait for Default and GPU capture readiness. If a previous capture was cancelled, let its export finish. Use Save Log for details.",L"F9 capture",MB_OK|MB_ICONERROR);}
        }
        capture_suite::Tick(game,now);
        performance_capture::Tick(game,now);
        const bool foreground=GetForegroundWindow()==game || PanelHasForeground(panel);
        const bool modifier=(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_SHIFT)&0x8000);
        static_assert(kModeCount <= 9, "Numeric shortcuts support nine choices");
        for(int i=0;i<int(kModeCount+1);++i){const bool down=(GetAsyncKeyState(i == 0 ? VK_F10 : '0' + i)&0x8000)!=0;
            if(!capture_suite::active&&foreground&&down&&!held[i]&&(i==0||modifier)&&(!performance_capture::Active()||i==0)){
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
            if (GetForegroundWindow()==game || PanelHasForeground(panel)) ClipCursor(nullptr);
            if(GetTickCount64()>=nextRefresh){RedrawWindow(panel,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);nextRefresh=GetTickCount64()+250;}
        }else {RestoreCursorVisibility();if(IsWindowVisible(panel))ShowWindow(panel,SW_HIDE);if(!foreground)panelOpen=false;}
    }
    DrainDiagnosticLog(true);
    KillTimer(nullptr,timer);RestoreCursorVisibility();if(panel)DestroyWindow(panel);
    DeleteObject(headingFont);DeleteObject(bodyFont);DeleteObject(detailFont);if(SUCCEEDED(comResult))CoUninitialize();return 0;
}
} // namespace native_lighting::ui
namespace native_lighting {
inline void StartOverlay() noexcept {
    if(ui::started.exchange(true))return;
    HANDLE thread=CreateThread(nullptr,0,ui::ThreadProc,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);else {ui::started=false;Trace("overlay_failed reason=thread");}
}
}

