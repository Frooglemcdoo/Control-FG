"""Derive R19 from the exact reviewed R18 controller source; do not edit rendering code."""
from pathlib import Path
import hashlib
import sys
root = Path(sys.argv[1])
p = root / 'src/guard.cpp'
original = p.read_text(encoding='utf-8')
assert hashlib.sha256(original.encode()).hexdigest() == '4a0c2431e4e208d6dbbfd4c250eee6e716b07290694dc95a0b2ca1105209c876', 'Unexpected R18 source'
s = original

def replace(old, new):
    global s
    assert s.count(old) == 1, ('source edit anchor', old[:100], s.count(old))
    s = s.replace(old, new)

replace('#include "transition.h"', '#include "transition.h"\n#include "input_policy.h"')
replace('static std::atomic<uint32_t> physicalKeys{0};', '''static std::atomic<uint64_t> frameCalls{0}, modeCalls{0}, commitCalls{0}, commandCalls{0};
static bool ownsCoreHold=false; // Present-thread ownership, never clear core state while idle.
static unsigned probeStep=0; static DWORD probeError=0, probeFlags=0; // Worker thread only.
static uint64_t Age(uint64_t now,uint64_t timestamp){return timestamp && now>=timestamp ? now-timestamp : UINT64_MAX;}''')
replace('struct Display { bool valid{}, hdr{}; uint64_t tick{}; HMONITOR monitor{}; };',
        'struct Display { bool valid{}, hdr{}; uint64_t tick{}; HMONITOR monitor{}; unsigned step{}; DWORD error{}, flags{}; };')
start=s.index('static bool QueryHdr(')
end=s.index('static Frame sample(',start)
s=s[:start]+'''static bool QueryHdr(HMONITOR mon, bool& hdr) {
 probeStep=1;probeError=0;probeFlags=0;
 if(!mon){probeError=ERROR_INVALID_HANDLE;return false;}
 MONITORINFOEXW info{};info.cbSize=sizeof(info);probeStep=2;
 if(!GetMonitorInfoW(mon,&info)){probeError=GetLastError();return false;}
 for(unsigned retry=0;retry<3;++retry){UINT32 np=0,nm=0;probeStep=3;
  LONG result=GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&np,&nm);
  if(result!=ERROR_SUCCESS){probeError=static_cast<DWORD>(result);return false;}
  if(np>128 || nm>4096){probeError=ERROR_INVALID_DATA;return false;}
  std::vector<DISPLAYCONFIG_PATH_INFO> paths(np);std::vector<DISPLAYCONFIG_MODE_INFO> modes(nm);probeStep=4;
  result=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&np,paths.data(),&nm,modes.data(),nullptr);
  if(result==ERROR_INSUFFICIENT_BUFFER)continue;
  if(result!=ERROR_SUCCESS){probeError=static_cast<DWORD>(result);return false;}
  for(UINT32 i=0;i<np;++i){const auto& path=paths[i];DISPLAYCONFIG_SOURCE_DEVICE_NAME src{};
   src.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;src.header.size=sizeof(src);src.header.adapterId=path.sourceInfo.adapterId;src.header.id=path.sourceInfo.id;
   probeStep=5;result=DisplayConfigGetDeviceInfo(&src.header);
   if(result!=ERROR_SUCCESS){probeError=static_cast<DWORD>(result);continue;}
   if(_wcsicmp(src.viewGdiDeviceName,info.szDevice)!=0)continue;
   DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO ac{};ac.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
   ac.header.size=sizeof(ac);ac.header.adapterId=path.targetInfo.adapterId;ac.header.id=path.targetInfo.id;
   probeStep=6;result=DisplayConfigGetDeviceInfo(&ac.header);
   if(result!=ERROR_SUCCESS){probeError=static_cast<DWORD>(result);return false;}
   probeFlags=ac.value;probeStep=7;
   if(!ac.advancedColorSupported){probeError=ERROR_NOT_SUPPORTED;return false;}
   hdr=ac.advancedColorEnabled!=0;probeStep=0;probeError=0;return true;
  }
  if(!probeError)probeError=ERROR_NOT_FOUND;return false;
 }
 probeStep=4;probeError=ERROR_INSUFFICIENT_BUFFER;return false;
}
static void RefreshDisplay(){
 if(!busy.load() && OurForeground())targetMonitor.store(MonitorFromWindow(GetForegroundWindow(),MONITOR_DEFAULTTONEAREST));
 Display old=currentDisplay(),d{};d.monitor=targetMonitor.load();d.valid=QueryHdr(d.monitor,d.hdr);d.tick=GetTickCount64();
 d.step=probeStep;d.error=probeError;d.flags=probeFlags;
 AcquireSRWLockExclusive(&displayLock);display=d;ReleaseSRWLockExclusive(&displayLock);
 if(d.valid!=old.valid || d.monitor!=old.monitor || d.step!=old.step || d.error!=old.error || (d.valid && d.hdr!=old.hdr))
  Log("ORDERED_HDR_DISPLAY valid=%u hdr=%u monitor=%p probe_step=%u error=%lu flags=0x%lX controller_busy=%u",
      unsigned(d.valid),unsigned(d.hdr),d.monitor,d.step,d.error,d.flags,unsigned(busy.load()));
 if(old.valid && d.valid && old.monitor==d.monitor && old.hdr!=d.hdr && !busy.load())
  Log("ORDERED_HDR_EXTERNAL_CHANGE old_hdr=%u new_hdr=%u controlled_request=0 use=CtrlAltF11",unsigned(old.hdr),unsigned(d.hdr));
}
static InputReadiness InputState(){
 InputReadiness r{};const auto now=GetTickCount64();const auto d=currentDisplay();
 r.ready=ready.load();r.foreground=OurForeground();r.busy=busy.load() || request.load();
 r.frames=frameCalls.load();r.frameAge=Age(now,lastFrameTick.load());
 r.gpuKnown=read32(0xABB10)!=0;r.rtx40=read32(0xABB14)!=0;r.selection=read32(0xAA0A0);
 r.displayValid=d.valid && d.monitor==targetMonitor.load();r.displayAge=Age(now,d.tick);return r;
}
static void Heartbeat(){
 const auto r=InputState();const auto d=currentDisplay();
 Log("ORDERED_HDR_HEARTBEAT ready=%u reason=%s frame_calls=%llu mode_calls=%llu commit_calls=%llu command_calls=%llu core_present=%llu frame_age_ms=%llu selection=%u fg_api=%u gpu_known=%u rtx40=%u foreground=%u display_valid=%u display_age_ms=%llu probe_step=%u probe_error=%lu phase=%s",
   unsigned(r.ready),inputReason(r),r.frames,modeCalls.load(),commitCalls.load(),commandCalls.load(),read64(0xAB7B8),r.frameAge,r.selection,read32(0xAB9D0),unsigned(r.gpuKnown),unsigned(r.rtx40),unsigned(r.foreground),unsigned(r.displayValid),r.displayAge,d.step,d.error,name(static_cast<Phase>(publishedPhase.load())));
}
static void HandleControllerCommand(){
 ++commandCalls;RefreshDisplay();const auto r=InputState();
 Log("ORDERED_HDR_COMMAND_RECEIVED source=WM_HOTKEY shortcut=CtrlAltF11 decision=%s",inputReason(r));Heartbeat();
 if(!inputAllowed(r)){if(r.foreground)MessageBeep(MB_ICONWARNING);return;}
 bool expected=false;if(!busy.compare_exchange_strong(expected,true)){Log("ORDERED_HDR_COMMAND_REJECT reason=request_race");return;}
 request.store(true,std::memory_order_release);
 Log("ORDERED_HDR_COMMAND_QUEUED source=CtrlAltF11 hdr_toggle_sent=0");MessageBeep(MB_OK);
}
''' + s[end:]
replace('bool h=control.hold();hold.store(h,std::memory_order_release);writeHold(h);',
'''bool h=control.hold();hold.store(h,std::memory_order_release);
 if(h){writeHold(true);ownsCoreHold=true;}else if(ownsCoreHold){writeHold(false);ownsCoreHold=false;}''')
replace('busy.store(control.active(),std::memory_order_release);publishedPhase.store',
        'busy.store(control.active() || request.load(),std::memory_order_release);publishedPhase.store')
replace('static void OnCommit(uint64_t p,HRESULT hr,const char* source){\n originalCommit',
        'static void OnCommit(uint64_t p,HRESULT hr,const char* source){\n ++commitCalls;originalCommit')
replace('static void OnMode(uint64_t p,bool frameReady){',
        'static void OnMode(uint64_t p,bool frameReady){\n ++modeCalls;')
replace('static void OnFrame(){\n if(!ready.load()){originalFrame();return;}',
'''static void OnFrame(){
 ++frameCalls;
 if(!ready.load()){originalFrame();return;}''')
replace('insideFrame=true;lastFrameTick.store(GetTickCount64());',
'''insideFrame=true;lastFrameTick.store(GetTickCount64());
 static bool connectionLogged=false;if(!connectionLogged){connectionLogged=true;Log("ORDERED_HDR_RENDER_CONNECTED core_present=%llu mode_rva=0x41130 commit_rva=0x10950",read64(0xAB7B8));}''')
replace('Log("ORDERED_HDR_REQUEST_CANCELLED reason=not_ready_or_user_already_off");',
'''Log("ORDERED_HDR_REQUEST_CANCELLED selection=%u foreground=%u display_valid=%u",f.selection,unsigned(f.foreground),unsigned(f.displayValid));''')
replace('d.valid && f.now-d.tick<500;', 'd.valid && Age(GetTickCount64(),d.tick)<500;')
start=s.index('static LRESULT CALLBACK Keyboard(')
end=s.index('static bool KeysReleased()',start)
s=s[:start]+s[end:]
replace(' if(physicalKeys.load()!=0)return false;\n','')
replace("static_cast<int>('B')}", "static_cast<int>('B'),VK_F11}")
start=s.index('  HHOOK keyboard=SetWindowsHookExW(')
end=s.index('  uint64_t lastPoll=0;',start)
s=s[:start]+'''  struct HotkeyRegistration {
   ATOM id{};bool registered{};
   ~HotkeyRegistration(){if(registered)UnregisterHotKey(nullptr,id);if(id)GlobalDeleteAtom(id);}
  } registration;
  registration.id=GlobalAddAtomW(L"ControlFG.OrderedHDR.R19.CtrlAltF11");
  if(!registration.id){Log("ORDERED_HDR_INSTALL_FAIL reason=hotkey_atom error=%lu",GetLastError());return 0;}
  registration.registered=RegisterHotKey(nullptr,registration.id,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F11)!=FALSE;
  if(!registration.registered){Log("ORDERED_HDR_INSTALL_FAIL reason=RegisterHotKey_CtrlAltF11 error=%lu",GetLastError());return 0;}
  ready.store(true,std::memory_order_release);
  Log("ORDERED_HDR_COMMAND_HOTKEY registered=1 shortcut=CtrlAltF11 low_level_hook=disabled native_WinAltB=not_intercepted");
''' + s[end:]
replace('uint64_t lastPoll=0;', 'uint64_t lastPoll=0,lastHeartbeat=0;')
replace('if(msg.message==WM_QUIT)return 0;TranslateMessage(&msg);DispatchMessageW(&msg);',
'''if(msg.message==WM_QUIT)return 0;
    if(isControllerMessage(msg.message,msg.wParam,registration.id)){HandleControllerCommand();continue;}
    TranslateMessage(&msg);DispatchMessageW(&msg);''')
start=s.index('    if(!busy.load() && OurForeground())targetMonitor.store(', s.index('uint64_t lastPoll=0'))
end=s.index('   TryReplay();',start)
s=s[:start]+'''    RefreshDisplay();lastPoll=GetTickCount64();
   }
   if(GetTickCount64()-lastHeartbeat>=5000){Heartbeat();lastHeartbeat=GetTickCount64();}
''' + s[end:]
replace('  UnhookWindowsHookEx(keyboard);','  Log("ORDERED_HDR_WORKER_EXIT error=%lu",GetLastError());')
s=s.replace('R18','R19').replace('0x00120001','0x00130001').replace('0x4346474844523138ULL','0x4346474844523139ULL')
p.write_text(s,encoding='utf-8')
assert hashlib.sha256((root/'src/transition.h').read_text().encode()).hexdigest()=='6ebe60fbe7d60aee48e02584b01fef62849cab507b61912f127495da13a4f757'
for name in ('tests/load_test.cpp','tests/core_load_test.cpp'):
 p=root/name;p.write_text(p.read_text().replace('0x00120001','0x00130001'))
p=root/'tools/prepare.py';p.write_text(p.read_text().replace('R18','R19'))
p=root/'CMakeLists.txt';p.write_text(p.read_text()+'''
add_executable(input_policy_test tests/input_policy_test.cpp)
target_include_directories(input_policy_test PRIVATE src)
target_compile_options(input_policy_test PRIVATE /W4 /WX)
add_test(NAME input_policy COMMAND input_policy_test)
add_executable(hotkey_message_test tests/hotkey_message_test.cpp)
target_include_directories(hotkey_message_test PRIVATE src)
target_link_libraries(hotkey_message_test PRIVATE user32)
target_compile_options(hotkey_message_test PRIVATE /W4 /WX)
add_test(NAME hotkey_message COMMAND hotkey_message_test)
''')
print('R19 controller generated; original transition policy and v2 rendering are unchanged')
