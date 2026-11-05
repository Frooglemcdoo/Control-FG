#pragma once
#include <windows.h>

namespace lighting_panel {
inline bool Valid(HWND panel, HWND game, const wchar_t* expectedClass) noexcept {
    if (!game || !IsWindow(panel) || GetWindowThreadProcessId(panel, nullptr) != GetCurrentThreadId() ||
        GetWindow(panel, GW_OWNER) != game) return false;
    wchar_t name[96]{};
    return GetClassNameW(panel, name, 96) && lstrcmpW(name, expectedClass) == 0;
}

inline bool Foreground(HWND game, HWND panel, HWND foreground) noexcept {
    if (!game || !foreground) return false;
    return foreground == game || foreground == panel ||
           GetAncestor(foreground, GA_ROOTOWNER) == game;
}
}
