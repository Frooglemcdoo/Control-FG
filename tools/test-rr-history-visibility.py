"""Reproduce visibility-driven RR resets and guard the corrected call sites."""
from pathlib import Path
import re, shutil, subprocess, tempfile

root = Path(__file__).resolve().parents[1]
entry = (root/'src/rr_evaluation_entry.h').read_text()
assert 'worldHistory.Update(RRWorldAlbedoRequested())' in entry
assert 'viewAngleHistory.Update(RRWorldPerspectiveGuideRequested())' in entry
assert 'viewAngleHistory.Update(bindings.viewAngleApplied)' not in entry
assert 'worldHistory.Update(in.frame' not in entry
# Other reset reasons must remain active at the parameter binding boundary.
assert 'RRNativeSetGuides(parameters,&bindings,hitDistance,rrNativeFrame.Reset()||presetReset||distanceReset||clampReset||worldReset||viewAngleReset)' in entry
policy = re.search(r'struct RRWorldAlbedoHistory \{.*?\n\};', (root/'src/rr_world_albedo.h').read_text(), re.S).group()
code = '#include <cassert>\n#include <cstdio>\n#include "'+(root/'src/native_lighting_live_state.h').as_posix()+'"\n'+policy+r'''
int main() {
 using namespace native_lighting;
 assert(UsesWorldAlbedo(kNativeBlendDefaultMode));
 RRWorldAlbedoHistory oldCoverage, modes, perspective;
 const bool coverage[] = {true,true,false,true,false,false,true,true};
 unsigned oldResets=0, newResets=0;
 for (bool visible : coverage) {
  oldResets += oldCoverage.Update(visible);
  newResets += modes.Update(UsesWorldAlbedo(kNativeBlendDefaultMode));
  assert(!perspective.Update(UsesPerspectiveGuide(kNativeBlendDefaultMode)));
 }
 assert(oldResets==5 && newResets==1);
 // Actual guide option transitions still discard incompatible histories.
 assert(modes.Update(UsesWorldAlbedo(Mode::Native)));
 assert(!modes.Update(UsesWorldAlbedo(Mode::Native)));
 assert(modes.Update(UsesWorldAlbedo(kNativeBlendDefaultMode)));
 assert(perspective.Update(UsesPerspectiveGuide(Mode::RtSpotViewAngle)));
 for (bool visible : coverage) {
  (void)visible;
  assert(!perspective.Update(UsesPerspectiveGuide(Mode::RtSpotViewAngle)));
 }
 assert(perspective.Update(UsesPerspectiveGuide(kNativeBlendDefaultMode)));
 std::puts("PASS visibility regression: old 5 resets, corrected 1 initial reset; actual mode changes preserved");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp); cpp=tmp/'history.cpp'; cpp.write_text(code); exe=tmp/'history.exe'
    cmd=(['cl','/nologo','/std:c++17','/EHsc','/W4','/WX',str(cpp),'/Fe'+str(exe),'/Fo'+str(tmp/'history.obj')]
         if shutil.which('cl') else ['g++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)])
    subprocess.run(cmd,check=True); subprocess.run([str(exe)],check=True)
