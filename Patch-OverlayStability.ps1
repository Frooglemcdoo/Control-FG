$ErrorActionPreference = 'Stop'

$overlayPath = Join-Path $PSScriptRoot 'src\fg_overlay.h'
if (-not (Test-Path -LiteralPath $overlayPath)) {
    throw "Missing overlay source: $overlayPath"
}

$text = [IO.File]::ReadAllText($overlayPath).Replace("`r`n", "`n")
if ($text.Contains('CONTROL_FG_OVERLAY_STABILITY_TEST_R1')) {
    Write-Host 'Overlay stability test R1 is already applied.'
    exit 0
}

function Replace-Exact {
    param(
        [Parameter(Mandatory=$true)][string]$Text,
        [Parameter(Mandatory=$true)][string]$Old,
        [Parameter(Mandatory=$true)][string]$New,
        [Parameter(Mandatory=$true)][string]$Name
    )
    if (-not $Text.Contains($Old)) {
        throw "Overlay stability patch failed; missing expected block: $Name"
    }
    return $Text.Replace($Old, $New)
}

$oldGlobals = @'
static HBITMAP fgOverlayLogoBitmap = nullptr;
static bool fgOverlayLogoLoadAttempted = false;
'@
$newGlobals = @'
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
'@
$text = Replace-Exact $text $oldGlobals $newGlobals 'overlay globals'

$oldFind = @'
static HWND FindFGGameWindow() noexcept {
    FGWindowSearch search{GetCurrentProcessId(), nullptr, 0};
    EnumWindows(FGOverlayEnumWindows, reinterpret_cast<LPARAM>(&search));
    return search.best;
}
'@
$newFind = @'
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
'@
$text = Replace-Exact $text $oldFind $newFind 'cached game-window lookup'

$oldPaintStart = @'
static void PaintFGOverlay(HWND hwnd) noexcept {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    if (!dc) return;
    RECT client{};
    GetClientRect(hwnd, &client);
    HBRUSH background = CreateSolidBrush(RGB(0, 0, 0));
'@
$newPaintStart = @'
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
'@
$text = Replace-Exact $text $oldPaintStart $newPaintStart 'double-buffered paint start'

$oldPaintEnd = @'
    SelectObject(dc, oldFont);
    DeleteObject(metricFont); DeleteObject(bodyFont); DeleteObject(buttonFont); DeleteObject(buttonSelectedFont);
    DeleteObject(smallFont); DeleteObject(labelFont); DeleteObject(sectionFont);
    EndPaint(hwnd, &ps);
}
'@
$newPaintEnd = @'
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
'@
$text = Replace-Exact $text $oldPaintEnd $newPaintEnd 'double-buffered paint finish'

$oldPosition = @'
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
'@
$newPosition = @'
static bool PositionFGOverlay(HWND overlay, HWND game, bool force = false) noexcept {
    if (!overlay || !game) return false;
    RECT client{};
    if (!GetClientRect(game, &client) || client.right <= client.left || client.bottom <= client.top) return false;
    POINT origin{0, 0};
    if (!ClientToScreen(game, &origin)) return false;
    const int clientWidth = client.right - client.left;
    const int x = origin.x + ((clientWidth > kFGOverlayWidth + 36) ? (clientWidth - kFGOverlayWidth - 18) : 18);
    RECT placement{x, origin.y + 18, x + kFGOverlayWidth, origin.y + 18 + kFGOverlayHeight};
    if (!force && fgOverlayLastPlacementValid && EqualRect(&placement, &fgOverlayLastPlacement)) return false;
    if (!SetWindowPos(overlay, HWND_TOPMOST, placement.left, placement.top, kFGOverlayWidth, kFGOverlayHeight,
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
'@
$text = Replace-Exact $text $oldPosition $newPosition 'position and visibility transition logic'

$oldTimer = @'
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
'@
$newTimer = @'
        if (msg.message == WM_TIMER) {
            const ULONGLONG now = GetTickCount64();
            FGOverlayFlushSettingsIfDue(false);
            HWND game = FindFGGameWindowCached(false);
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
'@
$text = Replace-Exact $text $oldTimer $newTimer 'timer cadence logic'

$oldTimerStart = @'
    SetTimer(overlay, kFGOverlayTimer, 50, nullptr);
'@
$newTimerStart = @'
    Log("FG_OVERLAY_STABILITY_TEST revision=R1 double_buffered=1 timer_ms=50 window_search_ms=1000 placement_poll_ms=250 status_paint_ms=250 setwindowpos_on_change_only=1 showhide_on_transition_only=1 fg_path_unchanged=v0.8.26");
    SetTimer(overlay, kFGOverlayTimer, 50, nullptr);
'@
$text = Replace-Exact $text $oldTimerStart $newTimerStart 'stability test runtime marker'

[IO.File]::WriteAllText($overlayPath, $text, [Text.UTF8Encoding]::new($false))
Write-Host 'Applied Control FG overlay stability test R1.'
Write-Host '  - double-buffered GDI paint'
Write-Host '  - game-window enumeration throttled to 1 Hz'
Write-Host '  - SetWindowPos only when placement actually changes'
Write-Host '  - ShowWindow only on visibility transitions'
Write-Host '  - passive status repaint capped at 4 Hz'
Write-Host '  - FG/HDR/Streamline generation path unchanged'
