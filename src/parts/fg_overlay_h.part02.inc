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
    SetTextColor(dc, dynamicControlsSupported ? RGB(239, 239, 239) : RGB(85, 85, 85));
    RECT targetRc{316, 386, 710, 434};
    DrawTextW(dc, targetSummary, -1, &targetRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    const RECT sliderBox{30, 456, 710, 500};
    const unsigned int sliderFps = manualTarget ? manualTarget : (detectedMilliHz ? static_cast<unsigned int>((detectedMilliHz + 500u) / 1000u) : kSLDynamicTargetFallbackFps);
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
                Log("FG_OVERLAY_SELECTION_BLOCKED selected=%s reason=%s max_supported=%u dynamic_known=%u dynamic_supported=%u gpu_policy=%s",
                    GetFGSelectionName(selection), blockedGpuPolicy ? "gpu_policy_rtx40_2x_only" : (blockedDynamic ? "dynamic_unsupported" : "above_gpu_max"),
                    maxSupported, IsFGDynamicCapabilityKnown() ? 1u : 0u, IsFGDynamicMFGSupported() ? 1u : 0u,
                    IsFGRtx40Series() ? "rtx40_2x_only" : "native");
                return 0;
            }
            SetFGUserMultiplier(selection);
            FGOverlayMarkSettingsDirty();
            Log("FG_OVERLAY_SELECTION source=mouse selected=%s selected_value=%u persistence=dirty",
                GetFGSelectionName(selection), selection);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!IsFGRtx40Series() && FGOverlayAutoTargetFromPoint(x, y)) {
            SetFGDynamicManualTargetFps(0);
            FGOverlayMarkSettingsDirty();
            Log("FG_OVERLAY_DYNAMIC_TARGET action=auto policy=%s detected_refresh_fps=%.3f resolved_target_fps=%.3f persistence=dirty",
                GetFGDynamicTargetPolicyName(), static_cast<double>(GetFGDynamicDetectedRefreshMilliHz()) / 1000.0,
                static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!IsFGRtx40Series() && FGOverlayManualTargetFromPoint(x, y)) {
            unsigned int fps = GetFGDynamicManualTargetFps();
            if (!fps) {
                const unsigned int mhz = GetFGDynamicDetectedRefreshMilliHz();
                fps = mhz ? static_cast<unsigned int>((mhz + 500u) / 1000u) : kSLDynamicTargetFallbackFps;
            }
            SetFGDynamicManualTargetFps(fps);
            FGOverlayMarkSettingsDirty();
            Log("FG_OVERLAY_DYNAMIC_TARGET action=manual policy=%s manual_target_fps=%u resolved_target_fps=%.3f persistence=dirty",
                GetFGDynamicTargetPolicyName(), GetFGDynamicManualTargetFps(), static_cast<double>(GetFGDynamicResolvedTargetFrameRate()));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!IsFGRtx40Series() && FGOverlaySliderFromPoint(x, y)) {
            fgOverlaySliderDragging.store(1, std::memory_order_release);
            SetCapture(hwnd);
            FGOverlayApplySliderPoint(hwnd, x);
            return 0;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (fgOverlaySliderDragging.load(std::memory_order_acquire) != 0 && (wParam & MK_LBUTTON) != 0) {
            FGOverlayApplySliderPoint(hwnd, static_cast<short>(LOWORD(lParam)));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (fgOverlaySliderDragging.exchange(0, std::memory_order_acq_rel) != 0) {
            ReleaseCapture();
            FGOverlayFlushSettingsIfDue(true);
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        fgOverlaySliderDragging.store(0, std::memory_order_release);
        break;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        fgOverlayVisible.store(0, std::memory_order_release);
        return 0;
    case WM_DESTROY:
        fgOverlayWindow = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
