from pathlib import Path
import re, subprocess, tempfile, hashlib, json
root=Path(__file__).resolve().parents[2]
s=(root/'src/streamline_bridge.h').read_text()
b=(root/'src/rtx40_mfg/control_bridge.cpp').read_text()
assert s.count('InitializeRTX40MFGTest(baseDevice);')==2
for fn in ['TryConfigureSLNativeDeviceEarly','TryConfigureSLNativeDeviceDeferred']:
 part=s[s.index('static void '+fn):];part=part[:part.index('\n}\n')]
 assert part.index('CacheFGGpuClassFromLuid(luid)') < part.index('InitializeRTX40MFGTest(baseDevice)') < part.index('slSetD3DDeviceApi(baseDevice)')
a=s[s.index('static void InitializeRTX40MFGTest'):s.index('static bool PrepareRTX40MFGTest')]
assert a.index('if (!IsFGRtx40Series()) return;') < a.index('LoadLibraryExW')
a=s[s.index('static bool PrepareRTX40MFGTest'):s.index('// Observed frame counts')]
assert a.index('if (!IsFGRtx40Series()) return true;') < a.index('slMfgExperimentPrepare()')
a=s[s.rindex('static void SetDLSSGModeForPresent'):]
assert a.index('PrepareRTX40MFGTest()') < a.index('EnsureDLSSGMFGCapability') < a.index('slDLSSGSetOptionsApi(')
assert 'selected != kSLSelectionOff && experimentReady' in a
assert 'else if (!EnsureDLSSGMFGCapability(present, requestedGenerated))' in a
assert 'slMfgMaxGenerated.store(5' not in s
assert 'LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32' in s
assert 'MH_' not in b and 'CreateFeature' not in b
# Compile the unmodified bridge body with mock platform/provider declarations.
# This exercises production orchestration, not a parallel policy implementation.
body='\n'.join(line for line in b.splitlines() if not line.startswith('#include'))
preamble=r'''
#include <mutex>
#include <string>
#include <cassert>
#include <cstdint>
#define __declspec(x)
#define __cdecl
using HMODULE=void*;using LPCWSTR=const wchar_t*;
constexpr unsigned LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR=1,LOAD_LIBRARY_SEARCH_SYSTEM32=2;
constexpr unsigned GET_MODULE_HANDLE_EX_FLAG_PIN=4,GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS=8;
static bool ada=true,temporal=true,valid=true,published=false;
static unsigned mutate=0,loads=0,patchcalls=0;
static bool archOK=true,flipOK=true,maxOK=true;
HMODULE LoadLibraryExW(const wchar_t*,void*,unsigned){++loads;return (void*)1;}
HMODULE GetModuleHandleW(const wchar_t*){return (void*)2;}
bool GetModuleHandleExW(unsigned,LPCWSTR,HMODULE* out){*out=(void*)2;return true;}
namespace mfglog {void Open(const wchar_t*){}void Write(const wchar_t*,...){}void WriteMessage(const wchar_t*){}}
namespace ada_patch {
void SetLogCallback(void(*)(const wchar_t*)){}
bool ObserveD3D12Device(void*){return ada;}
bool Ready(){return published;}
bool PatchProvider(HMODULE,const wchar_t*){++patchcalls;published=temporal;return temporal;}
unsigned FailureCode(){return 0;}const wchar_t* FailureName(unsigned){return L"mock";}
}
namespace provider_policy {
struct VersionTriplet{unsigned major=0,minor=0,build=0;};
bool IsSupportedProvider(HMODULE,const wchar_t*){return valid;}
bool ReadProviderVersion(const wchar_t*,VersionTriplet& v){v={310,9,1};return true;}
}
namespace patches {
struct ArchGateResult{size_t found,patched;};
struct FlipMeterResult{bool located,derived;unsigned offset,value;size_t sites;};
struct Result{bool candidate,patched;unsigned compiledMaximum;};
ArchGateResult PatchArchGates(HMODULE,const wchar_t*){++mutate;return {2,archOK?2u:1u};}
FlipMeterResult PatchFlipMetering(HMODULE,const wchar_t*){++mutate;return {true,true,0,0,flipOK?2u:0u};}
Result PatchStreamlineMaximum(HMODULE,const wchar_t*){++mutate;return {true,maxOK,5};}
}
'''
checks=r'''
void reset(){provider=nullptr;providerPath.clear();started=prepared=failed=false;attempts=0;
ada=temporal=valid=archOK=flipOK=maxOK=true;published=false;mutate=loads=patchcalls=0;}
int main(){
reset();ada=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!mutate&&!loads);assert(!ControlFGMFGPrepare());
reset();valid=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!mutate);
reset();archOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare()&&!patchcalls);
reset();flipOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare());
reset();maxOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare());
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(mutate==3);
assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(mutate==3);
temporal=false;assert(!ControlFGMFGPrepare());temporal=true;assert(ControlFGMFGPrepare());assert(ControlFGMFGPrepare());assert(patchcalls==3);
temporal=false;assert(!ControlFGMFGPrepare());
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));temporal=false;
for(unsigned i=0;i<121;++i) assert(!ControlFGMFGPrepare());assert(patchcalls==120);temporal=true;assert(!ControlFGMFGPrepare());
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.cpp';p.write_text(preamble+body+checks)
 subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(p),'-o',td+'/test'],check=True)
 subprocess.run([td+'/test'],check=True)
print('PASS: production bridge mocked orchestration; non-Ada/version rejection, partial patch failure, pending temporal, idempotence, descriptor recheck, retry limit; source isolation and bind order')
