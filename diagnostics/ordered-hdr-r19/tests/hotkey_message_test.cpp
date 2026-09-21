#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include "input_policy.h"
static_assert(WM_HOTKEY==0x0312u,"dispatch contract changed");
int main(){
 MSG msg{};PeekMessageW(&msg,nullptr,WM_USER,WM_USER,PM_NOREMOVE);
 ATOM id=GlobalAddAtomW(L"ControlFG.OrderedHDR.R19.CI.MessageTest");if(!id)return 1;
 if(!RegisterHotKey(nullptr,id,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F11)){std::printf("RegisterHotKey error %lu\n",GetLastError());GlobalDeleteAtom(id);return 2;}
 if(!PostThreadMessageW(GetCurrentThreadId(),WM_HOTKEY,id,MAKELPARAM(MOD_CONTROL|MOD_ALT,VK_F11))){UnregisterHotKey(nullptr,id);GlobalDeleteAtom(id);return 3;}
 const bool found=PeekMessageW(&msg,nullptr,WM_HOTKEY,WM_HOTKEY,PM_REMOVE)!=FALSE;
 const bool dispatched=found && hdrguard::isControllerMessage(msg.message,msg.wParam,id);
 UnregisterHotKey(nullptr,id);GlobalDeleteAtom(id);
 if(!dispatched)return 4;
 std::puts("PASS CtrlAltF11 registration, queued WM_HOTKEY dispatch and cleanup; physical keyboard and HDR were not exercised");return 0;
}
