from pathlib import Path
import re, subprocess, tempfile, hashlib, json, os
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
assert a.index('if (!IsFGRtx40Series() || !IsRTX40MFGSessionEnabled()) return true;') < a.index('slMfgExperimentPrepare()')
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
#include <cwchar>
#include <vector>
#define __declspec(x)
#define __cdecl
using HMODULE=void*;using LPCWSTR=const wchar_t*;
using HANDLE=void*;using DWORD=unsigned long;using BOOL=int;
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
constexpr unsigned TH32CS_SNAPMODULE=8, ERROR_NO_MORE_FILES=18;
struct MODULEENTRY32W {unsigned dwSize;HMODULE hModule;wchar_t szExePath[260];};
static std::vector<HMODULE> modules{(void*)1};
static size_t moduleIndex=0;static bool snapshotOK=true,enumerationOK=true;
static HMODULE temporalProvider=nullptr,devicePolicyProvider=nullptr;
HANDLE CreateToolhelp32Snapshot(unsigned,unsigned){moduleIndex=0;return snapshotOK?(void*)9:INVALID_HANDLE_VALUE;}
unsigned GetCurrentProcessId(){return 1;}
BOOL Module32FirstW(HANDLE,MODULEENTRY32W* e){if(modules.empty())return 0;e->hModule=modules[0];std::wcscpy(e->szExePath,L"cache.bin");return 1;}
BOOL Module32NextW(HANDLE,MODULEENTRY32W* e){if(++moduleIndex>=modules.size())return 0;e->hModule=modules[moduleIndex];return 1;}
DWORD GetLastError(){return enumerationOK?ERROR_NO_MORE_FILES:5;}
void CloseHandle(HANDLE){}

constexpr unsigned LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR=1,LOAD_LIBRARY_SEARCH_SYSTEM32=2;
constexpr unsigned GET_MODULE_HANDLE_EX_FLAG_PIN=4,GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS=8;
static bool ada=true,temporal=true,valid=true,published=false;
static unsigned mutate=0,loads=0,patchcalls=0;
static bool archOK=true,flipOK=true,maxOK=true;
HMODULE LoadLibraryExW(const wchar_t*,void*,unsigned){++loads;return (void*)1;}
HMODULE GetModuleHandleW(const wchar_t*){return (void*)2;}
bool GetModuleHandleExW(unsigned,LPCWSTR address,HMODULE* out){*out=(void*)address;return true;}
namespace mfglog {void Open(const wchar_t*){}void Write(const wchar_t*,...){}void WriteMessage(const wchar_t*){}}
namespace ada_patch {
void SetLogCallback(void(*)(const wchar_t*)){}
bool ObserveD3D12Device(void*){return ada;}
bool Ready(){return published;}
bool PatchProvider(HMODULE module,const wchar_t*){temporalProvider=module;++patchcalls;published=temporal;return temporal;}
unsigned FailureCode(){return 0;}const wchar_t* FailureName(unsigned){return L"mock";}
}
namespace provider_policy {
struct VersionTriplet{unsigned major=0,minor=0,build=0;};
bool IsDlssgImplementationModule(HMODULE module){return module!=(void*)8;}
bool IsSupportedProvider(HMODULE,const wchar_t*){return valid;}
bool ReadProviderVersion(const wchar_t*,VersionTriplet& v){v={310,9,1};return true;}
}
namespace patches {
struct ArchGateResult{size_t found,patched;};
struct FlipMeterResult{bool located,derived;unsigned offset,value;size_t sites;};
struct Result{bool candidate,patched;unsigned compiledMaximum;};
Result PatchNgxDeviceSupport(HMODULE module,const wchar_t*){devicePolicyProvider=module;++mutate;return {true,true,5};}
ArchGateResult PatchArchGates(HMODULE,const wchar_t*){++mutate;return {2,archOK?2u:1u};}
FlipMeterResult PatchFlipMetering(HMODULE,const wchar_t*){++mutate;return {true,true,0,0,flipOK?2u:0u};}
Result PatchStreamlineMaximum(HMODULE,const wchar_t*){++mutate;return {true,maxOK,5};}
}
'''
checks=r'''
void reset(){providerResolved=false;modules={(void*)1};snapshotOK=enumerationOK=true;temporalProvider=devicePolicyProvider=nullptr;provider=nullptr;providerPath.clear();started=prepared=failed=false;attempts=0;
ada=temporal=valid=archOK=flipOK=maxOK=true;published=false;mutate=loads=patchcalls=0;}
int main(){
reset();ada=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!mutate&&!loads);assert(!ControlFGMFGPrepare());
reset();valid=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!mutate);
reset();archOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare()&&!patchcalls);
reset();flipOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare());
reset();maxOK=false;assert(!ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare());
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(mutate==4);
assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(mutate==4);
temporal=false;assert(!ControlFGMFGPrepare());temporal=true;assert(ControlFGMFGPrepare());assert(ControlFGMFGPrepare());assert(patchcalls==3);
temporal=false;assert(!ControlFGMFGPrepare());
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));temporal=false;
for(unsigned i=0;i<121;++i) assert(!ControlFGMFGPrepare());assert(patchcalls==120);temporal=true;assert(!ControlFGMFGPrepare());
// Cached DLSS-G implementation is patched and receives temporal correction;
// unrelated SR/RR modules are excluded even when their cache names match.
reset();modules={(void*)1,(void*)8,(void*)4};assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));
assert(ControlFGMFGPrepare());assert(provider==(void*)4);assert(temporalProvider==(void*)4);assert(devicePolicyProvider==(void*)4);
const auto mutations=mutate;assert(ControlFGMFGPrepare());assert(mutate==mutations);
reset();modules={(void*)1,(void*)4,(void*)5};assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));assert(!ControlFGMFGPrepare());assert(!patchcalls);
reset();modules={(void*)1,(void*)4};assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));valid=false;assert(!ControlFGMFGPrepare());assert(!patchcalls);
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));snapshotOK=false;assert(!ControlFGMFGPrepare());assert(!patchcalls);
reset();assert(ControlFGMFGInitialize((void*)3,L"runtime",L"logs"));enumerationOK=false;assert(!ControlFGMFGPrepare());assert(!patchcalls);

}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.cpp';p.write_text(preamble+body+checks)
 executable=td+('/test.exe' if os.name=='nt' else '/test')
 command=['cl','/nologo','/std:c++20','/EHsc',str(p),'/Fe'+executable,'/Fo'+td+'/test.obj'] if os.name=='nt' else ['g++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(p),'-o',executable]
 subprocess.run(command,check=True)
 subprocess.run([executable],check=True)
print('PASS: production bridge mocked orchestration; non-Ada/version rejection, partial patch failure, pending temporal, idempotence, descriptor recheck, retry limit; source isolation and bind order')
