#pragma once
#include <mutex>
#include "rr_user_control.h"
#include "pt_reflection_user_control.h"
#include "rr_gi_sample_control.h"

// Native-menu settings persistence, display refresh discovery and optional
// read-only VRAM monitor. No settings window, shortcut polling or input hooks.
static constexpr wchar_t kFGVramProbeClassName[] = L"ControlFGVramLifecycle_R1";
static constexpr int kFGVramProbeWidth = 390, kFGVramProbeHeight = 86;
static constexpr ULONGLONG kFGSettingsDebounceMs = 350;
static std::recursive_mutex fgSettingsMutex;
static std::atomic<bool> fgNativeMenuReady{false};
static std::atomic<bool> fgSettingsDirty{false};
static std::atomic<ULONGLONG> fgSettingsDirtyMs{0};
static std::wstring fgSettingsPath;
static std::atomic<unsigned int> fgSettingsLoaded{0};
static std::atomic<unsigned int> fgVramMonitorEnabled{1};
static HANDLE fgSettingsThreadHandle = nullptr;
static ULONGLONG fgSettingsLastRefreshPollMs = 0;
static HWND fgSettingsLastRefreshGame = nullptr;
static HWND fgSettingsCachedGame = nullptr;
static ULONGLONG fgSettingsLastGameWindowSearchMs = 0;
static constexpr ULONGLONG kFGSettingsGameWindowSearchIntervalMs = 1000;

static bool IsFGVramMonitorEnabled() noexcept {
    return fgVramMonitorEnabled.load(std::memory_order_acquire)!=0;
}
static void SetFGVramMonitorEnabled(bool enabled) noexcept {
    fgVramMonitorEnabled.store(enabled?1u:0u,std::memory_order_release);
}

static std::atomic<unsigned int> fgTextureStreamingBudgetMs{0};

static unsigned int NormalizeFGTextureStreamingBudgetMs(unsigned int value) noexcept {
    return value==4u||value==6u||value==8u ? value : 0u;
}
static unsigned int GetFGTextureStreamingBudgetMs() noexcept {
    return NormalizeFGTextureStreamingBudgetMs(fgTextureStreamingBudgetMs.load(std::memory_order_acquire));
}
static void SetFGTextureStreamingBudgetMs(unsigned int value) noexcept {
    fgTextureStreamingBudgetMs.store(NormalizeFGTextureStreamingBudgetMs(value),std::memory_order_release);
}

struct FGWindowSearch {
    DWORD pid;
    HWND best;
    long long bestArea;
};

static BOOL CALLBACK FGSettingsEnumWindows(HWND hwnd, LPARAM param) noexcept {
    auto* search = reinterpret_cast<FGWindowSearch*>(param);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != search->pid || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER)) return TRUE;
    wchar_t className[64]{};
    if (GetClassNameW(hwnd, className, _countof(className)) &&
        wcscmp(className, kFGVramProbeClassName) == 0) return TRUE;
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) return TRUE;
    const long long area = static_cast<long long>(rc.right - rc.left) * static_cast<long long>(rc.bottom - rc.top);
    if (area > search->bestArea) { search->best = hwnd; search->bestArea = area; }
    return TRUE;
}

static HWND FindFGGameWindow() noexcept {
    FGWindowSearch search{GetCurrentProcessId(), nullptr, 0};
    EnumWindows(FGSettingsEnumWindows, reinterpret_cast<LPARAM>(&search));
    return search.best;
}

static HWND FindFGGameWindowCached(bool force = false) noexcept {
    const ULONGLONG now = GetTickCount64();
    if (fgSettingsCachedGame && !IsWindow(fgSettingsCachedGame)) {
        fgSettingsCachedGame = nullptr;
        fgSettingsLastGameWindowSearchMs = 0;
    }
    if (!force && fgSettingsCachedGame) return fgSettingsCachedGame;
    if (!force && fgSettingsLastGameWindowSearchMs &&
        now - fgSettingsLastGameWindowSearchMs < kFGSettingsGameWindowSearchIntervalMs) {
        return nullptr;
    }
    fgSettingsLastGameWindowSearchMs = now;
    fgSettingsCachedGame = FindFGGameWindow();
    return fgSettingsCachedGame;
}

static bool FGSettingsGameIsForeground(HWND game) noexcept {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(foreground, &pid);
    return pid == GetCurrentProcessId() || foreground == game;
}

static bool FGSettingsResolveSettingsPath() noexcept {
    if (!fgSettingsPath.empty()) return true;
    wchar_t base[32768]{};
    const DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", base, _countof(base));
    if (!count || count >= _countof(base)) return false;
    std::wstring dir = std::wstring(base) + L"\\ControlFG";
    if (!CreateDirectoryW(dir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    fgSettingsPath = dir + L"\\settings-mfg-test.ini";
    const std::wstring originalSettings=dir+L"\\settings.ini";
    CopyFileW(originalSettings.c_str(),fgSettingsPath.c_str(),TRUE);
    return true;
}

static void FGSettingsLoadSettings() noexcept {
    unsigned int expected = 0;
    if (!fgSettingsLoaded.compare_exchange_strong(expected, 1u)) return;
    if (!FGSettingsResolveSettingsPath()) {
        SetFGDynamicManualTargetFps(0);
        control_rr::RRUserSetSharpnessPercent(0);
        control_rr::RRUserSetSharpnessOverrideEnabled(false);
        control_rr::RRUserSetPresetValue(control_rr::RRPresetF);
        control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
        control_rr::RRUserMarkPresetRestartRequired(false);
        control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
        control_rr::RRUserRequest(false);
        SetFGTextureStreamingBudgetMs(0u);
        SetFGVramMonitorEnabled(true);
        Log("FG_SETTINGS_LOAD success=0 reason=path_unavailable defaults=4x,dynamic_auto,rr_off,rr_model_F,texture_streaming_off,vram_monitor_on ui=fg_full_rr_toggle_model_E_F");
        return;
    }

    const unsigned int rawSelection = GetPrivateProfileIntW(L"FrameGeneration", L"Mode", kSLDefaultMultiplier, fgSettingsPath.c_str());
    const unsigned int selection = IsFGRtx40Series() ? 2u : NormalizeFGMultiplier(rawSelection);
    const unsigned int rawTarget = GetPrivateProfileIntW(L"FrameGeneration", L"DynamicTargetFPS", 0, fgSettingsPath.c_str());
    const unsigned int manualTarget = ClampFGDynamicManualTargetFps(rawTarget);
    control_rr_clamp::Select(control_rr_clamp::Normalize(GetPrivateProfileIntW(L"RayReconstruction", L"ReflectionClamp", 60, fgSettingsPath.c_str())));
    control_rr::GISampleSelect(GetPrivateProfileIntW(L"RayReconstruction", L"GISamples", 4, fgSettingsPath.c_str()));
    const bool rrEnabled = GetPrivateProfileIntW(L"RayReconstruction", L"Enabled", 0, fgSettingsPath.c_str()) != 0;
    const unsigned int rawRrPreset = GetPrivateProfileIntW(L"RayReconstruction", L"Preset", control_rr::RRPresetF, fgSettingsPath.c_str());
    const unsigned int rrPreset = rawRrPreset == control_rr::RRPresetE ? control_rr::RRPresetE : control_rr::RRPresetF;
    const unsigned int rawTextureStreamingMs = GetPrivateProfileIntW(L"Experimental", L"TextureStreamingBudgetMs", 0, fgSettingsPath.c_str());
    const unsigned int textureStreamingMs = NormalizeFGTextureStreamingBudgetMs(rawTextureStreamingMs);
    const bool vramMonitorEnabled = GetPrivateProfileIntW(L"Overlay", L"VramMonitorEnabled", 1, fgSettingsPath.c_str()) != 0;

    slFgUserMultiplier.store(selection, std::memory_order_release);
    slFgDynamicManualTargetFps.store(manualTarget, std::memory_order_release);
    control_rr::RRUserSetSharpnessPercent(0);
    control_rr::RRUserSetSharpnessOverrideEnabled(false);
    control_rr::RRUserSetPresetValue(rrPreset);
    control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
    control_rr::RRUserMarkPresetRestartRequired(false);
    control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
    control_rr::RRUserRequest(rrEnabled);
    SetFGTextureStreamingBudgetMs(textureStreamingMs);
    SetFGVramMonitorEnabled(vramMonitorEnabled);
    PTReflectionUserRequest(GetPrivateProfileIntW(L"GlassReflections", L"Enabled", 1, fgSettingsPath.c_str()) != 0);
    Log("GLASS_REFLECTION_SETTINGS enabled=%u source=settings default=on", unsigned(PTReflectionUserRequested()));
    Log("FG_SETTINGS_LOAD success=1 path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u rr_enabled=%u rr_preset=%s rr_preset_value=%u texture_streaming_ms=%u vram_monitor=%u skin_mode=off schema=10 ui=fg_full_rr_toggle_model_E_F",
        fgSettingsPath.c_str(), GetFGSelectionName(selection), selection, manualTarget ? "manual" : "auto", manualTarget, unsigned(rrEnabled), control_rr::RRUserPresetLabel(), control_rr::RRUserPresetValue(), textureStreamingMs, unsigned(vramMonitorEnabled));
}

static bool FGSettingsSaveSettingsNow() noexcept {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    if (!FGSettingsResolveSettingsPath()) {
        Log("FG_SETTINGS_SAVE success=0 reason=path_unavailable ui=fg_full_rr_toggle_only");
        return false;
    }

    wchar_t clampText[8]{};
    swprintf_s(clampText,L"%u",control_rr_clamp::Requested());
    wchar_t experimentalText[8]{};
    swprintf_s(experimentalText, L"%u", IsRTX40MFGRequested() ? 1u : 0u);
    wchar_t textureStreamingText[8]{};
    swprintf_s(textureStreamingText,L"%u",GetFGTextureStreamingBudgetMs());
    wchar_t vramMonitorText[8]{};
    swprintf_s(vramMonitorText,L"%u",IsFGVramMonitorEnabled()?1u:0u);
    wchar_t modeText[16]{};
    wchar_t targetText[16]{};
    wchar_t rrEnabledText[8]{};
    wchar_t ptEnabledText[8]{};
    swprintf_s(ptEnabledText, L"%u", PTReflectionUserRequested() ? 1u : 0u);
    wchar_t giSamplesText[8]{};
    swprintf_s(giSamplesText,L"%u",control_rr::GISampleRequested());
    wchar_t rrPresetText[8]{};
    swprintf_s(modeText, L"%u", GetFGUserMultiplier());
    swprintf_s(targetText, L"%u", GetFGDynamicManualTargetFps());
    swprintf_s(rrEnabledText, L"%u", control_rr::RRUserRequested() ? 1u : 0u);
    swprintf_s(rrPresetText, L"%u", control_rr::RRUserPresetValue());

    bool ok = WritePrivateProfileStringW(L"ControlFG", L"Schema", L"10", fgSettingsPath.c_str()) != FALSE;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"Mode", modeText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"DynamicTargetFPS", targetText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"Enabled", rrEnabledText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"Preset", rrPresetText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"ReflectionClamp", clampText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"GISamples", giSamplesText, fgSettingsPath.c_str()) != FALSE) && ok;
    WritePrivateProfileStringW(L"Overlay", L"ToggleKey", nullptr, fgSettingsPath.c_str());
    ok = (WritePrivateProfileStringW(L"Overlay", L"VramMonitorEnabled", vramMonitorText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"Experimental", L"RTX40MultiFG", experimentalText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"Experimental", L"TextureStreamingBudgetMs", textureStreamingText, fgSettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"GlassReflections", L"Enabled", ptEnabledText, fgSettingsPath.c_str()) != FALSE) && ok;
    // Old experimental RR/AA settings are intentionally removed; FG settings remain intact.
    WritePrivateProfileStringW(L"AntiAliasing", nullptr, nullptr, fgSettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinResponsivity", nullptr, fgSettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinPreserveNative", nullptr, fgSettingsPath.c_str());
    if (ok) WritePrivateProfileStringW(nullptr, nullptr, nullptr, fgSettingsPath.c_str());

    Log("FG_SETTINGS_SAVE success=%u path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u rr_enabled=%u rr_preset=%s rr_preset_value=%u texture_streaming_ms=%u vram_monitor=%u schema=10 ui=fg_full_rr_toggle_model_E_F",
        unsigned(ok), fgSettingsPath.c_str(), GetFGSelectionName(GetFGUserMultiplier()), GetFGUserMultiplier(),
        GetFGDynamicManualTargetFps() ? "manual" : "auto", GetFGDynamicManualTargetFps(), unsigned(control_rr::RRUserRequested()), control_rr::RRUserPresetLabel(), control_rr::RRUserPresetValue(), GetFGTextureStreamingBudgetMs(), unsigned(IsFGVramMonitorEnabled()));
    return ok;
}

static void FGSettingsMarkSettingsDirty() noexcept {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    fgSettingsDirty = true;
    fgSettingsDirtyMs = GetTickCount64();
}

static void FGSettingsFlushSettingsIfDue(bool force = false) noexcept {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    if (!fgSettingsDirty) return;
    const ULONGLONG now = GetTickCount64();
    if (!force && now - fgSettingsDirtyMs < kFGSettingsDebounceMs) return;
    if (FGSettingsSaveSettingsNow()) fgSettingsDirty = false;
}

static unsigned int FGSettingsDetectDisplayRefreshMilliHz(HWND game, const char** sourceOut) noexcept {
    if (sourceOut) *sourceOut = "none";
    if (!game) return 0;

    const HMONITOR monitor = MonitorFromWindow(game, MONITOR_DEFAULTTONEAREST);
    if (!monitor) return 0;

    MONITORINFOEXW monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, reinterpret_cast<MONITORINFO*>(&monitorInfo))) return 0;

    UINT32 pathCount = 0;
    UINT32 modeCount = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) == ERROR_SUCCESS && pathCount && modeCount) {
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
        std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) == ERROR_SUCCESS) {
            paths.resize(pathCount);
            for (const auto& path : paths) {
                DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName{};
                sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                sourceName.header.size = sizeof(sourceName);
                sourceName.header.adapterId = path.sourceInfo.adapterId;
                sourceName.header.id = path.sourceInfo.id;
                if (DisplayConfigGetDeviceInfo(&sourceName.header) != ERROR_SUCCESS) continue;
                if (_wcsicmp(sourceName.viewGdiDeviceName, monitorInfo.szDevice) != 0) continue;
                const DISPLAYCONFIG_RATIONAL rate = path.targetInfo.refreshRate;
                if (rate.Numerator && rate.Denominator) {
                    const double hz = static_cast<double>(rate.Numerator) / static_cast<double>(rate.Denominator);
                    if (hz >= static_cast<double>(kSLDynamicTargetMinFps) && hz <= static_cast<double>(kSLDynamicTargetMaxFps)) {
                        if (sourceOut) *sourceOut = "QueryDisplayConfig";
                        return static_cast<unsigned int>(hz * 1000.0 + 0.5);
                    }
                }
            }
        }
    }

    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (EnumDisplaySettingsExW(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0) &&
        mode.dmDisplayFrequency >= kSLDynamicTargetMinFps && mode.dmDisplayFrequency <= kSLDynamicTargetMaxFps) {
        if (sourceOut) *sourceOut = "EnumDisplaySettingsEx";
        return mode.dmDisplayFrequency * 1000u;
    }
    return 0;
}

static void FGSettingsRefreshDisplayTarget(HWND game, bool force = false) noexcept {
    if (!game) return;
    const ULONGLONG now = GetTickCount64();
    if (!force && game == fgSettingsLastRefreshGame && now - fgSettingsLastRefreshPollMs < 1000ull) return;
    const bool gameChanged = game != fgSettingsLastRefreshGame;
    fgSettingsLastRefreshGame = game;
    fgSettingsLastRefreshPollMs = now;

    const char* source = nullptr;
    const unsigned int milliHz = FGSettingsDetectDisplayRefreshMilliHz(game, &source);
    if (milliHz) SetFGDynamicDetectedRefreshMilliHz(milliHz, source);
    else if (gameChanged) SetFGDynamicDetectedRefreshMilliHz(0, "unresolved_new_window");
}

static HFONT CreateFGControlFont(int height, int weight, const wchar_t* face) noexcept {
    return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

static void PaintFGVramProbe(HWND hwnd) noexcept {
    PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);if(!dc)return;
    RECT client{};GetClientRect(hwnd,&client);
    HBRUSH bg=CreateSolidBrush(RGB(0,0,0));FillRect(dc,&client,bg);DeleteObject(bg);
    HPEN border=CreatePen(PS_SOLID,1,RGB(92,92,92));HGDIOBJ oldPen=SelectObject(dc,border);
    HGDIOBJ oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));Rectangle(dc,0,0,client.right-1,client.bottom-1);
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(border);SetBkMode(dc,TRANSPARENT);

    HFONT label=CreateFGControlFont(16,FW_BOLD,L"Bahnschrift Condensed");
    HFONT value=CreateFGControlFont(22,FW_SEMIBOLD,L"Bahnschrift SemiCondensed");
    HFONT smallFont=CreateFGControlFont(13,FW_NORMAL,L"Bahnschrift SemiCondensed");
    HGDIOBJ oldFont=SelectObject(dc,label);SetTextColor(dc,RGB(255,32,32));
    RECT labelRc{14,8,82,30};DrawTextW(dc,L"VRAM",-1,&labelRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    wchar_t mainText[160]{},detail[192]{};
    if(vramProbeSampleValid.load(std::memory_order_acquire)){
        constexpr double GiB=1024.0*1024.0*1024.0;
        const auto usage=vramProbeLocalUsageBytes.load(std::memory_order_acquire);
        const auto budget=vramProbeLocalBudgetBytes.load(std::memory_order_acquire);
        const auto peak=vramProbePeakLocalUsageBytes.load(std::memory_order_acquire);
        const auto physical=vramProbeDedicatedVideoBytes.load(std::memory_order_acquire);
        const unsigned pct=budget?static_cast<unsigned>((usage*100ull+budget/2ull)/budget):0u;
        swprintf_s(mainText,L"%.2f / %.2f GB  (%u%%)",double(usage)/GiB,double(budget)/GiB,pct);
        const bool hdrKnown=vramProbeHdrKnown.load(std::memory_order_acquire)!=0;
        const bool hdr=vramProbeHdrEnabled.load(std::memory_order_acquire)!=0;
        swprintf_s(detail,L"Peak %.2f GB   Physical %.2f GB   HDR %s",double(peak)/GiB,double(physical)/GiB,
            hdrKnown?(hdr?L"ON":L"OFF"):L"?");
    }else{
        wcscpy_s(mainText,L"Waiting for GPU sample...");
        wcscpy_s(detail,L"Updates while Control is rendering.");
    }
    SelectObject(dc,value);SetTextColor(dc,RGB(246,246,246));
    RECT valueRc{82,5,376,38};DrawTextW(dc,mainText,-1,&valueRc,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc,smallFont);SetTextColor(dc,RGB(185,185,185));
    RECT detailRc{14,48,376,72};DrawTextW(dc,detail,-1,&detailRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc,oldFont);DeleteObject(label);DeleteObject(value);DeleteObject(smallFont);EndPaint(hwnd,&ps);
}
static LRESULT CALLBACK FGVramProbeWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) noexcept {
    switch(message){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:PaintFGVramProbe(hwnd);return 0;
    case WM_NCHITTEST:return HTTRANSPARENT;
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}
// An owned, non-topmost monitor stays with the game without competing with
// NVIDIA/Steam overlays. It never takes focus, captures input or polls shortcuts.
static DWORD WINAPI FGSettingsThreadProc(LPVOID) noexcept {
    WNDCLASSEXW wc{};
    wc.cbSize=sizeof(wc); wc.lpfnWndProc=FGVramProbeWndProc;
    wc.hInstance=selfModule; wc.lpszClassName=kFGVramProbeClassName;
    RegisterClassExW(&wc);
    const UINT_PTR timer=SetTimer(nullptr,0,250,nullptr);
    if(!timer){Log("FG_SETTINGS_TIMER_FAILED error=%lu",GetLastError());return 0;}
    HWND monitor=nullptr, owner=nullptr;
    bool shown=false; RECT last{};
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
        TranslateMessage(&msg);DispatchMessageW(&msg);
        if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
        FGSettingsFlushSettingsIfDue();
        HWND game=FindFGGameWindowCached();
        if(game)FGSettingsRefreshDisplayTarget(game);
        if(IsFGRtx40Series()&&!IsFGSelectionAllowedByGpuPolicy(GetFGUserMultiplier())){
            SetFGUserMultiplier(2u);FGSettingsMarkSettingsDirty();
        }
        const bool visible=game&&FGSettingsGameIsForeground(game)&&IsFGVramMonitorEnabled();
        if(monitor&&owner!=game){DestroyWindow(monitor);monitor=nullptr;shown=false;}
        if(visible&&!monitor){
            monitor=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,
                kFGVramProbeClassName,L"Control FG VRAM",WS_POPUP,0,0,kFGVramProbeWidth,kFGVramProbeHeight,
                game,nullptr,selfModule,nullptr);
            owner=game;last={};
            if(monitor)SetLayeredWindowAttributes(monitor,0,238,LWA_ALPHA);
        }
        if(!monitor)continue;
        if(visible){
            POINT origin{18,18};
            if(ClientToScreen(game,&origin)){
                RECT next{origin.x,origin.y,origin.x+kFGVramProbeWidth,origin.y+kFGVramProbeHeight};
                if(!EqualRect(&last,&next)){
                    SetWindowPos(monitor,nullptr,next.left,next.top,kFGVramProbeWidth,kFGVramProbeHeight,
                        SWP_NOACTIVATE|SWP_NOZORDER);last=next;
                }
            }
            if(!shown){ShowWindow(monitor,SW_SHOWNOACTIVATE);shown=true;}
            InvalidateRect(monitor,nullptr,FALSE);
        }else if(shown){ShowWindow(monitor,SW_HIDE);shown=false;}
    }
    KillTimer(nullptr,timer);FGSettingsFlushSettingsIfDue(true);
    if(monitor)DestroyWindow(monitor);
    return 0;
}
static void StartFGSettingsService() noexcept {
    if(fgSettingsThreadHandle)return;
    FGSettingsLoadSettings();
    fgSettingsThreadHandle=CreateThread(nullptr,0,FGSettingsThreadProc,nullptr,0,nullptr);
    Log("FG_SETTINGS_SERVICE started=%u ui=native_menu_only input_hooks=none",unsigned(fgSettingsThreadHandle!=nullptr));
}
