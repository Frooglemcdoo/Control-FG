#pragma once
#include "theme.h"

// Drawing is intentionally independent of settings persistence and input.
static void PaintFGVramProbe(HWND window) noexcept {
    namespace theme = control_ui::theme;
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);
    if (!dc) return;

    RECT client{};
    GetClientRect(window, &client);
    HBRUSH background = CreateSolidBrush(theme::kBackground);
    FillRect(dc, &client, background);
    DeleteObject(background);

    HPEN border = CreatePen(PS_SOLID, 1, theme::kBorder);
    HGDIOBJ previousPen = SelectObject(dc, border);
    HGDIOBJ previousBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, 0, 0, client.right - 1, client.bottom - 1);
    SelectObject(dc, previousBrush);
    SelectObject(dc, previousPen);
    DeleteObject(border);
    SetBkMode(dc, TRANSPARENT);

    HFONT headingFont = theme::CreateFont(theme::kHeadingFont);
    HFONT valueFont = theme::CreateFont(theme::kValueFont);
    HFONT detailFont = theme::CreateFont(theme::kDetailFont);
    HGDIOBJ previousFont = SelectObject(dc, headingFont);
    SetTextColor(dc, theme::kAccent);
    RECT headingBounds = theme::vram::kLabelBounds;
    DrawTextW(dc, L"VRAM", -1, &headingBounds, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    wchar_t valueText[160]{}, detailText[192]{};
    if (vramProbeSampleValid.load(std::memory_order_acquire)) {
        constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
        const auto usage = vramProbeLocalUsageBytes.load(std::memory_order_acquire);
        const auto budget = vramProbeLocalBudgetBytes.load(std::memory_order_acquire);
        const auto peak = vramProbePeakLocalUsageBytes.load(std::memory_order_acquire);
        const auto physical = vramProbeDedicatedVideoBytes.load(std::memory_order_acquire);
        const unsigned percentage = budget ? static_cast<unsigned>((usage * 100ull + budget / 2ull) / budget) : 0;
        const bool hdrKnown = vramProbeHdrKnown.load(std::memory_order_acquire) != 0;
        const bool hdrEnabled = vramProbeHdrEnabled.load(std::memory_order_acquire) != 0;
        swprintf_s(valueText, L"%.2f / %.2f GB  (%u%%)", double(usage) / bytesPerGiB,
            double(budget) / bytesPerGiB, percentage);
        swprintf_s(detailText, L"Peak %.2f GB   Physical %.2f GB   HDR %s", double(peak) / bytesPerGiB,
            double(physical) / bytesPerGiB, hdrKnown ? (hdrEnabled ? L"ON" : L"OFF") : L"?");
    } else {
        wcscpy_s(valueText, L"Waiting for GPU sample...");
        wcscpy_s(detailText, L"Updates while Control is rendering.");
    }

    SelectObject(dc, valueFont);
    SetTextColor(dc, theme::kText);
    RECT valueBounds = theme::vram::kValueBounds;
    DrawTextW(dc, valueText, -1, &valueBounds, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, detailFont);
    SetTextColor(dc, theme::kMutedText);
    RECT detailBounds = theme::vram::kDetailBounds;
    DrawTextW(dc, detailText, -1, &detailBounds, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, previousFont);
    DeleteObject(headingFont);
    DeleteObject(valueFont);
    DeleteObject(detailFont);
    EndPaint(window, &paint);
}

static LRESULT CALLBACK FGVramProbeWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    switch (message) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: PaintFGVramProbe(window); return 0;
    case WM_NCHITTEST: return HTTRANSPARENT;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
