#pragma once

#include "control_fg_logo_resource.h"
#include "settings_cog_image.h"
#include "rr_user_control.h"

// Process-local Control FG overlay for v2.0.0.
// The v0.8.26 generation path is intentionally untouched. This layer owns only
// UI, settings persistence, monitor refresh discovery, and read-only runtime status.
// The overlay starts hidden and is shown only by an explicit F10 press.
// This revision rebuilds the panel around Control's native display-menu proportions,
// typography hierarchy, selected-state treatment, and thick slider aesthetic.

static constexpr wchar_t kFGOverlayClassName[] = L"ControlFGOverlay_v1000";
static constexpr int kFGOverlayWidth = 740;
static constexpr int kFGOverlayHeight = 762;
static constexpr int kFGOverlayCompactHeight = 542;
static constexpr int kFGOverlayDynamicSectionHeight = 220;
static constexpr UINT_PTR kFGOverlayTimer = 0xCF32;
static constexpr ULONGLONG kFGSettingsDebounceMs = 350;
static std::atomic<unsigned int> fgOverlayVisible{0};
static HANDLE fgOverlayThreadHandle = nullptr;
static DWORD fgOverlayThreadId = 0;
static ULONGLONG fgOverlayLastRefreshPollMs = 0;
static HWND fgOverlayLastRefreshGame = nullptr;
static bool fgOverlaySliderDragging = false;
static bool fgOverlayClampDragging = false;
static unsigned fgOverlayClampPreview = 60;
static bool fgOverlaySettingsDirty = false;
static ULONGLONG fgOverlaySettingsDirtyMs = 0;
static ULONGLONG fgOverlaySettingsSavedPulseUntilMs = 0;
static std::wstring fgOverlaySettingsPath;
static std::atomic<unsigned int> fgOverlaySettingsLoaded{0};
static bool fgOverlayOptionsPage = false;
static bool fgOverlayRRSettingsPage = false;
// RR panel track spans 30..500, leaving room for Reset to default.
static unsigned FGOverlayClampFromX(int x) noexcept {
    if (x <= 30) return 25;
    if (x >= 500) return 75;
    return 25u + static_cast<unsigned>((x - 30) * 50 + 235) / 470u;
}
static int FGOverlayClampToX(unsigned value) noexcept {
    return 30 + static_cast<int>(control_rr_clamp::Normalize(value) - 25u) * 470 / 50;
}
static bool fgOverlayBindingCapture = false;
static bool fgOverlayBindingKeysDown[256]{};
static bool fgOverlayToggleWasDown = false; // async fallback only
static unsigned int fgOverlayToggleKey = VK_F10;
static constexpr UINT kFGOverlayToggleMessage = WM_APP + 0x3F2;
static HWND fgOverlayToggleWindow = nullptr;
static HHOOK fgOverlayToggleKeyboardHook = nullptr;
static unsigned int fgOverlayToggleHookDownKey = 0;
static std::atomic<unsigned int> fgOverlayToggleInjectedIgnored{0};
static const wchar_t* fgOverlayBindingMessage = L"Choose a single keyboard key. Esc cancels; F9 is reserved.";

static bool FGOverlayBindingKeyAllowed(unsigned int key) noexcept {
    return (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') ||
        (key >= VK_F1 && key <= VK_F24 && key != VK_F9) ||
        (key >= VK_NUMPAD0 && key <= VK_DIVIDE) ||
        key == VK_INSERT || key == VK_DELETE || key == VK_HOME || key == VK_END ||
        key == VK_PRIOR || key == VK_NEXT || key == VK_SPACE || key == VK_TAB ||
        key == VK_RETURN || key == VK_BACK;
}

static void FGOverlayBindingLabel(wchar_t (&text)[64]) noexcept {
    const UINT scan = MapVirtualKeyW(fgOverlayToggleKey, MAPVK_VK_TO_VSC);
    LONG keyData = static_cast<LONG>(scan << 16);
    if (fgOverlayToggleKey == VK_INSERT || fgOverlayToggleKey == VK_DELETE ||
        fgOverlayToggleKey == VK_HOME || fgOverlayToggleKey == VK_END ||
        fgOverlayToggleKey == VK_PRIOR || fgOverlayToggleKey == VK_NEXT ||
        fgOverlayToggleKey == VK_DIVIDE) keyData |= 1L << 24;
    if (!GetKeyNameTextW(keyData, text, 64)) swprintf_s(text, L"Key %u", fgOverlayToggleKey);
}

static HBITMAP fgOverlayLogoBitmap = nullptr;
static bool fgOverlayLogoLoadAttempted = false;
// CONTROL_FG_OVERLAY_STABILITY_TEST_R1
// Keep the 50 ms input timer, but decouple expensive window/compositor work from it.
static HWND fgOverlayCachedGame = nullptr;
static bool fgOverlayActuallyShown = false;
static RECT fgOverlayLastPlacement{};
static bool fgOverlayLastPlacementValid = false;
static ULONGLONG fgOverlayLastGameWindowSearchMs = 0;
static ULONGLONG fgOverlayLastPlacementPollMs = 0;
static ULONGLONG fgOverlayLastStatusPaintMs = 0;
static ULONGLONG fgOverlayLastTelemetryMs = 0;
static unsigned int fgOverlayPaintsSinceTelemetry = 0;
static unsigned int fgOverlayMovesSinceTelemetry = 0;
static unsigned int fgOverlaySearchesSinceTelemetry = 0;
static constexpr ULONGLONG kFGOverlayGameWindowSearchIntervalMs = 1000;
static constexpr ULONGLONG kFGOverlayPlacementPollIntervalMs = 250;
static constexpr ULONGLONG kFGOverlayStatusPaintIntervalMs = 250;
static constexpr ULONGLONG kFGOverlayTelemetryIntervalMs = 5000;

struct FGWindowSearch {
    DWORD pid;
    HWND best;
    long long bestArea;
};

static BOOL CALLBACK FGOverlayEnumWindows(HWND hwnd, LPARAM param) noexcept {
    auto* search = reinterpret_cast<FGWindowSearch*>(param);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != search->pid || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER)) return TRUE;
    wchar_t className[64]{};
    if (GetClassNameW(hwnd, className, _countof(className)) && wcscmp(className, kFGOverlayClassName) == 0) return TRUE;
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) return TRUE;
    const long long area = static_cast<long long>(rc.right - rc.left) * static_cast<long long>(rc.bottom - rc.top);
    if (area > search->bestArea) { search->best = hwnd; search->bestArea = area; }
    return TRUE;
}

static HWND FindFGGameWindow() noexcept {
    FGWindowSearch search{GetCurrentProcessId(), nullptr, 0};
    EnumWindows(FGOverlayEnumWindows, reinterpret_cast<LPARAM>(&search));
    ++fgOverlaySearchesSinceTelemetry;
    return search.best;
}

static HWND FindFGGameWindowCached(bool force = false) noexcept {
    const ULONGLONG now = GetTickCount64();
    if (fgOverlayCachedGame && !IsWindow(fgOverlayCachedGame)) {
        fgOverlayCachedGame = nullptr;
        fgOverlayLastGameWindowSearchMs = 0;
    }
    if (!force && fgOverlayCachedGame) return fgOverlayCachedGame;
    if (!force && fgOverlayLastGameWindowSearchMs &&
        now - fgOverlayLastGameWindowSearchMs < kFGOverlayGameWindowSearchIntervalMs) {
        return nullptr;
    }
    fgOverlayLastGameWindowSearchMs = now;
    fgOverlayCachedGame = FindFGGameWindow();
    return fgOverlayCachedGame;
}

static bool FGOverlayGameIsForeground(HWND game) noexcept {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(foreground, &pid);
    return pid == GetCurrentProcessId() || foreground == game;
}

static bool FGOverlayResolveSettingsPath() noexcept {
    if (!fgOverlaySettingsPath.empty()) return true;
    wchar_t base[32768]{};
    const DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", base, _countof(base));
    if (!count || count >= _countof(base)) return false;
    std::wstring dir = std::wstring(base) + L"\\ControlFG";
    if (!CreateDirectoryW(dir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    fgOverlaySettingsPath = dir + L"\\settings-mfg-test.ini";
    const std::wstring originalSettings=dir+L"\\settings.ini";
    CopyFileW(originalSettings.c_str(),fgOverlaySettingsPath.c_str(),TRUE);
    return true;
}

static void FGOverlayLoadSettings() noexcept {
    unsigned int expected = 0;
    if (!fgOverlaySettingsLoaded.compare_exchange_strong(expected, 1u)) return;
    if (!FGOverlayResolveSettingsPath()) {
        SetFGDynamicManualTargetFps(0);
        control_rr::RRUserSetSharpnessPercent(0);
        control_rr::RRUserSetSharpnessOverrideEnabled(false);
        control_rr::RRUserSetPresetValue(control_rr::RRPresetF);
        control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
        control_rr::RRUserMarkPresetRestartRequired(false);
        control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
        control_rr::RRUserRequest(false);
        Log("FG_SETTINGS_LOAD success=0 reason=path_unavailable defaults=4x,dynamic_auto,rr_off,rr_model_F ui=fg_full_rr_toggle_model_E_F");
        return;
    }

    const unsigned int storedKey = GetPrivateProfileIntW(L"Overlay", L"ToggleKey", VK_F10, fgOverlaySettingsPath.c_str());
    fgOverlayToggleKey = FGOverlayBindingKeyAllowed(storedKey) ? storedKey : VK_F10;
    const unsigned int rawSelection = GetPrivateProfileIntW(L"FrameGeneration", L"Mode", kSLDefaultMultiplier, fgOverlaySettingsPath.c_str());
    const unsigned int selection = IsFGRtx40Series() ? 2u : NormalizeFGMultiplier(rawSelection);
    const unsigned int rawTarget = GetPrivateProfileIntW(L"FrameGeneration", L"DynamicTargetFPS", 0, fgOverlaySettingsPath.c_str());
    const unsigned int manualTarget = ClampFGDynamicManualTargetFps(rawTarget);
    control_rr_clamp::Select(control_rr_clamp::Normalize(GetPrivateProfileIntW(L"RayReconstruction", L"ReflectionClamp", 60, fgOverlaySettingsPath.c_str())));
    const bool rrEnabled = GetPrivateProfileIntW(L"RayReconstruction", L"Enabled", 0, fgOverlaySettingsPath.c_str()) != 0;
    const unsigned int rawRrPreset = GetPrivateProfileIntW(L"RayReconstruction", L"Preset", control_rr::RRPresetF, fgOverlaySettingsPath.c_str());
    const unsigned int rrPreset = rawRrPreset == control_rr::RRPresetE ? control_rr::RRPresetE : control_rr::RRPresetF;

    slFgUserMultiplier.store(selection, std::memory_order_release);
    slFgDynamicManualTargetFps.store(manualTarget, std::memory_order_release);
    control_rr::RRUserSetSharpnessPercent(0);
    control_rr::RRUserSetSharpnessOverrideEnabled(false);
    control_rr::RRUserSetPresetValue(rrPreset);
    control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
    control_rr::RRUserMarkPresetRestartRequired(false);
    control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
    control_rr::RRUserRequest(rrEnabled);
    Log("FG_SETTINGS_LOAD success=1 path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u rr_enabled=%u rr_preset=%s rr_preset_value=%u skin_mode=off schema=8 ui=fg_full_rr_toggle_model_E_F",
        fgOverlaySettingsPath.c_str(), GetFGSelectionName(selection), selection, manualTarget ? "manual" : "auto", manualTarget, unsigned(rrEnabled), control_rr::RRUserPresetLabel(), control_rr::RRUserPresetValue());
}

static bool FGOverlaySaveSettingsNow() noexcept {
    if (!FGOverlayResolveSettingsPath()) {
        Log("FG_SETTINGS_SAVE success=0 reason=path_unavailable ui=fg_full_rr_toggle_only");
        return false;
    }

    wchar_t clampText[8]{};
    swprintf_s(clampText,L"%u",control_rr_clamp::Requested());
    wchar_t experimentalText[8]{};
    swprintf_s(experimentalText, L"%u", IsRTX40MFGRequested() ? 1u : 0u);
    wchar_t bindingText[16]{};
    swprintf_s(bindingText, L"%u", fgOverlayToggleKey);
    wchar_t modeText[16]{};
    wchar_t targetText[16]{};
    wchar_t rrEnabledText[8]{};
    wchar_t rrPresetText[8]{};
    swprintf_s(modeText, L"%u", GetFGUserMultiplier());
    swprintf_s(targetText, L"%u", GetFGDynamicManualTargetFps());
    swprintf_s(rrEnabledText, L"%u", control_rr::RRUserRequested() ? 1u : 0u);
    swprintf_s(rrPresetText, L"%u", control_rr::RRUserPresetValue());

    bool ok = WritePrivateProfileStringW(L"ControlFG", L"Schema", L"8", fgOverlaySettingsPath.c_str()) != FALSE;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"Mode", modeText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"DynamicTargetFPS", targetText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"Enabled", rrEnabledText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"Preset", rrPresetText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"RayReconstruction", L"ReflectionClamp", clampText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"Overlay", L"ToggleKey", bindingText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"Experimental", L"RTX40MultiFG", experimentalText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    // Old experimental RR/AA settings are intentionally removed; FG settings remain intact.
    WritePrivateProfileStringW(L"AntiAliasing", nullptr, nullptr, fgOverlaySettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinResponsivity", nullptr, fgOverlaySettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinPreserveNative", nullptr, fgOverlaySettingsPath.c_str());
    if (ok) WritePrivateProfileStringW(nullptr, nullptr, nullptr, fgOverlaySettingsPath.c_str());

    Log("FG_SETTINGS_SAVE success=%u path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u rr_enabled=%u rr_preset=%s rr_preset_value=%u schema=8 ui=fg_full_rr_toggle_model_E_F",
        unsigned(ok), fgOverlaySettingsPath.c_str(), GetFGSelectionName(GetFGUserMultiplier()), GetFGUserMultiplier(),
        GetFGDynamicManualTargetFps() ? "manual" : "auto", GetFGDynamicManualTargetFps(), unsigned(control_rr::RRUserRequested()), control_rr::RRUserPresetLabel(), control_rr::RRUserPresetValue());
    if (ok) fgOverlaySettingsSavedPulseUntilMs = GetTickCount64() + 1200ull;
    return ok;
}

static void FGOverlayMarkSettingsDirty() noexcept {
    fgOverlaySettingsDirty = true;
    fgOverlaySettingsDirtyMs = GetTickCount64();
}

static void FGOverlayFlushSettingsIfDue(bool force = false) noexcept {
    if (!fgOverlaySettingsDirty) return;
    const ULONGLONG now = GetTickCount64();
    if (!force && now - fgOverlaySettingsDirtyMs < kFGSettingsDebounceMs) return;
    if (FGOverlaySaveSettingsNow()) fgOverlaySettingsDirty = false;
}

static unsigned int FGOverlayDetectDisplayRefreshMilliHz(HWND game, const char** sourceOut) noexcept {
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

static void FGOverlayRefreshDisplayTarget(HWND game, bool force = false) noexcept {
    if (!game) return;
    const ULONGLONG now = GetTickCount64();
    if (!force && game == fgOverlayLastRefreshGame && now - fgOverlayLastRefreshPollMs < 1000ull) return;
    const bool gameChanged = game != fgOverlayLastRefreshGame;
    fgOverlayLastRefreshGame = game;
    fgOverlayLastRefreshPollMs = now;

    const char* source = nullptr;
    const unsigned int milliHz = FGOverlayDetectDisplayRefreshMilliHz(game, &source);
    if (milliHz) SetFGDynamicDetectedRefreshMilliHz(milliHz, source);
    else if (gameChanged) SetFGDynamicDetectedRefreshMilliHz(0, "unresolved_new_window");
}

static void PaintFGRect(HDC dc, const RECT& box, COLORREF fill, COLORREF border, int borderWidth = 1) noexcept {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, borderWidth, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Rectangle(dc, box.left, box.top, box.right, box.bottom);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static void PaintFGDivider(HDC dc, int x1, int y, int x2) noexcept {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(101, 101, 101));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, x1, y, nullptr);
    LineTo(dc, x2, y);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

static void PaintFGVerticalDivider(HDC dc, int x, int y1, int y2) noexcept {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(77, 77, 77));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, x, y1, nullptr);
    LineTo(dc, x, y2);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

static HFONT CreateFGControlFont(int height, int weight, const wchar_t* face) noexcept {
    return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

static HBITMAP FGOverlayGetLogoBitmap() noexcept {
    if (fgOverlayLogoLoadAttempted) return fgOverlayLogoBitmap;
    fgOverlayLogoLoadAttempted = true;

    HRSRC resource = FindResourceW(selfModule, MAKEINTRESOURCEW(IDR_CONTROL_FG_LOGO_BGRA), RT_RCDATA);
    if (!resource) {
        Log("FG_OVERLAY_LOGO_LOAD success=0 stage=find_resource error=%lu", GetLastError());
        return nullptr;
    }
    const DWORD resourceSize = SizeofResource(selfModule, resource);
    if (resourceSize != CONTROL_FG_LOGO_BYTES) {
        Log("FG_OVERLAY_LOGO_LOAD success=0 stage=size expected=%u actual=%lu",
            unsigned(CONTROL_FG_LOGO_BYTES), resourceSize);
        return nullptr;
    }
    HGLOBAL loaded = LoadResource(selfModule, resource);
    const void* source = loaded ? LockResource(loaded) : nullptr;
    if (!source) {
        Log("FG_OVERLAY_LOGO_LOAD success=0 stage=lock_resource error=%lu", GetLastError());
        return nullptr;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = CONTROL_FG_LOGO_WIDTH;
    info.bmiHeader.biHeight = -CONTROL_FG_LOGO_HEIGHT; // top-down BGRA rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (screen) ReleaseDC(nullptr, screen);
    if (!bitmap || !pixels) {
        if (bitmap) DeleteObject(bitmap);
        Log("FG_OVERLAY_LOGO_LOAD success=0 stage=create_dib error=%lu", GetLastError());
        return nullptr;
    }

    memcpy(pixels, source, CONTROL_FG_LOGO_BYTES);
    fgOverlayLogoBitmap = bitmap;
    Log("FG_OVERLAY_LOGO_LOAD success=1 resource=%u size=%ux%u bytes=%u alpha=premultiplied_bgra",
        unsigned(IDR_CONTROL_FG_LOGO_BGRA), unsigned(CONTROL_FG_LOGO_WIDTH),
        unsigned(CONTROL_FG_LOGO_HEIGHT), unsigned(CONTROL_FG_LOGO_BYTES));
    return fgOverlayLogoBitmap;
}

static void PaintFGLogo(HDC dc, int x, int y) noexcept {
    HBITMAP bitmap = FGOverlayGetLogoBitmap();
    if (!bitmap) return;
    HDC sourceDc = CreateCompatibleDC(dc);
    if (!sourceDc) return;
    HGDIOBJ oldBitmap = SelectObject(sourceDc, bitmap);
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(dc, x, y, CONTROL_FG_LOGO_WIDTH, CONTROL_FG_LOGO_HEIGHT,
        sourceDc, 0, 0, CONTROL_FG_LOGO_WIDTH, CONTROL_FG_LOGO_HEIGHT, blend);
    SelectObject(sourceDc, oldBitmap);
    DeleteDC(sourceDc);
}

static HBITMAP fgSettingsCogBitmap = nullptr;
static void PaintFGSettingsCog(HDC dc, int x, int y) noexcept {
    if (!fgSettingsCogBitmap) {
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = 30;
        info.bmiHeader.biHeight = -30;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!bitmap || !pixels) { if (bitmap) DeleteObject(bitmap); return; }
        memcpy(pixels, kFGSettingsCogBGRA, sizeof(kFGSettingsCogBGRA));
        fgSettingsCogBitmap = bitmap;
    }
    HDC sourceDc = CreateCompatibleDC(dc);
    if (!sourceDc) return;
    HGDIOBJ old = SelectObject(sourceDc, fgSettingsCogBitmap);
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    AlphaBlend(dc, x, y, 30, 30, sourceDc, 0, 0, 30, 30, blend);
    SelectObject(sourceDc, old);
    DeleteDC(sourceDc);
}

static void PaintFGButton(HDC dc, const RECT& button, const wchar_t* label, bool active, bool supported = true) noexcept {
    const bool selected = active && supported;
    const COLORREF fill = selected ? RGB(246, 246, 246) : RGB(0, 0, 0);
    const COLORREF border = selected ? RGB(246, 246, 246) : (supported ? RGB(87, 87, 87) : RGB(42, 42, 42));
    PaintFGRect(dc, button, fill, border);
    SetTextColor(dc, selected ? RGB(12, 12, 12) : (supported ? RGB(238, 238, 238) : RGB(78, 78, 78)));
    DrawTextW(dc, label, -1, const_cast<RECT*>(&button), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static unsigned int FGOverlaySliderFpsFromX(int x, const RECT& slider) noexcept {
    const int left = slider.left;
    const int right = slider.right;
    if (x < left) x = left;
    if (x > right) x = right;
    const double t = right > left ? static_cast<double>(x - left) / static_cast<double>(right - left) : 0.0;
    const double span = static_cast<double>(kSLDynamicTargetMaxFps - kSLDynamicTargetMinFps);
    return ClampFGDynamicManualTargetFps(static_cast<unsigned int>(static_cast<double>(kSLDynamicTargetMinFps) + t * span + 0.5));
}

static int FGOverlaySliderXFromFps(unsigned int fps, const RECT& slider) noexcept {
    fps = ClampFGDynamicManualTargetFps(fps ? fps : kSLDynamicTargetFallbackFps);
    const int left = slider.left;
    const int right = slider.right;
    const double span = static_cast<double>(kSLDynamicTargetMaxFps - kSLDynamicTargetMinFps);
    const double t = span > 0.0 ? static_cast<double>(fps - kSLDynamicTargetMinFps) / span : 0.0;
    return left + static_cast<int>(t * static_cast<double>(right - left) + 0.5);
}

static void PaintFGTargetSlider(HDC dc, const RECT& slider, unsigned int fps, bool enabled) noexcept {
    const int trackY = (slider.top + slider.bottom) / 2;
    const int knobX = FGOverlaySliderXFromFps(fps, slider);
    const COLORREF trackFill = enabled ? RGB(29, 29, 29) : RGB(17, 17, 17);
    const COLORREF trackBorder = enabled ? RGB(91, 91, 91) : RGB(50, 50, 50);
    const COLORREF progressFill = enabled ? RGB(204, 204, 204) : RGB(72, 72, 72);
    const COLORREF knobFill = enabled ? RGB(245, 245, 245) : RGB(87, 87, 87);
    RECT track{slider.left, trackY - 13, slider.right, trackY + 13};
    PaintFGRect(dc, track, trackFill, trackBorder);
    RECT progress{slider.left, trackY - 13, knobX, trackY + 13};
    if (progress.right > progress.left) PaintFGRect(dc, progress, progressFill, progressFill);
    RECT knob{knobX - 8, trackY - 19, knobX + 9, trackY + 19};
    PaintFGRect(dc, knob, knobFill, knobFill);
}

static const wchar_t* FGOverlayRuntimeStatus(unsigned int selected, bool applied, bool apiEnabled,
                                             bool dynamicKnown, bool dynamicSupported,
                                             unsigned int maxSupported) noexcept {
    if (selected == kSLSelectionOff) return L"OFF";
    if (selected == kSLSelectionDynamic && dynamicKnown && !dynamicSupported) return L"UNSUPPORTED";
    if (selected >= 2 && maxSupported && selected > maxSupported) return L"UNSUPPORTED";
    if (apiEnabled && applied) return L"ACTIVE";
    if (!apiEnabled) return L"WAITING";
    return L"APPLYING";
}

static void PaintFGOverlay(HWND hwnd) noexcept {
    PAINTSTRUCT ps{};
    HDC paintDc = BeginPaint(hwnd, &ps);
    if (!paintDc) return;
    RECT client{};
    GetClientRect(hwnd, &client);
    const int clientWidth = client.right - client.left;
    const int clientHeight = client.bottom - client.top;

    // Draw the complete panel off-screen and publish it with one BitBlt. This keeps
    // DWM/capture software from observing the intermediate clear/text/control passes.
    HDC bufferDc = nullptr;
    HBITMAP bufferBitmap = nullptr;
    HGDIOBJ oldBufferBitmap = nullptr;
    HDC dc = paintDc;
    if (clientWidth > 0 && clientHeight > 0) {
        bufferDc = CreateCompatibleDC(paintDc);
        if (bufferDc) {
            bufferBitmap = CreateCompatibleBitmap(paintDc, clientWidth, clientHeight);
            if (bufferBitmap) {
                oldBufferBitmap = SelectObject(bufferDc, bufferBitmap);
                if (oldBufferBitmap && oldBufferBitmap != HGDI_ERROR) {
                    dc = bufferDc;
                } else {
                    DeleteObject(bufferBitmap);
                    bufferBitmap = nullptr;
                    DeleteDC(bufferDc);
                    bufferDc = nullptr;
                    oldBufferBitmap = nullptr;
                }
            } else {
                DeleteDC(bufferDc);
                bufferDc = nullptr;
            }
        }
    }

    HBRUSH background = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &client, background);
    DeleteObject(background);
    SetBkMode(dc, TRANSPARENT);
    {
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(92, 92, 92));
        HGDIOBJ oldPen = SelectObject(dc, borderPen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, 0, 0, client.right - 1, client.bottom - 1);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(borderPen);
    }

    // Native-style typography hierarchy. Bahnschrift is a condensed geometric Windows face
    // and is intentionally used instead of the generic Segoe UI from previous revisions.
    HFONT metricFont = CreateFGControlFont(29, FW_SEMIBOLD, L"Bahnschrift SemiCondensed");
    HFONT bodyFont = CreateFGControlFont(19, FW_NORMAL, L"Bahnschrift SemiCondensed");
    HFONT buttonFont = CreateFGControlFont(18, FW_NORMAL, L"Bahnschrift SemiCondensed");
    HFONT buttonSelectedFont = CreateFGControlFont(18, FW_BOLD, L"Bahnschrift SemiBold SemiCondensed");
    HFONT smallFont = CreateFGControlFont(14, FW_NORMAL, L"Bahnschrift SemiCondensed");
    HFONT labelFont = CreateFGControlFont(13, FW_SEMIBOLD, L"Bahnschrift SemiCondensed");
    HFONT sectionFont = CreateFGControlFont(20, FW_BOLD, L"Bahnschrift Condensed");
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dc, smallFont));
    const int left = 28;
    const int right = kFGOverlayWidth - 28;

    PaintFGLogo(dc, left, 22);
    SelectObject(dc, smallFont);
    SetTextColor(dc, RGB(185, 185, 185));
    RECT versionRc{410, 31, 480, 55};
    DrawTextW(dc, L"v2.0.0", -1, &versionRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    PaintFGSettingsCog(dc, 502, 26);
    RECT keyBox{545, 25, 666, 57};
    SelectObject(dc, buttonSelectedFont);
    wchar_t bindingLabel[64]{};
    FGOverlayBindingLabel(bindingLabel);
    PaintFGButton(dc, keyBox, bindingLabel, true);
    PaintFGVerticalDivider(dc, 676, 27, 55);
    SelectObject(dc, bodyFont);
    SetTextColor(dc, RGB(232, 232, 232));
    RECT hideRc{687, 28, 730, 56};
    DrawTextW(dc, L"Hide", -1, &hideRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    PaintFGDivider(dc, left, 86, right);

    if (fgOverlayOptionsPage) {
        SelectObject(dc, sectionFont); SetTextColor(dc, RGB(255, 32, 32));
        RECT title{30, 105, 500, 140};
        DrawTextW(dc, L"OPTIONS", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, bodyFont); SetTextColor(dc, RGB(246, 246, 246));
        RECT label{30, 170, 350, 210};
        DrawTextW(dc, L"Overlay shortcut", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT bind{380, 165, 710, 215};
        PaintFGButton(dc, bind, fgOverlayBindingCapture ? L"Press a key..." : bindingLabel, true);
        SelectObject(dc, smallFont); SetTextColor(dc, RGB(232, 232, 232));
        RECT help{30, 232, 710, 300};
        DrawTextW(dc, fgOverlayBindingMessage, -1, &help, DT_LEFT | DT_WORDBREAK);
        SelectObject(dc, buttonFont);
        PaintFGButton(dc, RECT{30, 325, 270, 375}, L"Reset to F10", false);
        {
            const bool editable = IsFGRtx40Series();
            SelectObject(dc, sectionFont); SetTextColor(dc, RGB(255, 32, 32));
            RECT experimentalTitle{30, 390, 710, 422};
            DrawTextW(dc, L"EXPERIMENTAL", -1, &experimentalTitle, DT_LEFT | DT_SINGLELINE);
            const bool requested = editable && IsRTX40MFGRequested();
            PaintFGRect(dc, RECT{30, 440, 56, 466}, RGB(0,0,0), editable ? RGB(232,232,232) : RGB(95,95,95));
            if (requested) {
                HPEN checkPen = CreatePen(PS_SOLID, 3, RGB(246,246,246));
                HGDIOBJ oldCheckPen = SelectObject(dc, checkPen);
                MoveToEx(dc, 35, 452, nullptr); LineTo(dc, 41, 459); LineTo(dc, 52, 446);
                SelectObject(dc, oldCheckPen); DeleteObject(checkPen);
            }
            SelectObject(dc, bodyFont); SetTextColor(dc, editable ? RGB(246,246,246) : RGB(125,125,125));
            RECT experimentalLabel{70, 437, 710, 473};
            DrawTextW(dc, L"Enable RTX 40-series Multi Frame Generation", -1, &experimentalLabel, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            SelectObject(dc, smallFont);
            RECT experimentalHelp{30, 485, 710, 548};
            const wchar_t* note = !editable ? L"RTX 40-series only. RTX 50-series uses native NVIDIA Multi Frame Generation."
                : requested != IsRTX40MFGSessionEnabled()
                ? L"Restart required to change the FG implementation. Disabled: native NVIDIA FG, up to 2x."
                : (requested ? L"Experimental MFG enabled. Disabling requires a restart to restore native NVIDIA FG."
                             : L"Native NVIDIA Frame Generation, up to 2x. Enabling experimental MFG requires a restart.");
            DrawTextW(dc, note, -1, &experimentalHelp, DT_LEFT | DT_WORDBREAK);
        }
        SelectObject(dc,buttonFont);
        PaintFGButton(dc, RECT{550, 560, 710, 610}, L"Back", false);
    } else if (fgOverlayRRSettingsPage) {
        SelectObject(dc, sectionFont); SetTextColor(dc, RGB(255,32,32));
        RECT title{30,105,710,140};
        DrawTextW(dc,L"RAY RECONSTRUCTION SETTINGS",-1,&title,DT_LEFT|DT_SINGLELINE);
        SelectObject(dc,bodyFont); SetTextColor(dc,RGB(246,246,246));
        const unsigned value=fgOverlayClampDragging?fgOverlayClampPreview:control_rr_clamp::Requested();
        wchar_t label[80]{}; swprintf_s(label,L"Reflection clamp: %u%%",value);
        RECT labelRc{30,170,710,205};
        DrawTextW(dc,label,-1,&labelRc,DT_LEFT|DT_SINGLELINE);
        const int knob=FGOverlayClampToX(value);
        PaintFGRect(dc,RECT{30,234,500,256},RGB(45,45,45),RGB(95,95,95));
        PaintFGRect(dc,RECT{30,234,knob,256},RGB(240,240,240),RGB(240,240,240));
        PaintFGRect(dc,RECT{knob-6,224,knob+6,266},RGB(255,255,255),RGB(255,255,255));
        SelectObject(dc,buttonFont);
        PaintFGButton(dc,RECT{530,220,710,270},L"Reset to default",false);
        SelectObject(dc,smallFont); SetTextColor(dc,RGB(246,246,246));
        RECT lo{30,280,240,305},hi{290,280,500,305},def{530,280,710,305};
        DrawTextW(dc,L"25% - Looser",-1,&lo,DT_LEFT|DT_SINGLELINE);
        DrawTextW(dc,L"75% - Tighter",-1,&hi,DT_RIGHT|DT_SINGLELINE);
        DrawTextW(dc,L"Default: 60%",-1,&def,DT_CENTER|DT_SINGLELINE);
        wchar_t status[180]{};
        swprintf_s(status,L"Last RR: %u%%%s. Release to apply. Saves automatically.",control_rr_clamp::effective.load(),control_rr_clamp::applied.load()?L"":L" (native fallback)");
        RECT help{30,330,710,375}; DrawTextW(dc,status,-1,&help,DT_LEFT|DT_WORDBREAK);
        SelectObject(dc,buttonFont);
        PaintFGButton(dc,RECT{550,410,710,460},L"Back",false);
    } else {
    const unsigned int selected = GetFGUserMultiplier();
    const unsigned int maxSupported = GetFGMaxSupportedMultiplier();
    const bool dynamicKnown = IsFGDynamicCapabilityKnown();
    const bool dynamicSupported = IsFGDynamicMFGSupported();
    const bool apiEnabled = slFgEnabledByApi.load(std::memory_order_acquire) != 0;
    const bool applied = IsFGSelectionAppliedForOverlay();
    const unsigned int effective = GetFGEffectiveMultiplierForOverlay();
    const unsigned int manualTarget = GetFGDynamicManualTargetFps();
    const unsigned int detectedMilliHz = GetFGDynamicDetectedRefreshMilliHz();
    const bool hdrActive = IsHdr10BridgeActive();

    const int colX[5] = {30, 168, 306, 444, 582};
    const wchar_t* labels[5] = {L"MODE", L"EFFECTIVE", L"CURRENT FPS", L"HDR", L"GPU MAX"};
    for (int i = 0; i < 5; ++i) {
        SelectObject(dc, labelFont);
        SetTextColor(dc, RGB(169, 169, 169));
        RECT labelRc{colX[i], 104, colX[i] + 116, 124};
        DrawTextW(dc, labels[i], -1, &labelRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        if (i) PaintFGVerticalDivider(dc, colX[i] - 15, 101, 168);
    }

    SelectObject(dc, metricFont);
    SetTextColor(dc, RGB(246, 246, 246));
    RECT modeValue{30, 126, 145, 160};
    DrawTextW(dc, GetFGSelectionNameWide(selected), -1, &modeValue, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    wchar_t effectiveText[32]{};
    if (effective) swprintf_s(effectiveText, L"%ux", effective); else swprintf_s(effectiveText, L"--");
    RECT effectiveRc{168, 126, 282, 160};
    DrawTextW(dc, effectiveText, -1, &effectiveRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    const float currentFps = GetFGCurrentFpsForOverlay();
    wchar_t currentFpsText[32]{};
    if (currentFps > 0.0f) swprintf_s(currentFpsText, L"%.0f", static_cast<double>(currentFps)); else swprintf_s(currentFpsText, L"--");
    RECT currentFpsRc{306, 126, 420, 160};
    DrawTextW(dc, currentFpsText, -1, &currentFpsRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT hdrRc{444, 126, 558, 160};
    DrawTextW(dc, hdrActive ? L"ON" : L"OFF", -1, &hdrRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    wchar_t maxText[32]{};
    if (maxSupported) swprintf_s(maxText, L"%ux", maxSupported); else swprintf_s(maxText, L"--");
    RECT maxRc{582, 126, 704, 160};
    DrawTextW(dc, maxText, -1, &maxRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, smallFont);
    const wchar_t* runtimeStatus = FGOverlayRuntimeStatus(selected, applied, apiEnabled, dynamicKnown, dynamicSupported, maxSupported);
    SetTextColor(dc, apiEnabled && applied ? RGB(71, 224, 139) : RGB(163, 163, 163));
    RECT runtimeRc{30, 157, 145, 176};
    DrawTextW(dc, runtimeStatus, -1, &runtimeRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    PaintFGDivider(dc, left, 186, right);

    SelectObject(dc, sectionFont);
    SetTextColor(dc, RGB(243, 45, 45));
    RECT fgLabel{30, 208, 360, 235};
    DrawTextW(dc, L"FRAME GENERATION", -1, &fgLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    const unsigned int modeValues[7] = {kSLSelectionOff, kSLSelectionDynamic, 2, 3, 4, 5, 6};
    const wchar_t* modeLabels[7] = {L"OFF", L"DYNAMIC", L"2X", L"3X", L"4X", L"5X", L"6X"};
    const int modeWidths[7] = {80, 104, 76, 76, 76, 76, 76};
    int x = 30;
    const int modeY = 246, modeHeight = 52, modeGap = 8;
    for (int i = 0; i < 7; ++i) {
        const unsigned int value = modeValues[i];
        const bool gpuPolicyAllowed = IsFGSelectionAllowedByGpuPolicy(value);
        const bool supported = gpuPolicyAllowed && (value == kSLSelectionOff ||
            (value == kSLSelectionDynamic ? (!dynamicKnown || dynamicSupported) : (!maxSupported || value <= maxSupported)));
        RECT button{x, modeY, x + modeWidths[i], modeY + modeHeight};
        SelectObject(dc, (selected == value && supported) ? buttonSelectedFont : buttonFont);
        PaintFGButton(dc, button, modeLabels[i], selected == value, supported);
        x += modeWidths[i] + modeGap;
    }
    PaintFGDivider(dc, left, 322, right);

    const bool dynamicSectionVisible = IsFGDynamicSelection(selected);
    const int lowerSectionOffset = dynamicSectionVisible ? 0 : -kFGOverlayDynamicSectionHeight;
    if (dynamicSectionVisible) {
        const bool dynamicControlsSupported = IsFGDynamicAllowedByGpuPolicy();
        SelectObject(dc, sectionFont);
        SetTextColor(dc, dynamicControlsSupported ? RGB(243, 45, 45) : RGB(89, 43, 43));
        RECT dynamicLabel{30, 344, 390, 372};
        DrawTextW(dc, L"DYNAMIC TARGET FPS", -1, &dynamicLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        const RECT autoButton{30, 386, 142, 434};
        const RECT manualButton{152, 386, 278, 434};
        SelectObject(dc, (manualTarget == 0 && dynamicControlsSupported) ? buttonSelectedFont : buttonFont);
        PaintFGButton(dc, autoButton, L"AUTO", manualTarget == 0, dynamicControlsSupported);
        SelectObject(dc, (manualTarget != 0 && dynamicControlsSupported) ? buttonSelectedFont : buttonFont);
        PaintFGButton(dc, manualButton, L"MANUAL", manualTarget != 0, dynamicControlsSupported);

        wchar_t targetSummary[96]{};
        if (manualTarget) swprintf_s(targetSummary, L"Target  %u FPS", manualTarget);
        else if (detectedMilliHz) swprintf_s(targetSummary, L"Target  %.0f FPS", static_cast<double>(detectedMilliHz) / 1000.0);
        else swprintf_s(targetSummary, L"Target  Auto");
        SelectObject(dc, bodyFont);
        SetTextColor(dc, dynamicControlsSupported ? RGB(238, 238, 238) : RGB(78, 78, 78));
        RECT summaryRc{316, 388, 690, 432};
        DrawTextW(dc, targetSummary, -1, &summaryRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        unsigned int sliderFps = manualTarget;
        if (!sliderFps && detectedMilliHz) sliderFps = (detectedMilliHz + 500u) / 1000u;
        if (!sliderFps) sliderFps = kSLDynamicTargetFallbackFps;
        const RECT sliderBox{30, 456, 710, 500};
        PaintFGTargetSlider(dc, sliderBox, sliderFps, dynamicControlsSupported && manualTarget != 0);

        SelectObject(dc, smallFont);
        SetTextColor(dc, dynamicControlsSupported ? RGB(177, 177, 177) : RGB(72, 72, 72));
        RECT minRc{30, 500, 80, 522};
        DrawTextW(dc, L"30", -1, &minRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT maxRc2{655, 500, 710, 522};
        DrawTextW(dc, L"1000", -1, &maxRc2, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        PaintFGDivider(dc, left, 542, right);
    }

    SelectObject(dc, smallFont);
    SetTextColor(dc, RGB(160, 160, 160));
    RECT footerRc{30, 556 + lowerSectionOffset, 610, 580 + lowerSectionOffset};
    DrawTextW(dc, L"Settings save automatically  |  changes apply on the next presented frame", -1, &footerRc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (GetTickCount64() < fgOverlaySettingsSavedPulseUntilMs) {
        SetTextColor(dc, RGB(237, 237, 237));
        RECT savedRc{626, 556 + lowerSectionOffset, 708, 580 + lowerSectionOffset};
        DrawTextW(dc, L"SAVED", -1, &savedRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    PaintFGDivider(dc, left, 594 + lowerSectionOffset, right);
    SelectObject(dc, sectionFont); SetTextColor(dc, RGB(210, 32, 32));
    RECT rrTitle{30, 614 + lowerSectionOffset, 510, 642 + lowerSectionOffset};
    DrawTextW(dc, L"RAY RECONSTRUCTION", -1, &rrTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    const bool rrEnabled = control_rr::RRUserRequested();
    const unsigned int rrPreset = control_rr::RRUserPresetValue();
    const RECT rrToggle{30, 656 + lowerSectionOffset, 280, 708 + lowerSectionOffset};
    SelectObject(dc, rrEnabled ? buttonSelectedFont : buttonFont);
    PaintFGButton(dc, rrToggle, rrEnabled ? L"RR ON" : L"RR OFF", rrEnabled, true);
    PaintFGSettingsCog(dc, 287, 667 + lowerSectionOffset);

    SelectObject(dc, labelFont);
    SetTextColor(dc, RGB(169, 169, 169));
    RECT rrModelLabel{332, 656 + lowerSectionOffset, 392, 708 + lowerSectionOffset};
    DrawTextW(dc, L"MODEL", -1, &rrModelLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    const RECT rrPresetE{408, 656 + lowerSectionOffset, 524, 708 + lowerSectionOffset};
    const RECT rrPresetF{536, 656 + lowerSectionOffset, 652, 708 + lowerSectionOffset};
    SelectObject(dc, rrPreset == control_rr::RRPresetE ? buttonSelectedFont : buttonFont);
    PaintFGButton(dc, rrPresetE, L"E", rrPreset == control_rr::RRPresetE, true);
    SelectObject(dc, rrPreset == control_rr::RRPresetF ? buttonSelectedFont : buttonFont);
    PaintFGButton(dc, rrPresetF, L"F", rrPreset == control_rr::RRPresetF, true);


    }
    SelectObject(dc, oldFont);
    DeleteObject(metricFont); DeleteObject(bodyFont); DeleteObject(buttonFont); DeleteObject(buttonSelectedFont);
    DeleteObject(smallFont); DeleteObject(labelFont); DeleteObject(sectionFont);

    if (bufferDc && bufferBitmap) {
        BitBlt(paintDc, 0, 0, clientWidth, clientHeight, bufferDc, 0, 0, SRCCOPY);
        SelectObject(bufferDc, oldBufferBitmap);
        DeleteObject(bufferBitmap);
        DeleteDC(bufferDc);
    }
    ++fgOverlayPaintsSinceTelemetry;
    EndPaint(hwnd, &ps);
}

static unsigned int FGOverlayMultiplierFromPoint(int x, int y) noexcept {
    const unsigned int values[7] = {kSLSelectionOff, kSLSelectionDynamic, 2, 3, 4, 5, 6};
    const int widths[7] = {80, 104, 76, 76, 76, 76, 76};
    const int buttonY = 246, height = 52, gap = 8;
    if (y < buttonY || y >= buttonY + height) return 0xFFFFFFFFu;
    int left = 30;
    for (int i = 0; i < 7; ++i) {
        if (x >= left && x < left + widths[i]) return values[i];
        left += widths[i] + gap;
    }
    return 0xFFFFFFFFu;
}
static bool FGOverlayAutoTargetFromPoint(int x, int y) noexcept { return IsFGDynamicSelection(GetFGUserMultiplier()) && y >= 386 && y < 434 && x >= 30 && x < 142; }
static bool FGOverlayManualTargetFromPoint(int x, int y) noexcept { return IsFGDynamicSelection(GetFGUserMultiplier()) && y >= 386 && y < 434 && x >= 152 && x < 278; }
static bool FGOverlaySliderFromPoint(int x, int y) noexcept {
    return IsFGDynamicSelection(GetFGUserMultiplier()) && y >= 443 && y < 512 && x >= 30 && x < 710;
}
static int FGOverlayLowerSectionOffset() noexcept { return IsFGDynamicSelection(GetFGUserMultiplier()) ? 0 : -kFGOverlayDynamicSectionHeight; }
static int FGOverlayDesiredHeight() noexcept { if (fgOverlayOptionsPage) return 640; if (fgOverlayRRSettingsPage) return 490; return IsFGDynamicSelection(GetFGUserMultiplier()) ? kFGOverlayHeight : kFGOverlayCompactHeight; }
static void FGOverlayResizeWindowForCurrentSelection(HWND hwnd) noexcept {
    if (!hwnd) return;
    SetWindowPos(hwnd, nullptr, 0, 0, kFGOverlayWidth, FGOverlayDesiredHeight(),
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    fgOverlayLastPlacementValid = false;
}
static bool FGOverlayRRToggleFromPoint(int x,int y) noexcept { const int y0=656+FGOverlayLowerSectionOffset(); return y>=y0&&y<y0+52&&x>=30&&x<280; }
static unsigned int FGOverlayRRPresetFromPoint(int x,int y) noexcept {
    const int y0=656+FGOverlayLowerSectionOffset();
    if(y<y0||y>=y0+52)return 0;
    if(x>=408&&x<524)return control_rr::RRPresetE;
    if(x>=536&&x<652)return control_rr::RRPresetF;
    return 0;
}
static void FGOverlayApplySliderPoint(HWND hwnd, int x) noexcept {
    if (!IsFGDynamicSelection(GetFGUserMultiplier())) return;
    const RECT slider{30, 456, 710, 500};
    const unsigned int fps = FGOverlaySliderFpsFromX(x, slider);
    SetFGDynamicManualTargetFps(fps);
    FGOverlayMarkSettingsDirty();
    Log("FG_OVERLAY_DYNAMIC_TARGET action=slider policy=%s manual_target_fps=%u detected_refresh_fps=%.3f resolved_target_fps=%.3f persistence=dirty",
        GetFGDynamicTargetPolicyName(), GetFGDynamicManualTargetFps(), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
        static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
    InvalidateRect(hwnd, nullptr, FALSE);
}


static LRESULT CALLBACK FGOverlayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    switch (message) {
    case kFGOverlayToggleMessage: {
        if (fgOverlayBindingCapture || !FGOverlayGameIsForeground(nullptr)) return 0;
        const unsigned int next = fgOverlayVisible.load(std::memory_order_acquire) ? 0u : 1u;
        fgOverlayVisible.store(next, std::memory_order_release);
        Log("FG_OVERLAY_TOGGLE visible=%u key=%u input=physical_keyboard_hook injected_ignored=%u",
            next, fgOverlayToggleKey, fgOverlayToggleInjectedIgnored.load(std::memory_order_acquire));
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintFGOverlay(hwnd);
        return 0;
    case WM_LBUTTONDOWN: {
        const int x = static_cast<short>(LOWORD(lParam));
        const int y = static_cast<short>(HIWORD(lParam));
        if (y >= 20 && y < 65 && x >= 680) {
            fgOverlayBindingCapture = false;
            fgOverlayVisible.store(0, std::memory_order_release);
            FGOverlayFlushSettingsIfDue(true);
            return 0;
        }
        if (y >= 20 && y < 65 && x >= 495 && x < 540) {
            fgOverlayOptionsPage = !fgOverlayOptionsPage;
            fgOverlayRRSettingsPage = false;
            fgOverlayClampDragging = false;
            fgOverlayBindingCapture = false;
            fgOverlaySliderDragging = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            FGOverlayResizeWindowForCurrentSelection(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (fgOverlayOptionsPage) {
            if (x >= 380 && x < 710 && y >= 165 && y < 215) {
                for (int key = 0; key < 256; ++key)
                    fgOverlayBindingKeysDown[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
                fgOverlayBindingCapture = true;
                fgOverlayBindingMessage = L"Press a single key. Esc cancels; F9 is reserved. Modifier combinations are not supported.";
            } else if (x >= 30 && x < 270 && y >= 325 && y < 375) {
                fgOverlayBindingCapture = false;
                fgOverlayToggleKey = VK_F10;
                fgOverlayToggleHookDownKey = 0;
                fgOverlayToggleWasDown = !fgOverlayToggleKeyboardHook &&
                    ((GetAsyncKeyState(VK_F10) & 0x8000) != 0);
                FGOverlayMarkSettingsDirty();
                FGOverlayFlushSettingsIfDue(true);
                fgOverlayBindingMessage = fgOverlaySettingsDirty ? L"F10 restored for this session. Saving failed; retrying." : L"Shortcut restored to F10 and saved.";
            } else if (IsFGRtx40Series() && x >= 30 && x < 710 && y >= 437 && y < 475) {
                fgOverlayBindingCapture = false;
                const bool requested = !IsRTX40MFGRequested();
                slRtx40MfgRequested.store(requested ? 1u : 0u);
                if (!requested && GetFGUserMultiplier() != kSLSelectionOff) SetFGUserMultiplier(2u);
                FGOverlayMarkSettingsDirty();
                FGOverlayFlushSettingsIfDue(true);
                fgOverlayBindingMessage = fgOverlaySettingsDirty ? L"Saving failed; retrying. Keep the game open until settings save." : L"Settings saved.";
                Log("FG_RTX40_EXPERIMENTAL_SETTING requested=%u session_enabled=%u restart_required=%u", unsigned(requested), unsigned(IsRTX40MFGSessionEnabled()), unsigned(requested != IsRTX40MFGSessionEnabled()));
            } else if (x >= 550 && x < 710 && y >= 560 && y < 610) {
                fgOverlayBindingCapture = false;
                fgOverlayOptionsPage = false;
                FGOverlayResizeWindowForCurrentSelection(hwnd);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0; // Options never fall through to hidden FG/RR controls.
        }
        if (fgOverlayRRSettingsPage) {
            if (x >= 24 && x <= 506 && y >= 220 && y < 270) {
                fgOverlayClampPreview=FGOverlayClampFromX(x);
                fgOverlayClampDragging=true;
                SetCapture(hwnd);
            } else if (x >= 530 && x < 710 && y >= 220 && y < 270) {
                fgOverlayClampPreview=60;
                control_rr_clamp::Select(60);
                FGOverlayMarkSettingsDirty();
                FGOverlayFlushSettingsIfDue(true);
                Log("RR_CLAMP_RESET_CS4 selected=60");
            } else if (x >= 550 && x < 710 && y >= 410 && y < 460) {
                fgOverlayRRSettingsPage=false;
                FGOverlayResizeWindowForCurrentSelection(hwnd);
            }
            InvalidateRect(hwnd,nullptr,FALSE);
            return 0; // RR settings never activate hidden main-page controls.
        }
        const int rrSettingsY=656+FGOverlayLowerSectionOffset();
        if (x >= 282 && x < 326 && y >= rrSettingsY && y < rrSettingsY+52) {
            fgOverlayRRSettingsPage=true;
            fgOverlaySliderDragging=false;
            if (GetCapture()==hwnd) ReleaseCapture();
            FGOverlayResizeWindowForCurrentSelection(hwnd);
            InvalidateRect(hwnd,nullptr,FALSE);
            return 0;
        }
        if(FGOverlayRRToggleFromPoint(x,y)){
            const bool previous=control_rr::RRUserRequested();
            control_rr::RRUserMarkPresetRestartRequired(false);
            control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
            control_rr::RRUserSetSharpnessOverrideEnabled(false);
            control_rr::RRUserRequest(!previous);
            FGOverlayMarkSettingsDirty();
            Log("RR_TOGGLE previous=%u enabled=%u preset=%s preset_value=%u partial_hidden=1 skin_hidden=1 sharpness_hidden=1 apply=next_frame_boundary persistence=dirty",unsigned(previous),unsigned(!previous),control_rr::RRUserPresetLabel(),control_rr::RRUserPresetValue());
            InvalidateRect(hwnd,nullptr,FALSE);return 0;
        }
        const unsigned int rrPresetSelection=FGOverlayRRPresetFromPoint(x,y);
        if(rrPresetSelection){
            const unsigned int previousPreset=control_rr::RRUserPresetValue();
            if(previousPreset!=rrPresetSelection){
                control_rr::RRUserSetPresetValue(rrPresetSelection);
                control_rr::RRUserMarkPresetRestartRequired(false);
                FGOverlayMarkSettingsDirty();
                Log("RR_PRESET_UI previous=%u selected=%u selected_label=%s rr_enabled=%u live_switch=1 apply=frame_boundary persistence=dirty",previousPreset,rrPresetSelection,control_rr::RRUserPresetLabel(),unsigned(control_rr::RRUserRequested()));
                InvalidateRect(hwnd,nullptr,FALSE);
            }
            return 0;
        }
        const unsigned int selection = FGOverlayMultiplierFromPoint(x, y);
        if (selection != 0xFFFFFFFFu) {
            const unsigned int maxSupported = GetFGMaxSupportedMultiplier();
            const bool blockedGpuPolicy = !IsFGSelectionAllowedByGpuPolicy(selection);
            const bool blockedDynamic = selection == kSLSelectionDynamic &&
                                        IsFGDynamicCapabilityKnown() && !IsFGDynamicMFGSupported();
            const bool blockedFixed = selection >= 2 && maxSupported && selection > maxSupported;
            if (blockedGpuPolicy || blockedDynamic || blockedFixed) {
                const char* reason = blockedGpuPolicy ? "rtx40_unlock_or_dynamic_capability_unavailable" :
                    (blockedDynamic ? "dynamic_unsupported" : "fixed_above_gpu_max");
                Log("FG_OVERLAY_SELECTION_BLOCKED selection=%s code=%u reason=%s max_supported=%u dynamic_known=%u dynamic_supported=%u rtx40_series=%u",
                    GetFGSelectionName(selection), selection, reason,
                    maxSupported, unsigned(IsFGDynamicCapabilityKnown()), unsigned(IsFGDynamicMFGSupported()), unsigned(IsFGRtx40Series()));
                return 0;
            }
            SetFGUserMultiplier(selection);
            FGOverlayResizeWindowForCurrentSelection(hwnd);
            FGOverlayMarkSettingsDirty();
            InvalidateRect(hwnd, nullptr, FALSE);
            Log("FG_OVERLAY_SELECTION selection=%s code=%u requested_generated=%u mode=%s persistence=dirty",
                GetFGSelectionName(selection), selection, selection >= 2 ? selection - 1 : 0,
                IsFGDynamicSelection(selection) ? "dynamic" : (selection ? "fixed" : "off"));
            return 0;
        }
        if (FGOverlayAutoTargetFromPoint(x, y)) {
            if (!IsFGDynamicAllowedByGpuPolicy()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=dynamic_runtime_unavailable action=auto");
                return 0;
            }
            SetFGDynamicManualTargetFps(0);
            FGOverlayMarkSettingsDirty();
            Log("FG_OVERLAY_DYNAMIC_TARGET action=auto policy=%s manual_target_fps=0 detected_refresh_fps=%.3f resolved_target_fps=%.3f persistence=dirty",
                GetFGDynamicTargetPolicyName(), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (FGOverlayManualTargetFromPoint(x, y)) {
            if (!IsFGDynamicAllowedByGpuPolicy()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=dynamic_runtime_unavailable action=manual");
                return 0;
            }
            unsigned int manualFps = GetFGDynamicManualTargetFps();
            if (!manualFps) {
                const unsigned int detected = GetFGDynamicDetectedRefreshMilliHz();
                manualFps = detected ? (detected + 500u) / 1000u : kSLDynamicTargetFallbackFps;
                SetFGDynamicManualTargetFps(manualFps);
                FGOverlayMarkSettingsDirty();
            }
            Log("FG_OVERLAY_DYNAMIC_TARGET action=manual policy=%s manual_target_fps=%u detected_refresh_fps=%.3f resolved_target_fps=%.3f persistence=dirty",
                GetFGDynamicTargetPolicyName(), GetFGDynamicManualTargetFps(),
                static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (FGOverlaySliderFromPoint(x, y)) {
            if (!IsFGDynamicAllowedByGpuPolicy()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=dynamic_runtime_unavailable action=slider");
                return 0;
            }
            fgOverlaySliderDragging = true;
            SetCapture(hwnd);
            FGOverlayApplySliderPoint(hwnd, x);
            return 0;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if(fgOverlayClampDragging){
            fgOverlayClampPreview=FGOverlayClampFromX(static_cast<short>(LOWORD(lParam)));
            InvalidateRect(hwnd,nullptr,FALSE);return 0;
        }
        if (fgOverlaySliderDragging && !IsFGDynamicSelection(GetFGUserMultiplier())) {
            fgOverlaySliderDragging = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            return 0;
        }
        if (fgOverlaySliderDragging && (wParam & MK_LBUTTON)) {
            FGOverlayApplySliderPoint(hwnd, static_cast<short>(LOWORD(lParam)));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if(fgOverlayClampDragging){
            fgOverlayClampPreview=FGOverlayClampFromX(static_cast<short>(LOWORD(lParam)));
            fgOverlayClampDragging=false;
            control_rr_clamp::Select(fgOverlayClampPreview);
            FGOverlayMarkSettingsDirty();FGOverlayFlushSettingsIfDue(true);
            if(GetCapture()==hwnd)ReleaseCapture();
            Log("RR_CLAMP_SLIDER_CS4 selected=%u",fgOverlayClampPreview);
            InvalidateRect(hwnd,nullptr,FALSE);return 0;
        }
        if (fgOverlaySliderDragging) {
            fgOverlaySliderDragging = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            FGOverlayFlushSettingsIfDue(true);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        fgOverlayClampDragging=false;
        fgOverlaySliderDragging = false;
        break;
    case WM_CLOSE:
        fgOverlayClampDragging=false;
        if(GetCapture()==hwnd)ReleaseCapture();
        fgOverlayBindingCapture = false;
        FGOverlayFlushSettingsIfDue(true);
        fgOverlayVisible.store(0, std::memory_order_release);
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

static bool PositionFGOverlay(HWND overlay, HWND game, bool force = false) noexcept {
    if (!overlay || !game) return false;
    RECT client{};
    if (!GetClientRect(game, &client) || client.right <= client.left || client.bottom <= client.top) return false;
    POINT origin{0, 0};
    if (!ClientToScreen(game, &origin)) return false;
    const int clientWidth = client.right - client.left;
    const int x = origin.x + ((clientWidth > kFGOverlayWidth + 36) ? (clientWidth - kFGOverlayWidth - 18) : 18);
    const int overlayHeight = FGOverlayDesiredHeight();
    RECT placement{x, origin.y + 18, x + kFGOverlayWidth, origin.y + 18 + overlayHeight};
    if (!force && fgOverlayLastPlacementValid && EqualRect(&placement, &fgOverlayLastPlacement)) return false;
    if (!SetWindowPos(overlay, HWND_TOPMOST, placement.left, placement.top, kFGOverlayWidth, overlayHeight,
        SWP_NOACTIVATE)) return false;
    fgOverlayLastPlacement = placement;
    fgOverlayLastPlacementValid = true;
    ++fgOverlayMovesSinceTelemetry;
    return true;
}

static void FGOverlaySetShown(HWND overlay, HWND game, bool show) noexcept {
    if (show == fgOverlayActuallyShown) return;
    fgOverlayActuallyShown = show;
    const ULONGLONG now = GetTickCount64();
    if (show) {
        PositionFGOverlay(overlay, game, true);
        ShowWindow(overlay, SW_SHOWNOACTIVATE);
        InvalidateRect(overlay, nullptr, FALSE);
        fgOverlayLastPlacementPollMs = now;
        fgOverlayLastStatusPaintMs = now;
        fgOverlayLastTelemetryMs = now;
        fgOverlayPaintsSinceTelemetry = 0;
        fgOverlayMovesSinceTelemetry = 0;
        fgOverlaySearchesSinceTelemetry = 0;
    } else {
        ShowWindow(overlay, SW_HIDE);
        fgOverlayLastPlacementValid = false;
    }
    Log("FG_OVERLAY_VISIBILITY_TRANSITION shown=%u", unsigned(show));
}

static void FGOverlayLogCadenceIfDue() noexcept {
    const ULONGLONG now = GetTickCount64();
    if (!fgOverlayLastTelemetryMs) {
        fgOverlayLastTelemetryMs = now;
        return;
    }
    const ULONGLONG elapsed = now - fgOverlayLastTelemetryMs;
    if (elapsed < kFGOverlayTelemetryIntervalMs) return;
    const double paintHz = elapsed ? (static_cast<double>(fgOverlayPaintsSinceTelemetry) * 1000.0 / static_cast<double>(elapsed)) : 0.0;
    Log("FG_OVERLAY_CADENCE interval_ms=%llu paints=%u paint_hz=%.2f placement_updates=%u window_searches=%u visible=%u",
        static_cast<unsigned long long>(elapsed), fgOverlayPaintsSinceTelemetry, paintHz,
        fgOverlayMovesSinceTelemetry, fgOverlaySearchesSinceTelemetry, unsigned(fgOverlayActuallyShown));
    fgOverlayLastTelemetryMs = now;
    fgOverlayPaintsSinceTelemetry = 0;
    fgOverlayMovesSinceTelemetry = 0;
    fgOverlaySearchesSinceTelemetry = 0;
}

// r30 legacy Windows-HDR hotkey serialization source retained for comparison. r31
// no longer installs or services this hook; HDR recovery is display/resize-driven
// and uses a hard DLSS-G resource reset instead of delaying Win+Alt+B.
// The old low-level hook lived on the existing
// overlay message thread so the callback remains tiny and never touches DXGI.
// It only consumes the physical B event for Win+Alt+B while DLSS-G is active;
// the render thread performs the actual eOff/Present commit. Once committed, the
// overlay thread replays the shortcut with a private dwExtraInfo tag so our own
// hook will always pass the synthetic event through.
static constexpr ULONG_PTR kFGHdrHotkeyReplayTag = static_cast<ULONG_PTR>(0x4346474844524255ull);
static std::atomic<unsigned int> fgOverlayHdrHotkeyPhysicalBConsumed{0};
static HHOOK fgOverlayKeyboardHook = nullptr;

static bool FGOverlayKeyDown(int vk) noexcept {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Overlay toggle input is intentionally non-consuming. The old 50 ms
// GetAsyncKeyState edge detector could observe transient synthetic/high-state
// input from third-party overlays (reproduced under GOG Galaxy) and interpret it
// as a real F10 press. The low-level hook only accepts a non-injected physical
// keydown, ignores autorepeat until the matching keyup, and posts the actual
// visibility change back to this overlay thread. If hook installation fails, the
// timer retains the historical GetAsyncKeyState path as a fallback.
static LRESULT CALLBACK FGOverlayToggleLowLevelKeyboardProc(int code, WPARAM message, LPARAM param) noexcept {
    if (code < 0 || !param) return CallNextHookEx(nullptr, code, message, param);
    const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(param);
    const bool keyDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool keyUp = message == WM_KEYUP || message == WM_SYSKEYUP;

    if (keyUp && fgOverlayToggleHookDownKey == key->vkCode) {
        fgOverlayToggleHookDownKey = 0;
        return CallNextHookEx(nullptr, code, message, param);
    }
    if (!keyDown || key->vkCode != fgOverlayToggleKey)
        return CallNextHookEx(nullptr, code, message, param);

    if ((key->flags & LLKHF_INJECTED) != 0) {
        fgOverlayToggleInjectedIgnored.fetch_add(1u, std::memory_order_relaxed);
        return CallNextHookEx(nullptr, code, message, param);
    }
    if (fgOverlayToggleHookDownKey == key->vkCode)
        return CallNextHookEx(nullptr, code, message, param);

    fgOverlayToggleHookDownKey = key->vkCode;
    if (!fgOverlayBindingCapture && fgOverlayToggleWindow &&
        FGOverlayGameIsForeground(nullptr)) {
        PostMessageW(fgOverlayToggleWindow, kFGOverlayToggleMessage, 0, 0);
    }
    return CallNextHookEx(nullptr, code, message, param);
}

static LRESULT CALLBACK FGOverlayLowLevelKeyboardProc(int code, WPARAM message, LPARAM param) noexcept {
    if (code < 0 || !param) return CallNextHookEx(nullptr, code, message, param);
    const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(param);
    if (key->dwExtraInfo == kFGHdrHotkeyReplayTag || (key->flags & LLKHF_INJECTED) != 0)
        return CallNextHookEx(nullptr, code, message, param);

    const bool keyDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool keyUp = message == WM_KEYUP || message == WM_SYSKEYUP;
    if (key->vkCode != 'B' || (!keyDown && !keyUp))
        return CallNextHookEx(nullptr, code, message, param);

    if (keyUp && fgOverlayHdrHotkeyPhysicalBConsumed.exchange(0u, std::memory_order_acq_rel) != 0u) {
        Log("FG_HDR_HOTKEY_INPUT event=b_up action=consume_original");
        return 1;
    }
    if (!keyDown) return CallNextHookEx(nullptr, code, message, param);

    if (fgOverlayHdrHotkeyPhysicalBConsumed.load(std::memory_order_acquire) != 0u)
        return 1; // repeat while the original B remains physically held

    if (!FGOverlayGameIsForeground(nullptr)) return CallNextHookEx(nullptr, code, message, param);
    const bool winDown = FGOverlayKeyDown(VK_LWIN) || FGOverlayKeyDown(VK_RWIN);
    const bool altDown = FGOverlayKeyDown(VK_MENU);
    if (!winDown || !altDown) return CallNextHookEx(nullptr, code, message, param);

    if (!ArmSLHdrHotkeyGuardFromInput()) {
        Log("FG_HDR_HOTKEY_INPUT event=b_down chord=win_alt_b fg_enabled=0 action=pass_through");
        return CallNextHookEx(nullptr, code, message, param);
    }

    fgOverlayHdrHotkeyPhysicalBConsumed.store(1u, std::memory_order_release);
    Log("FG_HDR_HOTKEY_INPUT event=b_down chord=win_alt_b fg_enabled=1 action=consume_until_fg_off_present_commits");
    return 1;
}

static void FGOverlayFillKeyInput(INPUT& input, WORD vk, DWORD flags) noexcept {
    input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = flags;
    input.ki.dwExtraInfo = kFGHdrHotkeyReplayTag;
}

static bool FGOverlayTryReplayWindowsHdrHotkey() noexcept {
    if (!IsSLHdrHotkeyReplayReady()) return false;

    // Never synthesize while the original B is physically down. If Win+Alt are
    // still held after B comes up, replay only B for the most natural shortcut.
    // If both modifiers have already been released, replay the complete chord.
    const bool bDown = FGOverlayKeyDown('B');
    const bool winDown = FGOverlayKeyDown(VK_LWIN) || FGOverlayKeyDown(VK_RWIN);
    const bool altDown = FGOverlayKeyDown(VK_MENU);
    if (bDown) return false;

    const auto replayMode = control_fg_hdr_hotkey::ChooseReplayMode(
        static_cast<control_fg_hdr_hotkey::Stage>(GetSLHdrHotkeyStageValue()), bDown, winDown, altDown);
    if (replayMode == control_fg_hdr_hotkey::ReplayMode::None) return false;

    INPUT inputs[6]{};
    UINT count = 0;
    const char* mode = nullptr;
    if (replayMode == control_fg_hdr_hotkey::ReplayMode::BOnlyModifiersStillDown) {
        FGOverlayFillKeyInput(inputs[count++], 'B', 0);
        FGOverlayFillKeyInput(inputs[count++], 'B', KEYEVENTF_KEYUP);
        mode = "b_only_modifiers_still_down";
    } else {
        FGOverlayFillKeyInput(inputs[count++], VK_LWIN, 0);
        FGOverlayFillKeyInput(inputs[count++], VK_MENU, 0);
        FGOverlayFillKeyInput(inputs[count++], 'B', 0);
        FGOverlayFillKeyInput(inputs[count++], 'B', KEYEVENTF_KEYUP);
        FGOverlayFillKeyInput(inputs[count++], VK_MENU, KEYEVENTF_KEYUP);
        FGOverlayFillKeyInput(inputs[count++], VK_LWIN, KEYEVENTF_KEYUP);
        mode = "full_chord_after_release";
    }

    SetLastError(ERROR_SUCCESS);
    const UINT sent = SendInput(count, inputs, sizeof(INPUT));
    const DWORD error = sent == count ? ERROR_SUCCESS : GetLastError();
    const bool success = sent == count;
    Log("FG_HDR_HOTKEY_SENDINPUT mode=%s requested=%u sent=%u success=%u error=%lu", mode, count, sent, unsigned(success), error);
    MarkSLHdrHotkeyReplayResult(success);
    return success;
}

static DWORD WINAPI FGOverlayThreadProc(LPVOID) noexcept {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = FGOverlayWndProc;
    wc.hInstance = selfModule;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kFGOverlayClassName;
    RegisterClassExW(&wc);

    HWND overlay = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        kFGOverlayClassName, L"Control FG", WS_POPUP, 0, 0, kFGOverlayWidth, FGOverlayDesiredHeight(),
        nullptr, nullptr, selfModule, nullptr);
    if (!overlay) {
        Log("FG_OVERLAY_CREATE_FAILED error=%lu", GetLastError());
        return 0;
    }
    SetLayeredWindowAttributes(overlay, 0, 248, LWA_ALPHA);
    ShowWindow(overlay, SW_HIDE);
    fgOverlayKeyboardHook = nullptr;
    Log("FG_HDR_HOTKEY_HOOK installed=0 hook=null error=0 policy=disabled_r31_display_or_resize_detected_hard_dlssg_reset");

    fgOverlayToggleWindow = overlay;
    SetLastError(ERROR_SUCCESS);
    fgOverlayToggleKeyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL, FGOverlayToggleLowLevelKeyboardProc, selfModule, 0);
    const DWORD toggleHookError = fgOverlayToggleKeyboardHook ? ERROR_SUCCESS : GetLastError();
    Log("FG_OVERLAY_HOTKEY_HOOK installed=%u hook=%p error=%lu mode=physical_nonexclusive ignore_injected=1 fallback=GetAsyncKeyState",
        unsigned(fgOverlayToggleKeyboardHook != nullptr), fgOverlayToggleKeyboardHook, toggleHookError);

    Log("FG_OVERLAY_READY hwnd=%p hotkey=F10 input=physical_keyboard_hook_with_async_fallback default_selection=%s selector=off,dynamic,2x,3x,4x,5x,6x dynamic_auto_target=explicit_game_monitor_refresh dynamic_manual_target=30-1000_fps rr_controls=on_off_plus_model_E_F rr_model_default=F rr_model_live_switch=E_F partial=hidden skin=hidden aa_sharpness=hidden controls=auto,manual,thick_slider_30_1000 presentation=os_layered_noactivate_control_native_menu_anchor_top_right startup_visibility=hidden_f10_only persistence=localappdata_ini runtime_status=selected,effective,current_fps,hdr,capability title=embedded_control_fg_logo_control_native font=bahnschrift_semicondensed selected_style=white_fill_black_text section_headers=control_red no_side_scrollbar=1 dynamic_vsync_policy=syncinterval0_while_active dynamic_reflex_limiter=target_fps gpu_policy=rtx40_off_plus_2x_only",
        overlay, GetFGSelectionName(GetFGUserMultiplier()));
    Log("FG_OVERLAY_STABILITY_TEST revision=R1 double_buffered=1 timer_ms=50 window_search_ms=1000 placement_poll_ms=250 status_paint_ms=250 setwindowpos_on_change_only=1 showhide_on_transition_only=1 fg_path_unchanged=v0.8.26");
    Log("FG_OVERLAY_SETTINGS_S5 binding=persisted_single_key options=replacement_page default=F10");
    SetTimer(overlay, kFGOverlayTimer, 50, nullptr);

    fgOverlayToggleWasDown = !fgOverlayToggleKeyboardHook &&
        ((GetAsyncKeyState(fgOverlayToggleKey) & 0x8000) != 0);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_TIMER) {
            const ULONGLONG now = GetTickCount64();
            FGOverlayFlushSettingsIfDue(false);
            HWND game = FindFGGameWindowCached(false);
            if (game) FGOverlayRefreshDisplayTarget(game);
            if (IsFGRtx40Series()) {
                const unsigned int selected = GetFGUserMultiplier();
                if (!IsFGSelectionAllowedByGpuPolicy(selected)) {
                    SetFGUserMultiplier(2u);
                    FGOverlayResizeWindowForCurrentSelection(overlay);
                    FGOverlayMarkSettingsDirty();
                    Log("FG_OVERLAY_GPU_POLICY_COERCE gpu=rtx40 previous_selection=%s previous_code=%u selected_selection=2x selected_code=2 persistence=dirty",
                        GetFGSelectionName(selected), selected);
                }
            }
            const bool gameForeground = game && FGOverlayGameIsForeground(game);
            // r31 does not intercept or replay Win+Alt+B. Windows owns the shortcut;
            // recovery is driven by fresh DXGI output / Control resize detection.
            const bool capturingBinding = fgOverlayBindingCapture;
            if (capturingBinding) {
                if (!gameForeground) {
                    fgOverlayBindingCapture = false;
                    fgOverlayBindingMessage = L"Binding canceled because the game lost focus.";
                } else {
                    for (unsigned int key = 8; key < 256; ++key) {
                        const bool down = (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
                        const bool pressed = down && !fgOverlayBindingKeysDown[key];
                        fgOverlayBindingKeysDown[key] = down;
                        if (!pressed) continue;
                        if (key == VK_ESCAPE) {
                            fgOverlayBindingCapture = false;
                            fgOverlayBindingMessage = L"Binding canceled. Your shortcut has not changed.";
                            break;
                        }
                        const bool modifiers = ((GetAsyncKeyState(VK_CONTROL) | GetAsyncKeyState(VK_MENU) |
                            GetAsyncKeyState(VK_SHIFT) | GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;
                        if (!FGOverlayBindingKeyAllowed(key) || modifiers) {
                            fgOverlayBindingMessage = L"Choose a single letter, number, function or navigation key. F9 is reserved; Esc cancels.";
                            continue;
                        }
                        fgOverlayToggleKey = key;
                        fgOverlayBindingCapture = false;
                        FGOverlayMarkSettingsDirty();
                        FGOverlayFlushSettingsIfDue(true);
                        fgOverlayBindingMessage = fgOverlaySettingsDirty ? L"Shortcut changed for this session. Saving failed; retrying." : L"Shortcut saved. Press it to show or hide the overlay.";
                        Log("FG_OVERLAY_BINDING key=%u saved=%u", key, unsigned(!fgOverlaySettingsDirty));
                        break;
                    }
                }
                if (!fgOverlayBindingCapture) InvalidateRect(overlay, nullptr, FALSE);
            }
            if (!fgOverlayToggleKeyboardHook) {
                const bool toggleDown =
                    (GetAsyncKeyState(static_cast<int>(fgOverlayToggleKey)) & 0x8000) != 0;
                if (!capturingBinding && gameForeground && toggleDown && !fgOverlayToggleWasDown) {
                    const unsigned int next =
                        fgOverlayVisible.load(std::memory_order_acquire) ? 0u : 1u;
                    fgOverlayVisible.store(next, std::memory_order_release);
                    Log("FG_OVERLAY_TOGGLE visible=%u key=%u input=GetAsyncKeyState_fallback",
                        next, fgOverlayToggleKey);
                }
                fgOverlayToggleWasDown = toggleDown;
            }
            const bool show = gameForeground && fgOverlayVisible.load(std::memory_order_acquire);
            FGOverlaySetShown(overlay, game, show);
            if (show) {
                if (now - fgOverlayLastPlacementPollMs >= kFGOverlayPlacementPollIntervalMs) {
                    PositionFGOverlay(overlay, game, false);
                    fgOverlayLastPlacementPollMs = now;
                }
                if (now - fgOverlayLastStatusPaintMs >= kFGOverlayStatusPaintIntervalMs) {
                    InvalidateRect(overlay, nullptr, FALSE);
                    fgOverlayLastStatusPaintMs = now;
                }
                FGOverlayLogCadenceIfDue();
            }
        }
    }
    FGOverlayFlushSettingsIfDue(true);
    if (fgOverlayToggleKeyboardHook) {
        UnhookWindowsHookEx(fgOverlayToggleKeyboardHook);
        fgOverlayToggleKeyboardHook = nullptr;
        Log("FG_OVERLAY_HOTKEY_HOOK installed=0 reason=overlay_thread_exit");
    }
    fgOverlayToggleWindow = nullptr;
    fgOverlayToggleHookDownKey = 0;
    if (fgOverlayKeyboardHook) {
        UnhookWindowsHookEx(fgOverlayKeyboardHook);
        fgOverlayKeyboardHook = nullptr;
        Log("FG_HDR_HOTKEY_HOOK installed=0 reason=overlay_thread_exit");
    }
    if (fgSettingsCogBitmap) { DeleteObject(fgSettingsCogBitmap); fgSettingsCogBitmap = nullptr; }
    DestroyWindow(overlay);
    return 0;
}

static void StartFGOverlay() noexcept {
    if (fgOverlayThreadHandle) return;
    FGOverlayLoadSettings();
    fgOverlayThreadHandle = CreateThread(nullptr, 0, FGOverlayThreadProc, nullptr, 0, &fgOverlayThreadId);
    if (fgOverlayThreadHandle) Log("FG_OVERLAY_THREAD_STARTED thread_id=%lu settings_loaded=%u", fgOverlayThreadId, fgOverlaySettingsLoaded.load());
    else Log("FG_OVERLAY_THREAD_FAILED error=%lu", GetLastError());
}
