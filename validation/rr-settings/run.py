"""Exercise the actual RR panel input branch with mocked platform/persistence calls."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'src/fg_overlay.h').read_text()
helpers=s[s.index('static unsigned FGOverlayClampFromX'):s.index('static bool fgOverlayBindingCapture')]
a=s.index('        if (fgOverlayRRSettingsPage) {',s.index('case WM_LBUTTONDOWN'))
b=s.index('        if(FGOverlayRRToggleFromPoint',a)
branch=s[a:b]
code='''#include <cassert>
namespace control_rr_clamp { unsigned value=60; unsigned Normalize(unsigned v){return v>=25&&v<=75?v:60;} void Select(unsigned v){value=v;} }
bool fgOverlayRRSettingsPage=false,fgOverlayClampDragging=false,fgOverlaySliderDragging=false;
unsigned fgOverlayClampPreview=60;
int captured=0,saves=0,resizes=0,offset=-220;
#define FALSE 0
void SetCapture(int h){captured=h;} int GetCapture(){return captured;} void ReleaseCapture(){captured=0;}
void FGOverlayMarkSettingsDirty(){} void FGOverlayFlushSettingsIfDue(bool){++saves;}
void Log(const char*){} void InvalidateRect(int,void*,int){} void FGOverlayResizeWindowForCurrentSelection(int){++resizes;}
int FGOverlayLowerSectionOffset(){return offset;}
'''+helpers+'\nint click(int x,int y){int hwnd=1;\n'+branch+'''return 1;}
int main(){
for(unsigned v=25;v<=75;++v)assert(FGOverlayClampFromX(FGOverlayClampToX(v))==v);
assert(FGOverlayClampFromX(-32768)==25&&FGOverlayClampFromX(32767)==75);
for(int x=31;x<501;++x)assert(FGOverlayClampFromX(x)>=FGOverlayClampFromX(x-1));
for(int off : {-220,0}) { offset=off; fgOverlayRRSettingsPage=false;
assert(click(300,680+off)==0&&fgOverlayRRSettingsPage);
control_rr_clamp::value=75; int before=saves;
assert(click(600,245)==0&&control_rr_clamp::value==60&&saves==before+1);
assert(click(30,245)==0&&fgOverlayClampDragging&&fgOverlayClampPreview==25);
assert(control_rr_clamp::value==60); fgOverlayClampDragging=false; ReleaseCapture();
assert(click(500,245)==0&&fgOverlayClampPreview==75);fgOverlayClampDragging=false;ReleaseCapture();
assert(click(100,440)==0); // Hidden main-page RR button cannot toggle.
assert(click(600,435)==0&&!fgOverlayRRSettingsPage);
}
}
'''
code=code.replace('#include <cassert>','#include <cassert>\n#include <initializer_list>')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: actual RR page click routing, reset/save, drag preview, compact/dynamic entry, Back, range and mapping')
