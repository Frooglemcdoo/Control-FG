from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[2];s=(r/'src/streamline_bridge.h').read_text();ui=(r/'src/fg_overlay.h').read_text()
a=s.index('static bool IsFGExperimentalMultiplier(');b=s.index('static void CacheFGGpuClassFromLuid(',a)
body=s[a:b]
preamble='''#include <atomic>
#include <cassert>
static bool ada=false,known=false,supported=false;
static std::atomic<unsigned> slMfgExperimentReady{0};
static constexpr unsigned kSLSelectionOff=0,kSLSelectionDynamic=1;
static bool IsFGRtx40Series(){return ada;}
static bool IsFGDynamicCapabilityKnown(){return known;}
static bool IsFGDynamicMFGSupported(){return supported;}
static unsigned NormalizeFGMultiplier(unsigned n){return n;}
'''
# Actual source constant is used; do not assume Dynamic's numeric enum.
import re
m=re.search(r'kSLSelectionDynamic\s*=\s*(\d+)',s);assert m
preamble=preamble.replace('kSLSelectionDynamic=1','kSLSelectionDynamic='+m.group(1))
checks='''
int main(){
for(unsigned a=0;a<2;++a)for(unsigned ready=0;ready<2;++ready)
for(unsigned k=0;k<2;++k)for(unsigned supp=0;supp<2;++supp){
 ada=a;slMfgExperimentReady=ready;known=k;supported=supp;
 const bool expected=!ada||(ready&&known&&supported);
 assert(IsFGDynamicAllowedByGpuPolicy()==expected);
 assert(IsFGSelectionAllowedByGpuPolicy(kSLSelectionDynamic)==expected);
 assert(IsFGSelectionAllowedByGpuPolicy(0));assert(IsFGSelectionAllowedByGpuPolicy(2));
 for(unsigned n=3;n<=6;++n)assert(IsFGSelectionAllowedByGpuPolicy(n)==(!ada||ready));
}}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(preamble+body+checks)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',t+'/test'],check=True)
 subprocess.run([t+'/test'],check=True)
assert 'const bool dynamicControlsSupported = IsFGDynamicAllowedByGpuPolicy();' in ui
assert ui.count('if (!IsFGDynamicAllowedByGpuPolicy()) {')==3
assert 'if (!IsFGSelectionAllowedByGpuPolicy(selected)) {' in ui
assert 'if (!EnsureDLSSGDynamicCapability(present)) enable = false;' in s
assert 'dynamicRequested ? 2u : 1u' in s
assert 'dynamicRequested ? sl::DLSSGMode::eDynamic : sl::DLSSGMode::eOn' in s
print('PASS actual policy: 16 adapter/unlock/capability combinations; fixed-mode regression; all target controls and native Dynamic request guard')
