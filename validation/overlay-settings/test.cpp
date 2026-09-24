#include <atomic>
#include <cassert>
#include <cwchar>
#define FALSE 0
#define VK_F1 112
#define VK_F9 120
#define VK_F10 121
#define VK_F24 135
#define VK_NUMPAD0 96
#define VK_DIVIDE 111
#define VK_INSERT 45
#define VK_DELETE 46
#define VK_HOME 36
#define VK_END 35
#define VK_PRIOR 33
#define VK_NEXT 34
#define VK_SPACE 32
#define VK_TAB 9
#define VK_RETURN 13
#define VK_BACK 8
#define VK_ESCAPE 27
#define VK_CONTROL 17
#define VK_MENU 18
#define VK_SHIFT 16
#define VK_LWIN 91
#define VK_RWIN 92

bool down[256]{};
short GetAsyncKeyState(int key) { return down[key] ? static_cast<short>(0x8000) : 0; }
bool fgOverlayBindingCapture=false,fgOverlayBindingKeysDown[256]{},fgOverlayToggleWasDown=false,fgOverlaySettingsDirty=false;
unsigned int fgOverlayToggleKey=VK_F10;
const wchar_t* fgOverlayBindingMessage=L"";
std::atomic<unsigned int> fgOverlayVisible{1};
void FGOverlayMarkSettingsDirty(){fgOverlaySettingsDirty=true;}
void FGOverlayFlushSettingsIfDue(bool){fgOverlaySettingsDirty=false;}
void Log(const char*,...){ }
void InvalidateRect(int,void*,int){ }
static bool FGOverlayBindingKeyAllowed(unsigned int key) noexcept {
    return (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') ||
        (key >= VK_F1 && key <= VK_F24 && key != VK_F9) ||
        (key >= VK_NUMPAD0 && key <= VK_DIVIDE) ||
        key == VK_INSERT || key == VK_DELETE || key == VK_HOME || key == VK_END ||
        key == VK_PRIOR || key == VK_NEXT || key == VK_SPACE || key == VK_TAB ||
        key == VK_RETURN || key == VK_BACK;
}


void tick(bool gameForeground) { int overlay=0;
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
            const bool toggleDown = (GetAsyncKeyState(static_cast<int>(fgOverlayToggleKey)) & 0x8000) != 0;
            if (!capturingBinding && gameForeground && toggleDown && !fgOverlayToggleWasDown) {
                const unsigned int next = fgOverlayVisible.load(std::memory_order_acquire) ? 0u : 1u;
                fgOverlayVisible.store(next, std::memory_order_release);
                Log("FG_OVERLAY_TOGGLE visible=%u key=%u input=nonexclusive", next, fgOverlayToggleKey);
            }
            fgOverlayToggleWasDown = toggleDown;

}
void reset() { for(int i=0;i<256;++i) {down[i]=false;fgOverlayBindingKeysDown[i]=false;} fgOverlayBindingCapture=false;fgOverlayToggleWasDown=false;fgOverlayToggleKey=VK_F10;fgOverlayVisible=1; }
int main(){
assert(FGOverlayBindingKeyAllowed(VK_F10));assert(FGOverlayBindingKeyAllowed('K'));
assert(!FGOverlayBindingKeyAllowed(VK_F9));assert(!FGOverlayBindingKeyAllowed(VK_ESCAPE));assert(!FGOverlayBindingKeyAllowed(0));assert(!FGOverlayBindingKeyAllowed(999));
reset();fgOverlayBindingCapture=true;down['K']=true;tick(true);assert(fgOverlayToggleKey=='K'&&!fgOverlayBindingCapture&&fgOverlayVisible==1);
tick(true);assert(fgOverlayVisible==1);down['K']=false;tick(true);down['K']=true;tick(true);assert(fgOverlayVisible==0);tick(true);assert(fgOverlayVisible==0);
reset();fgOverlayBindingCapture=true;down[VK_ESCAPE]=true;tick(true);assert(!fgOverlayBindingCapture&&fgOverlayToggleKey==VK_F10&&fgOverlayVisible==1);
reset();fgOverlayBindingCapture=true;down[VK_F9]=true;tick(true);assert(fgOverlayBindingCapture&&fgOverlayToggleKey==VK_F10);
reset();fgOverlayBindingCapture=true;down[VK_CONTROL]=down['K']=true;tick(true);assert(fgOverlayBindingCapture&&fgOverlayToggleKey==VK_F10);
reset();fgOverlayBindingCapture=true;tick(false);assert(!fgOverlayBindingCapture&&fgOverlayVisible==1);
reset();down[VK_F10]=true;tick(false);tick(true);assert(fgOverlayVisible==1);
reset();fgOverlayBindingCapture=true;down['K']=fgOverlayBindingKeysDown['K']=true;tick(true);assert(fgOverlayBindingCapture);down['K']=false;tick(true);down['K']=true;tick(true);assert(fgOverlayToggleKey=='K');
}
