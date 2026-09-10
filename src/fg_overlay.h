#pragma once

#include "control_fg_logo_resource.h"

// Process-local Control FG overlay for v1.0.0.
// The v0.8.26 generation path is intentionally untouched. This layer owns only
// UI, settings persistence, monitor refresh discovery, and read-only runtime status.
// The overlay starts hidden and is shown only by an explicit F10 press.
// This revision rebuilds the panel around Control's native display-menu proportions,
// typography hierarchy, selected-state treatment, and thick slider aesthetic.

static constexpr wchar_t kFGOverlayClassName[] = L"ControlFGOverlay_v1000";
static constexpr int kFGOverlayWidth = 740;
static constexpr int kFGOverlayHeight = 600;
static constexpr UINT_PTR kFGOverlayTimer = 0xCF32;
static constexpr ULONGLONG kFGSettingsDebounceMs = 350;
static std::atomic<unsigned int> fgOverlayVisible{0};
static HANDLE fgOverlayThreadHandle = nullptr;
static DWORD fgOverlayThreadId = 0;
static ULONGLONG fgOverlayLastRefreshPollMs = 0;
static HWND fgOverlayLastRefreshGame = nullptr;
static bool fgOverlaySliderDragging = false;
static bool fgOverlaySettingsDirty = false;
static ULONGLONG fgOverlaySettingsDirtyMs = 0;
static ULONGLONG fgOverlaySettingsSavedPulseUntilMs = 0;
static std::wstring fgOverlaySettingsPath;
static std::atomic<unsigned int> fgOverlaySettingsLoaded{0};
static HBITMAP fgOverlayLogoBitmap = nullptr;
static bool fgOverlayLogoLoadAttempted = false;

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
    return search.best;
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
    fgOverlaySettingsPath = dir + L"\\settings.ini";
    return true;
}

static void FGOverlayLoadSettings() noexcept {
    unsigned int expected = 0;
    if (!fgOverlaySettingsLoaded.compare_exchange_strong(expected, 1u)) return;
    if (!FGOverlayResolveSettingsPath()) {
        Log("FG_SETTINGS_LOAD success=0 reason=path_unavailable defaults=4x,auto");
        return;
    }

    const unsigned int rawSelection = GetPrivateProfileIntW(L"FrameGeneration", L"Mode", kSLDefaultMultiplier, fgOverlaySettingsPath.c_str());
    const unsigned int selection = NormalizeFGMultiplier(rawSelection);
    const unsigned int rawTarget = GetPrivateProfileIntW(L"FrameGeneration", L"DynamicTargetFPS", 0, fgOverlaySettingsPath.c_str());
    const unsigned int manualTarget = ClampFGDynamicManualTargetFps(rawTarget);

    slFgUserMultiplier.store(selection, std::memory_order_release);
    slFgDynamicManualTargetFps.store(manualTarget, std::memory_order_release);
    Log("FG_SETTINGS_LOAD success=1 path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u schema=1",
        fgOverlaySettingsPath.c_str(), GetFGSelectionName(selection), selection, manualTarget ? "manual" : "auto", manualTarget);
}

static bool FGOverlaySaveSettingsNow() noexcept {
    if (!FGOverlayResolveSettingsPath()) {
        Log("FG_SETTINGS_SAVE success=0 reason=path_unavailable");
        return false;
    }

    wchar_t modeText[16]{};
    wchar_t targetText[16]{};
    swprintf_s(modeText, L"%u", GetFGUserMultiplier());
    swprintf_s(targetText, L"%u", GetFGDynamicManualTargetFps());

    bool ok = WritePrivateProfileStringW(L"ControlFG", L"Schema", L"1", fgOverlaySettingsPath.c_str()) != FALSE;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"Mode", modeText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"FrameGeneration", L"DynamicTargetFPS", targetText, fgOverlaySettingsPath.c_str()) != FALSE) && ok;
    if (ok) WritePrivateProfileStringW(nullptr, nullptr, nullptr, fgOverlaySettingsPath.c_str());

    Log("FG_SETTINGS_SAVE success=%u path=%ls selection=%s selected_code=%u target_policy=%s manual_target_fps=%u schema=1",
        unsigned(ok), fgOverlaySettingsPath.c_str(), GetFGSelectionName(GetFGUserMultiplier()), GetFGUserMultiplier(),
        GetFGDynamicManualTargetFps() ? "manual" : "auto", GetFGDynamicManualTargetFps());
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
    HDC dc = BeginPaint(hwnd, &ps);
    if (!dc) return;
    RECT client{};
    GetClientRect(hwnd, &client);
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
    RECT versionRc{510, 31, 602, 55};
    DrawTextW(dc, L"v1.0.0", -1, &versionRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    RECT keyBox{618, 25, 666, 57};
    SelectObject(dc, buttonSelectedFont);
    PaintFGButton(dc, keyBox, L"F10", true);
    PaintFGVerticalDivider(dc, 676, 27, 55);
    SelectObject(dc, bodyFont);
    SetTextColor(dc, RGB(232, 232, 232));
    RECT hideRc{687, 28, 730, 56};
    DrawTextW(dc, L"Hide", -1, &hideRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    PaintFGDivider(dc, left, 86, right);

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

    const bool dynamicControlsSupported = !IsFGRtx40Series();
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
    SelectObject(dc, smallFont);
    SetTextColor(dc, RGB(160, 160, 160));
    RECT footerRc{30, 556, 610, 580};
    DrawTextW(dc, L"Settings save automatically  |  changes apply on the next presented frame", -1, &footerRc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (GetTickCount64() < fgOverlaySettingsSavedPulseUntilMs) {
        SetTextColor(dc, RGB(237, 237, 237));
        RECT savedRc{626, 556, 708, 580};
        DrawTextW(dc, L"SAVED", -1, &savedRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(dc, oldFont);
    DeleteObject(metricFont); DeleteObject(bodyFont); DeleteObject(buttonFont); DeleteObject(buttonSelectedFont);
    DeleteObject(smallFont); DeleteObject(labelFont); DeleteObject(sectionFont);
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
static bool FGOverlayAutoTargetFromPoint(int x, int y) noexcept { return y >= 386 && y < 434 && x >= 30 && x < 142; }
static bool FGOverlayManualTargetFromPoint(int x, int y) noexcept { return y >= 386 && y < 434 && x >= 152 && x < 278; }
static bool FGOverlaySliderFromPoint(int x, int y) noexcept { return y >= 443 && y < 512 && x >= 30 && x < 710; }
static void FGOverlayApplySliderPoint(HWND hwnd, int x) noexcept {
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
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintFGOverlay(hwnd);
        return 0;
    case WM_LBUTTONDOWN: {
        const int x = static_cast<short>(LOWORD(lParam));
        const int y = static_cast<short>(HIWORD(lParam));
        const unsigned int selection = FGOverlayMultiplierFromPoint(x, y);
        if (selection != 0xFFFFFFFFu) {
            const unsigned int maxSupported = GetFGMaxSupportedMultiplier();
            const bool blockedGpuPolicy = !IsFGSelectionAllowedByGpuPolicy(selection);
            const bool blockedDynamic = selection == kSLSelectionDynamic &&
                                        IsFGDynamicCapabilityKnown() && !IsFGDynamicMFGSupported();
            const bool blockedFixed = selection >= 2 && maxSupported && selection > maxSupported;
            if (blockedGpuPolicy || blockedDynamic || blockedFixed) {
                const char* reason = blockedGpuPolicy ? "rtx40_off_plus_2x_only" :
                    (blockedDynamic ? "dynamic_unsupported" : "fixed_above_gpu_max");
                Log("FG_OVERLAY_SELECTION_BLOCKED selection=%s code=%u reason=%s max_supported=%u dynamic_known=%u dynamic_supported=%u rtx40_series=%u",
                    GetFGSelectionName(selection), selection, reason,
                    maxSupported, unsigned(IsFGDynamicCapabilityKnown()), unsigned(IsFGDynamicMFGSupported()), unsigned(IsFGRtx40Series()));
                return 0;
            }
            SetFGUserMultiplier(selection);
            FGOverlayMarkSettingsDirty();
            InvalidateRect(hwnd, nullptr, FALSE);
            Log("FG_OVERLAY_SELECTION selection=%s code=%u requested_generated=%u mode=%s persistence=dirty",
                GetFGSelectionName(selection), selection, selection >= 2 ? selection - 1 : 0,
                IsFGDynamicSelection(selection) ? "dynamic" : (selection ? "fixed" : "off"));
            return 0;
        }
        if (FGOverlayAutoTargetFromPoint(x, y)) {
            if (IsFGRtx40Series()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=rtx40_dynamic_unsupported action=auto");
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
            if (IsFGRtx40Series()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=rtx40_dynamic_unsupported action=manual");
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
            if (IsFGRtx40Series()) {
                Log("FG_OVERLAY_DYNAMIC_TARGET_BLOCKED reason=rtx40_dynamic_unsupported action=slider");
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
        if (fgOverlaySliderDragging && (wParam & MK_LBUTTON)) {
            FGOverlayApplySliderPoint(hwnd, static_cast<short>(LOWORD(lParam)));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (fgOverlaySliderDragging) {
            fgOverlaySliderDragging = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            FGOverlayFlushSettingsIfDue(true);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        fgOverlaySliderDragging = false;
        break;
    case WM_CLOSE:
        FGOverlayFlushSettingsIfDue(true);
        fgOverlayVisible.store(0, std::memory_order_release);
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

static void PositionFGOverlay(HWND overlay, HWND game) noexcept {
    if (!overlay || !game) return;
    RECT client{};
    if (!GetClientRect(game, &client) || client.right <= client.left || client.bottom <= client.top) return;
    POINT origin{0, 0};
    if (!ClientToScreen(game, &origin)) return;
    const int clientWidth = client.right - client.left;
    const int x = origin.x + ((clientWidth > kFGOverlayWidth + 36) ? (clientWidth - kFGOverlayWidth - 18) : 18);
    SetWindowPos(overlay, HWND_TOPMOST, x, origin.y + 18, kFGOverlayWidth, kFGOverlayHeight,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
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
        kFGOverlayClassName, L"Control FG", WS_POPUP, 0, 0, kFGOverlayWidth, kFGOverlayHeight,
        nullptr, nullptr, selfModule, nullptr);
    if (!overlay) {
        Log("FG_OVERLAY_CREATE_FAILED error=%lu", GetLastError());
        return 0;
    }
    SetLayeredWindowAttributes(overlay, 0, 248, LWA_ALPHA);
    ShowWindow(overlay, SW_HIDE);
    Log("FG_OVERLAY_READY hwnd=%p hotkey=F10 input=GetAsyncKeyState_nonexclusive default_selection=%s selector=off,dynamic,2x,3x,4x,5x,6x dynamic_auto_target=explicit_game_monitor_refresh dynamic_manual_target=30-1000_fps controls=auto,manual,thick_slider_30_1000 presentation=os_layered_noactivate_control_native_menu_anchor_top_right startup_visibility=hidden_f10_only persistence=localappdata_ini runtime_status=selected,effective,current_fps,hdr,capability title=embedded_control_fg_logo_control_native font=bahnschrift_semicondensed selected_style=white_fill_black_text section_headers=control_red no_side_scrollbar=1 dynamic_vsync_policy=syncinterval0_while_active dynamic_reflex_limiter=target_fps gpu_policy=rtx40_off_plus_2x_only",
        overlay, GetFGSelectionName(GetFGUserMultiplier()));
    SetTimer(overlay, kFGOverlayTimer, 50, nullptr);

    bool f10WasDown = false;
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_TIMER) {
            FGOverlayFlushSettingsIfDue(false);
            HWND game = FindFGGameWindow();
            if (game) FGOverlayRefreshDisplayTarget(game);
            if (IsFGRtx40Series()) {
                const unsigned int selected = GetFGUserMultiplier();
                if (selected != kSLSelectionOff && selected != 2u) {
                    SetFGUserMultiplier(2u);
                    FGOverlayMarkSettingsDirty();
                    Log("FG_OVERLAY_GPU_POLICY_COERCE gpu=rtx40 previous_selection=%s previous_code=%u selected_selection=2x selected_code=2 persistence=dirty",
                        GetFGSelectionName(selected), selected);
                }
            }
            const bool gameForeground = game && FGOverlayGameIsForeground(game);
            const bool f10Down = gameForeground && ((GetAsyncKeyState(VK_F10) & 0x8000) != 0);
            if (f10Down && !f10WasDown) {
                const unsigned int next = fgOverlayVisible.load(std::memory_order_acquire) ? 0u : 1u;
                fgOverlayVisible.store(next, std::memory_order_release);
                Log("FG_OVERLAY_TOGGLE visible=%u hotkey=F10 input=nonexclusive", next);
            }
            f10WasDown = f10Down;
            const bool show = gameForeground && fgOverlayVisible.load(std::memory_order_acquire);
            if (show) {
                PositionFGOverlay(overlay, game);
                InvalidateRect(overlay, nullptr, FALSE);
            } else {
                ShowWindow(overlay, SW_HIDE);
            }
        }
    }
    FGOverlayFlushSettingsIfDue(true);
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
