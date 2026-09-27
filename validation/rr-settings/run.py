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
namespace control_rr {
bool spec=true,active=false,diffuse=true,contact=false;
bool RRUserSpecularMotionRequested(){return spec;} bool RRUserSpecularMotionActive(){return active;}
void RRUserSetSpecularMotionRequested(bool v){spec=v;if(!v)active=false;}
bool RRUserDiffuseClampRenoDX(){return diffuse;} void RRUserSetDiffuseClampRenoDX(bool v){diffuse=v;}
bool RRUserContactShadowRenoDX(){return contact;} void RRUserSetContactShadowRenoDX(bool v){contact=v;}
}
bool fgOverlayRRSettingsPage=false,fgOverlayClampDragging=false,fgOverlaySliderDragging=false;
unsigned fgOverlayClampPreview=60;
int captured=0,saves=0,resizes=0,offset=-220;
#define FALSE 0
void SetCapture(int h){captured=h;} int GetCapture(){return captured;} void ReleaseCapture(){captured=0;}
void FGOverlayMarkSettingsDirty(){} void FGOverlayFlushSettingsIfDue(bool){++saves;}
template<class... A> void Log(const char*,A...){} void InvalidateRect(int,void*,int){} void FGOverlayResizeWindowForCurrentSelection(int){++resizes;}
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
control_rr::spec=true; control_rr::diffuse=true; control_rr::contact=false;
assert(click(600,400)==0&&!control_rr::spec); // GI27 specular-MV A/B.
assert(click(600,400)==0&&control_rr::spec);
assert(click(600,500)==0&&!control_rr::diffuse); // GI28 diffuse clamp RenoDX/OFF.
assert(click(600,500)==0&&control_rr::diffuse);
assert(click(600,607)==0&&control_rr::contact); // GI28 contact Current/RenoDX.
assert(click(600,607)==0&&!control_rr::contact);
assert(click(600,725)==0&&!fgOverlayRRSettingsPage);
}
}
'''
code=code.replace('#include <cassert>','#include <cassert>\n#include <initializer_list>')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: actual RR page click routing, GI27 specular-MV and GI28 diffuse/contact toggles, reset/save, drag preview, compact/dynamic entry, Back, range and mapping')
