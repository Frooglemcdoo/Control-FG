#pragma once
#include <mutex>
#include "rr_user_control.h"
#include "pt_reflection_user_control.h"
#include "rr_gi_sample_control.h"

// Native-menu settings persistence, display refresh discovery and optional
// read-only VRAM monitor. No settings window, shortcut polling or input hooks.
static constexpr wchar_t kFGVramProbeClassName[] = L"ControlFGVramLifecycle_R1";
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

#include "settings_catalog.h"
#include "ui/theme.h"

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

static unsigned FGSettingsReadStored(control_settings::Key key) noexcept {
    const auto& option = control_settings::Definition(key);
    const unsigned fallback = key == control_settings::Key::DynamicTargetFps ? 0 :
        control_settings::Default(option);
    return GetPrivateProfileIntW(option.section, option.storageKey, fallback, fgSettingsPath.c_str());
}

static void FGSettingsLoadSettings() noexcept {
    unsigned int expected = 0;
    if (!fgSettingsLoaded.compare_exchange_strong(expected, 1u)) return;
    if (!FGSettingsResolveSettingsPath()) {
        SetFGDynamicManualTargetFps(0);
        control_rr::RRUserSetSharpnessPercent(0);
        control_rr::RRUserSetSharpnessOverrideEnabled(false);
        control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
        control_rr::RRUserMarkPresetRestartRequired(false);
        control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
        control_rr::RRUserRequest(false);
        SetFGTextureStreamingBudgetMs(0u);
        SetFGVramMonitorEnabled(true);
        Log("FG_SETTINGS_LOAD success=0 reason=path_unavailable defaults=4x,dynamic_auto,rr_off,rr_model_F,texture_streaming_off,vram_monitor_on ui=fg_full_rr_toggle_model_E_F");
        return;
    }

    const unsigned int rawSelection = FGSettingsReadStored(control_settings::Key::FrameGeneration);
    const unsigned int selection = control_settings::RestoreFG(rawSelection);
    const unsigned int rawTarget = FGSettingsReadStored(control_settings::Key::DynamicTargetFps);
    const unsigned int manualTarget = ClampFGDynamicManualTargetFps(rawTarget);
    control_rr::GISampleSelect(FGSettingsReadStored(control_settings::Key::GlobalIlluminationSamples));
    const bool rrEnabled = FGSettingsReadStored(control_settings::Key::RayReconstruction) != 0;
    const unsigned int rawTextureStreamingMs = FGSettingsReadStored(control_settings::Key::TextureStreamingBudget);
    const unsigned int textureStreamingMs = NormalizeFGTextureStreamingBudgetMs(rawTextureStreamingMs);
    const bool vramMonitorEnabled = FGSettingsReadStored(control_settings::Key::VideoMemoryMonitor) != 0;

    slFgUserMultiplier.store(selection, std::memory_order_release);
    slFgDynamicManualTargetFps.store(manualTarget, std::memory_order_release);
    control_rr::RRUserSetSharpnessPercent(0);
    control_rr::RRUserSetSharpnessOverrideEnabled(false);
    control_rr::RRUserSetSkinMode(control_rr::RRSkinMode::Off);
    control_rr::RRUserMarkPresetRestartRequired(false);
    control_rr::RRUserSetSpecularSignalMode(control_rr::RRSpecularSignalMode::NativeClamp);
    control_rr::RRUserRequest(rrEnabled);
    SetFGTextureStreamingBudgetMs(textureStreamingMs);
    SetFGVramMonitorEnabled(vramMonitorEnabled);
    PTReflectionUserRequest(FGSettingsReadStored(control_settings::Key::GlassReflections) != 0);
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

    bool ok = WritePrivateProfileStringW(L"ControlFG", L"Schema", L"10", fgSettingsPath.c_str()) != FALSE;
    for (const auto& option : control_settings::kOptions) {
        if (!option.section) continue; // Derived controls do not own an INI key.
        wchar_t text[16]{};
        swprintf_s(text, L"%u", control_settings::ReadStored(option));
        const bool saved = WritePrivateProfileStringW(option.section, option.storageKey,
            text, fgSettingsPath.c_str()) != FALSE;
        ok = saved && ok;
    }
    WritePrivateProfileStringW(L"Overlay", L"ToggleKey", nullptr, fgSettingsPath.c_str());
    // Old experimental RR/AA settings are intentionally removed; FG settings remain intact.
    WritePrivateProfileStringW(L"AntiAliasing", nullptr, nullptr, fgSettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinResponsivity", nullptr, fgSettingsPath.c_str());
    WritePrivateProfileStringW(L"RayReconstruction", L"SkinPreserveNative", nullptr, fgSettingsPath.c_str());
    if (ok) ok = WritePrivateProfileStringW(nullptr, nullptr, nullptr, fgSettingsPath.c_str()) != FALSE;

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

#include "ui/vram_monitor.h"

// An owned, non-topmost monitor stays with the game without competing with
// NVIDIA/Steam overlays. It never takes focus, captures input or polls shortcuts.
static DWORD WINAPI FGSettingsThreadProc(LPVOID) noexcept {
    namespace layout = control_ui::theme::vram;
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = FGVramProbeWndProc;
    windowClass.hInstance = selfModule;
    windowClass.lpszClassName = kFGVramProbeClassName;
    RegisterClassExW(&windowClass);

    const UINT_PTR timer = SetTimer(nullptr, 0, 250, nullptr);
    if (!timer) {
        Log("FG_SETTINGS_TIMER_FAILED error=%lu", GetLastError());
        return 0;
    }
    HWND monitor = nullptr, owner = nullptr;
    bool shown = false;
    RECT lastPlacement{};
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        if (message.message != WM_TIMER || message.wParam != timer) continue;

        FGSettingsFlushSettingsIfDue();
        HWND game = FindFGGameWindowCached();
        if (game) FGSettingsRefreshDisplayTarget(game);
        if (IsFGRtx40Series() && !IsFGSelectionAllowedByGpuPolicy(GetFGUserMultiplier())) {
            std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
            SetFGUserMultiplier(control_settings::RestoreFG(GetFGUserMultiplier()));
            FGSettingsMarkSettingsDirty();
        }
        const bool visible = game && FGSettingsGameIsForeground(game) && IsFGVramMonitorEnabled();
        if (monitor && owner != game) {
            DestroyWindow(monitor);
            monitor = nullptr;
            shown = false;
        }
        if (visible && !monitor) {
            monitor = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
                kFGVramProbeClassName, L"Control FG VRAM", WS_POPUP, 0, 0, layout::kWidth, layout::kHeight,
                game, nullptr, selfModule, nullptr);
            owner = game;
            lastPlacement = {};
            if (monitor) SetLayeredWindowAttributes(monitor, 0, layout::kOpacity, LWA_ALPHA);
        }
        if (!monitor) continue;
        if (visible) {
            POINT origin{layout::kMargin, layout::kMargin};
            if (ClientToScreen(game, &origin)) {
                RECT placement{origin.x, origin.y, origin.x + layout::kWidth, origin.y + layout::kHeight};
                if (!EqualRect(&lastPlacement, &placement)) {
                    SetWindowPos(monitor, nullptr, placement.left, placement.top, layout::kWidth, layout::kHeight,
                        SWP_NOACTIVATE | SWP_NOZORDER);
                    lastPlacement = placement;
                }
            }
            if (!shown) {
                ShowWindow(monitor, SW_SHOWNOACTIVATE);
                shown = true;
            }
            InvalidateRect(monitor, nullptr, FALSE);
        } else if (shown) {
            ShowWindow(monitor, SW_HIDE);
            shown = false;
        }
    }
    KillTimer(nullptr, timer);
    FGSettingsFlushSettingsIfDue(true);
    if (monitor) DestroyWindow(monitor);
    return 0;
}

static void StartFGSettingsService() noexcept {
    if (fgSettingsThreadHandle) return;
    FGSettingsLoadSettings();
    fgSettingsThreadHandle = CreateThread(nullptr, 0, FGSettingsThreadProc, nullptr, 0, nullptr);
    Log("FG_SETTINGS_SERVICE started=%u ui=native_menu_only input_hooks=none", unsigned(fgSettingsThreadHandle != nullptr));
}
