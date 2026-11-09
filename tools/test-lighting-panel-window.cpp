#include "../src/ui/lighting_panel_window.h"
#include <cassert>

int main() {
    const wchar_t* name=L"ControlTestPanel";
    WNDCLASSW wc{};
    wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpfnWndProc=DefWindowProcW;
    wc.lpszClassName=name;
    assert(RegisterClassW(&wc));
    auto window=[&](HWND owner){
        HWND hwnd=CreateWindowExW(0,name,L"",WS_POPUP,0,0,32,32,owner,nullptr,wc.hInstance,nullptr);
        assert(hwnd);
        return hwnd;
    };
    HWND game=window(nullptr), other=window(nullptr), panel=window(game);
    assert(lighting_panel::Valid(panel,game,name));
    assert(!lighting_panel::Valid(panel,other,name));
    assert(lighting_panel::Foreground(game,panel,panel));
    assert(!lighting_panel::Foreground(game,panel,other));
    assert(!lighting_panel::Foreground(game,panel,nullptr));
    DestroyWindow(game);
    assert(!lighting_panel::Valid(panel,game,name));
    panel=window(other);
    assert(lighting_panel::Valid(panel,other,name));
    DestroyWindow(other);
}
