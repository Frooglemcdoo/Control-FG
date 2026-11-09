#pragma once

// The native Graphics menu inherits Control's theme. These tokens apply only
// to our optional Win32 VRAM monitor; do not override game menu typography.
namespace control_ui::theme {
inline constexpr wchar_t kBodyFontFamily[] = L"Bahnschrift SemiCondensed";
inline constexpr wchar_t kHeadingFontFamily[] = L"Bahnschrift Condensed";
struct FontStyle {
    int height;
    int weight;
    const wchar_t* family;
};
inline constexpr FontStyle kHeadingFont{16, FW_BOLD, kHeadingFontFamily};
inline constexpr FontStyle kValueFont{22, FW_SEMIBOLD, kBodyFontFamily};
inline constexpr FontStyle kDetailFont{13, FW_NORMAL, kBodyFontFamily};

inline constexpr COLORREF kBackground = RGB(0, 0, 0);
inline constexpr COLORREF kBorder = RGB(92, 92, 92);
inline constexpr COLORREF kAccent = RGB(255, 32, 32);
inline constexpr COLORREF kText = RGB(246, 246, 246);
inline constexpr COLORREF kMutedText = RGB(185, 185, 185);

namespace vram {
inline constexpr int kWidth = 390;
inline constexpr int kHeight = 86;
inline constexpr int kMargin = 18;
inline constexpr BYTE kOpacity = 238;
inline constexpr RECT kLabelBounds{14, 8, 82, 30};
inline constexpr RECT kValueBounds{82, 5, 376, 38};
inline constexpr RECT kDetailBounds{14, 48, 376, 72};
}
inline HFONT CreateFontForStyle(const FontStyle& style) noexcept {
    return CreateFontW(-style.height, 0, 0, 0, style.weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, style.family);
}
}
