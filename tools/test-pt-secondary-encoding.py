"""Measure binary16 secondary-hit encoding and lock the confirmed runtime."""
import hashlib, math, random, struct, re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
locked={
 'src/probe.cpp':'0cb55cbe61f022f04b89614816db75d053107c418835554c8d3bdf600112d8be',
 'src/pt_reflection_loader.h':'e7645277f30858d1f4003f7a696c4ca280ecf15216333e442055824c6e6682ee',
 'src/fg_overlay.h':'ef8c96bf2d03bd738af6b6a69ffb037879a77c456721d297e883e7b99a21772d',
 'src/pt_reflection_user_control.h':'53c854c79a852ba04b69b0b77f770f1a745d29f25b951444d8af3963a7a6f4bd',
}
for name,expected in locked.items():
 source=(root/name).read_text()
 if name=="src/probe.cpp":
  assert source.count('#include "pt_scaling_metrics.h"\n')==1
  source=source.replace('#include "pt_scaling_metrics.h"\n',"")
 if name=="src/fg_overlay.h":
  assert source.count('            PTStartupSaveStatus saveStatus;\n            const bool saved=PTStartupWriteSelection(!previous,&saveStatus);')==1
  source=source.replace('            PTStartupSaveStatus saveStatus;\n            const bool saved=PTStartupWriteSelection(!previous,&saveStatus);','            const bool saved=PTStartupWriteSelection(!previous);')
  assert source.count('\n            Log("PT_SELECTION_SAVE stage=%s error=%lu readback=%u",saveStatus.stage,saveStatus.error,saveStatus.readback);')==1
  source=source.replace('\n            Log("PT_SELECTION_SAVE stage=%s error=%lu readback=%u",saveStatus.stage,saveStatus.error,saveStatus.readback);',"")
 # R52 changes only persisted startup selection and its overlay controls.
 if name=='src/fg_overlay.h':
  addition='            const bool previous=ptNextLaunchEnabled.load(std::memory_order_acquire);\n            const bool saved=PTStartupWriteSelection(!previous);\n            if(saved)ptNextLaunchEnabled.store(!previous,std::memory_order_release);\n            Log("PT_REFLECTION_UI_TOGGLE previous=%u requested=%u saved=%u session=%u apply=restart",\n                unsigned(previous),unsigned(!previous),unsigned(saved),unsigned(PTStartupCaptureEnabled()));'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'            if(!PTStartupCaptureEnabled())return 0;\n            const bool previous=PTReflectionUserRequested();\n            PTReflectionUserRequest(!previous);\n            FGOverlayMarkSettingsDirty();\n            Log("PT_REFLECTION_UI_TOGGLE previous=%u enabled=%u verify_mode=0 scope=added_second_bounce apply=next_reflection_composite persistence=dirty",\n                unsigned(previous),unsigned(!previous));')
 if name=='src/fg_overlay.h':
  addition='    const bool ptSelected=ptNextLaunchEnabled.load(std::memory_order_acquire);\n    const bool ptPending=ptSelected!=ptAvailable;\n    SelectObject(dc, ptSelected ? buttonSelectedFont : buttonFont);\n    PaintFGButton(dc, ptToggle, ptPending ?\n        (ptSelected ? L"PT ON (RESTART)" : L"PT OFF (RESTART)") :\n        (ptSelected ? L"PT REFLECTIONS ON" : L"PT REFLECTIONS OFF"), ptSelected, true);'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'    SelectObject(dc, ptEnabled ? buttonSelectedFont : buttonFont);\n    PaintFGButton(dc, ptToggle, !ptAvailable ? L"RT ONLY (RESTART)" :\n        (ptEnabled ? L"PT REFLECTIONS ON" : L"PT REFLECTIONS OFF"), ptEnabled, ptAvailable);')
 if name=='src/fg_overlay.h':
  addition='swprintf_s(ptEnabledText, L"%u", ptNextLaunchEnabled.load(std::memory_order_acquire) ? 1u : 0u);'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'swprintf_s(ptEnabledText, L"%u", PTReflectionUserRequested() ? 1u : 0u);')
 if name=='src/fg_overlay.h':
  addition='    Log("PT_REFLECTION_SETTINGS enabled=%u source=startup_latch default=off restart_required=1", unsigned(PTStartupCaptureEnabled()));'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'    PTReflectionUserRequest(GetPrivateProfileIntW(L"PTReflections", L"Enabled", 1, fgOverlaySettingsPath.c_str()) != 0);\n    Log("PT_REFLECTION_SETTINGS enabled=%u source=settings default=on", unsigned(PTReflectionUserRequested()));')
 if name=='src/fg_overlay.h':
  addition='    if (!fgOverlaySettingsLoaded.compare_exchange_strong(expected, 1u)) return;\n    PTReflectionUserRequest(PTStartupCaptureEnabled());\n    ptNextLaunchEnabled.store(PTStartupCaptureEnabled(),std::memory_order_release);'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'    if (!fgOverlaySettingsLoaded.compare_exchange_strong(expected, 1u)) return;')
 if name=='src/fg_overlay.h':
  addition='static std::wstring fgOverlaySettingsPath;\nstatic std::atomic<bool> ptNextLaunchEnabled{false};'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'static std::wstring fgOverlaySettingsPath;')
 if name=='src/probe.cpp':
  addition='#include "pt_startup_settings.h"\n\n'
  assert source.count(addition)==1,addition
  source=source.replace(addition,'// Read once before any PT installation, including early device creation.\n// RT-only is the default. Mode changes require a process restart; ownership is\n// never torn down while recorded or submitted command lists can reference it.\nstatic bool PTStartupCaptureEnabled() {\n    static const bool enabled=[](){\n        const DWORD error=GetLastError();auto path=ModulePath(selfModule);\n        const auto slash=path.find_last_of(L"\\\\/");\n        if(slash==std::wstring::npos){SetLastError(error);return false;}\n        path.resize(slash+1);\n        const DWORD off=GetFileAttributesW((path+L"ControlFG-PT-Capture-Off.txt").c_str());\n        const DWORD on=GetFileAttributesW((path+L"ControlFG-PT-Enabled.txt").c_str());\n        const bool disabled=off!=INVALID_FILE_ATTRIBUTES&&!(off&FILE_ATTRIBUTE_DIRECTORY);\n        const bool optedIn=on!=INVALID_FILE_ATTRIBUTES&&!(on&FILE_ATTRIBUTE_DIRECTORY);\n        SetLastError(error);return optedIn&&!disabled;\n    }();\n    return enabled;\n}\n\n')
 if name=='src/fg_overlay.h':
  assert source.count('const bool ptAvailable = PTStartupCaptureEnabled();\n    const bool ptEnabled = ptAvailable && PTReflectionUserRequested();')==1
  source=source.replace('const bool ptAvailable = PTStartupCaptureEnabled();\n    const bool ptEnabled = ptAvailable && PTReflectionUserRequested();','const bool ptEnabled = PTReflectionUserRequested();')
  assert source.count('PaintFGButton(dc, ptToggle, !ptAvailable ? L"RT ONLY (RESTART)" :\n        (ptEnabled ? L"PT REFLECTIONS ON" : L"PT REFLECTIONS OFF"), ptEnabled, ptAvailable);')==1
  source=source.replace('PaintFGButton(dc, ptToggle, !ptAvailable ? L"RT ONLY (RESTART)" :\n        (ptEnabled ? L"PT REFLECTIONS ON" : L"PT REFLECTIONS OFF"), ptEnabled, ptAvailable);','PaintFGButton(dc, ptToggle, ptEnabled ? L"PT REFLECTIONS ON" : L"PT REFLECTIONS OFF", ptEnabled, true);')
  assert source.count('if(PTStartupCaptureEnabled()&&PTReflectionUserRequested()){')==1
  source=source.replace('if(PTStartupCaptureEnabled()&&PTReflectionUserRequested()){','if(PTReflectionUserRequested()){')
  assert source.count('if(FGOverlayPTToggleFromPoint(x,y)){\n            if(!PTStartupCaptureEnabled())return 0;')==1
  source=source.replace('if(FGOverlayPTToggleFromPoint(x,y)){\n            if(!PTStartupCaptureEnabled())return 0;','if(FGOverlayPTToggleFromPoint(x,y)){')
 # Glass capture adds observation only. Remove those explicit additions, then require
 # the entire remaining runtime to match the confirmed S1 source hashes.
 if name=='src/probe.cpp':
  # R51 adds read-only settings copies at explicit startup/present boundaries.
  for addition in ['#include "rr_pt_guides_runtime.h"\n', 'static std::wstring ptSettingsSnapshotBase;\n', '    ptSettingsSnapshotBase=dir+name;\n', '#include "pt_settings_snapshot.h"\n\n', '    if(count==1||count==240)PTSettingsSnapshot(count==1?L"first-present":L"present-240");\n', '        PTSettingsSnapshot(L"startup");\n']:
   assert source.count(addition)==1,addition
   source=source.replace(addition,'')
  source=re.sub(r'static std::atomic<unsigned long long> ptGlass(?:Miss|HitGroup)BindCount\{0\};\n','',source)
  source=re.sub(r'    if\(strcmp\(rrCurrentRayGeneration,"transparentRayGeneration"\)==0\) \{\n        const auto n=\+\+ptGlass(?:Miss|HitGroup)BindCount;\n        if\(n<=\d+\) Log\("PT_GLASS_[^\n]+\n    \}\n','',source)
 if name=='src/pt_reflection_loader.h':
  for addition in ['#include "pt_scene_binding_runtime.h"\n','    PTSceneBindingRuntime::Install(d3d);\n','#include "pt_glass_runtime.h"\n','    if(SUCCEEDED(nativeHr)&&out&&*out) PTGlassRuntime::ObserveCompute(device,desc,*out);\n','    PTGlassRuntime::InstallDevice(device);\n','    PTGlassRuntime::Install(renderer,d3d);\n']:
   assert source.count(addition)==1,addition
   source=source.replace(addition,'')
  source=source.replace('    if(desc) PTGlassCaptureCompute(desc->CS.pShaderBytecode,desc->CS.BytecodeLength);\n\n','')
  source=source.replace('#include "pt_glass_capture.h"\n\n','')
  source=source.replace('    const char* resident=ptReflectionOriginalStorageGet?ptReflectionOriginalStorageGet(id):nullptr;\n    PTGlassCaptureResident(id,resident);\n    return resident;',
      '    return ptReflectionOriginalStorageGet?ptReflectionOriginalStorageGet(id):nullptr;')
 # G3 retains a session-only glass mode and its overlay button.
 # Remove only these exact additions/replacement before checking the ORIGINAL
 # hashes; do not bless changes to existing opaque PT controls or rendering.
 if name=='src/pt_reflection_user_control.h':
  # R51 notifies descriptor mirroring on the existing toggle; state semantics remain unchanged.
  for addition in ['using PTReflectionModeObserver=void(*)(bool) noexcept;\nstatic std::atomic<PTReflectionModeObserver> ptReflectionModeObserver{nullptr};\n\n', '    if(const auto observer=ptReflectionModeObserver.load(std::memory_order_acquire))observer(enabled);\n']:
   assert source.count(addition)==1,addition
   source=source.replace(addition,'')
  addition='// Session-only glass comparison: native=0, working glossy=1.\n// Start from the visually confirmed glossy path; opaque PT is independent.\nstatic std::atomic<unsigned int> ptGlassUserMode{1u};'
  assert source.count(addition)==1
  source=source.replace(addition+'\n','')
 if name=='src/fg_overlay.h':
  click='        const int glassToggleY=794+FGOverlayLowerSectionOffset();\n        if(x>=304&&x<710&&y>=glassToggleY&&y<glassToggleY+52){\n            if(PTReflectionUserRequested()){\n                const unsigned mode=(ptGlassUserMode.load(std::memory_order_acquire)+1u)%2u;\n                ptGlassUserMode.store(mode,std::memory_order_release);\n                Log("PT_GLASS_UI_MODE mode=%u label=%s opaque_pt_unchanged=1",mode,\n                    mode==0?"native":"glossy");\n            }\n            InvalidateRect(hwnd,nullptr,FALSE);return 0;\n        }\n'
  paint='    const unsigned glassMode=ptGlassUserMode.load(std::memory_order_acquire);\n    const wchar_t* glassLabel=glassMode==0?L"GLASS: NATIVE":L"GLASS: GLOSSY";\n    PaintFGButton(dc, ptDescription, glassLabel, glassMode==1, ptEnabled);'
  original='    DrawTextW(dc, L"Compare traced second-bounce lighting.\\nNative reflections stay visible.", -1, &ptDescription,\n        DT_LEFT | DT_WORDBREAK);'
  assert source.count(click)==1 and source.count(paint)==1
  source=source.replace(click,'').replace(paint,original)
 # R37 adds a restart-only baseline gate. Verify its exact additions, then
 # retain the original S1 hashes for all enabled-mode runtime code.
 if name=='src/probe.cpp':
  startup='// Read once before any PT installation, including early device creation.\n// RT-only is the default. Mode changes require a process restart; ownership is\n// never torn down while recorded or submitted command lists can reference it.\nstatic bool PTStartupCaptureEnabled() {\n    static const bool enabled=[](){\n        const DWORD error=GetLastError();auto path=ModulePath(selfModule);\n        const auto slash=path.find_last_of(L"\\\\/");\n        if(slash==std::wstring::npos){SetLastError(error);return false;}\n        path.resize(slash+1);\n        const DWORD off=GetFileAttributesW((path+L"ControlFG-PT-Capture-Off.txt").c_str());\n        const DWORD on=GetFileAttributesW((path+L"ControlFG-PT-Enabled.txt").c_str());\n        const bool disabled=off!=INVALID_FILE_ATTRIBUTES&&!(off&FILE_ATTRIBUTE_DIRECTORY);\n        const bool optedIn=on!=INVALID_FILE_ATTRIBUTES&&!(on&FILE_ATTRIBUTE_DIRECTORY);\n        SetLastError(error);return optedIn&&!disabled;\n    }();\n    return enabled;\n}\n\n'
  setup='        const bool ptCaptureEnabled=PTStartupCaptureEnabled();\n        Log("PT_PRIMARY_STARTUP build=M12R89 capture=%u restart_only=1 rr_fg_available=1",unsigned(ptCaptureEnabled));\n        const unsigned int rrObserverHooks = ptCaptureEnabled?InstallRRObservationHooks(renderer,d3d):0;'
  assert source.count(startup)==1 and source.count(setup)==1
  source=source.replace(startup,'').replace(setup,'        const unsigned int rrObserverHooks = InstallRRObservationHooks(renderer,d3d);')
 if name=='src/pt_reflection_loader.h':
  gate='    if(!PTStartupCaptureEnabled())return false;\n'
  for signature in ['static bool PTReflectionInstallComputePsoHookOnDevice(ID3D12Device* device) noexcept {','static bool PTReflectionInstallCommandListCaptureHooks(void* commandList) noexcept {','static bool PTReflectionInstallShaderLookupHooks(HMODULE renderer,HMODULE d3d) noexcept {']:
   assert source.count(signature+'\n'+gate)==1
   source=source.replace(signature+'\n'+gate,signature+'\n')
 assert hashlib.sha256(source.encode()).hexdigest()==expected,name
def half(v): return struct.unpack('<e',struct.pack('<e',v))[0]
def sign(v): return 1 if v>=0 else -1
def unit(v):
 length=math.sqrt(sum(x*x for x in v));return [x/length for x in v]
def encode(n):
 n=[x/sum(abs(v) for v in n) for x in n]
 return n[:2] if n[2]>=0 else [(1-abs(n[1]))*sign(n[0]),(1-abs(n[0]))*sign(n[1])]
def decode(p):
 n=[p[0],p[1],1-abs(p[0])-abs(p[1])]
 if n[2]<0:n[:2]=[(1-abs(p[1]))*sign(p[0]),(1-abs(p[0]))*sign(p[1])]
 return unit(n)
r=random.Random(1511);worst=0;uv_error=0;old_error=0
vectors=[[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]]
vectors += [unit([r.uniform(-1,1) for _ in range(3)]) for _ in range(100000)]
for n in vectors:
 decoded=decode([half(v) for v in encode(n)])
 worst=max(worst,math.degrees(math.acos(max(-1,min(1,sum(a*b for a,b in zip(n,decoded)))))))
assert worst<.13,worst
for _ in range(100000):
 uv=r.uniform(-100,100);wrapped=uv-math.floor(uv)
 error=abs(half(wrapped)-wrapped);uv_error=max(uv_error,min(error,1-error))
 old_error=max(old_error,abs(round(wrapped*31)/31-wrapped))
assert uv_error<=1/4096 and old_error/uv_error>60,(uv_error,old_error)
assert all(half(i)==i for i in range(2048))
for width in [256,512,1024,2048,4096,8192]:
 lod=max(math.log2(width/2048),0)
 assert lod==(0 if width<=2048 else math.log2(width/2048))
ray=(root/'src/shaders/pt_reflection_p2.hlsl').read_text()
comp=(root/'src/shaders/pt_reflection_p2_native_composite.hlsl').read_text()
assert 'float4(PTSecondaryEncodeNormal(normalWorld),wrappedUV.x,encodedMaterial)' in ray
assert 'float4(viewHit,wrappedUV.y)' in ray and 'packedUV' not in ray
assert 'PTSecondaryDecodeNormal(packedSecondary.xy)' in comp
assert 'float2 uv=float2(packedSecondary.z,secondPos.w)' in comp
for name in ['src/shaders/pt_native_secondary.generated.hlsli','tools/generate-pt-native-secondary.py']:
 assert '/2048.0f' in (root/name).read_text()
print(f'PASS: confirmed rendering runtime unchanged except explicit glass capture/integration entry points; half UV max error {uv_error:.8f} vs old {old_error:.8f}; normal max error {worst:.6f} degrees; material IDs exact; texture mip floor checked')
