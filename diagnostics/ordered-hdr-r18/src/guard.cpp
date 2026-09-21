#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <bcrypt.h>
#include <atomic>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <initializer_list>
#include "MinHook.h"
#include "transition.h"
#include "expected_core.h"
#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"d3d12.lib")
using namespace hdrguard;
static_assert(sizeof(void*)==8 && sizeof(LONG)==4, "x64 Windows required");
static HMODULE selfModule{}; static unsigned char* core{};
static std::atomic<bool> ready{false}, busy{false}, hold{false}, request{false}, eligible{false};
static std::atomic<int> publishedPhase{0}, replayResult{0};
static std::atomic<uint64_t> replayWanted{0}, replaySent{0}, serial{0}, lastFrameTick{0};
static std::atomic<HMONITOR> targetMonitor{nullptr};
static std::atomic<ULONGLONG> replayTick{0};
static std::atomic<bool> replayOldHdr{false};
static std::atomic<uint32_t> physicalKeys{0};
static constexpr ULONG_PTR kReplayMarker=0x4346474844523138ULL;
static HANDLE logFile=INVALID_HANDLE_VALUE; static SRWLOCK logLock=SRWLOCK_INIT, displayLock=SRWLOCK_INIT;
struct Display { bool valid{}, hdr{}; uint64_t tick{}; HMONITOR monitor{}; };
static Display display;
static Transition control; // Present-thread ownership only.
static Phase lastLogged=Phase::Idle;
static uint64_t actualPresent{}, offAckPresent{}, lastWaitLog{};
static HRESULT actualResult=E_PENDING;
static thread_local bool insideFrame=false, startCall=false;
static std::atomic<DWORD> renderThread{0};
static ID3D12CommandQueue* directQueue{}; static ID3D12Fence* drainFence{};
static uint64_t nextFence=0, stopFence=0, warmFence=0;
static void (*originalFrame)(){};
static void (*originalMode)(uint64_t,bool){};
static void (*originalCommit)(uint64_t,HRESULT,const char*){};
static bool (*beginReset)(uint64_t,const char*){};
static uint32_t read32(size_t r){return static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(core+r),0,0));}
static uint64_t read64(size_t r){return static_cast<uint64_t>(InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(core+r),0,0));}
static void writeHold(bool enabled){InterlockedExchange(reinterpret_cast<volatile LONG*>(core+0xABC98),enabled?1:0);}
static void Log(const char* fmt,...) {
 if(logFile==INVALID_HANDLE_VALUE)return;
 char text[3072],line[3300];va_list ap;va_start(ap,fmt);vsnprintf(text,sizeof(text),fmt,ap);va_end(ap);
 LARGE_INTEGER q{};QueryPerformanceCounter(&q);
 int n=snprintf(line,sizeof(line),"qpc=%lld tick=%llu tid=%lu %s\r\n",q.QuadPart,GetTickCount64(),GetCurrentThreadId(),text);
 if(n<=0)return;DWORD wrote{};AcquireSRWLockExclusive(&logLock);WriteFile(logFile,line,static_cast<DWORD>(n<3300?n:3299),&wrote,nullptr);ReleaseSRWLockExclusive(&logLock);
}
static bool OurForeground(){DWORD pid{};HWND w=GetForegroundWindow();return w && GetWindowThreadProcessId(w,&pid) && pid==GetCurrentProcessId();}
static Display currentDisplay(){AcquireSRWLockShared(&displayLock);Display d=display;ReleaseSRWLockShared(&displayLock);return d;}
static bool QueryHdr(HMONITOR mon, bool& hdr) {
 if(!mon)return false; MONITORINFOEXW info{};info.cbSize=sizeof(info);if(!GetMonitorInfoW(mon,&info))return false;
 for(unsigned retry=0;retry<3;++retry){UINT32 np=0,nm=0;
  if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&np,&nm)!=ERROR_SUCCESS || np>128 || nm>4096)return false;
  std::vector<DISPLAYCONFIG_PATH_INFO> paths(np);std::vector<DISPLAYCONFIG_MODE_INFO> modes(nm);
  LONG result=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&np,paths.data(),&nm,modes.data(),nullptr);
  if(result==ERROR_INSUFFICIENT_BUFFER)continue;if(result!=ERROR_SUCCESS)return false;
  for(UINT32 i=0;i<np;++i){const auto& p=paths[i];DISPLAYCONFIG_SOURCE_DEVICE_NAME src{};
   src.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;src.header.size=sizeof(src);src.header.adapterId=p.sourceInfo.adapterId;src.header.id=p.sourceInfo.id;
   if(DisplayConfigGetDeviceInfo(&src.header)!=ERROR_SUCCESS || _wcsicmp(src.viewGdiDeviceName,info.szDevice)!=0)continue;
   DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO ac{};ac.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
   ac.header.size=sizeof(ac);ac.header.adapterId=p.targetInfo.adapterId;ac.header.id=p.targetInfo.id;
   if(DisplayConfigGetDeviceInfo(&ac.header)!=ERROR_SUCCESS)return false;
   if(!ac.advancedColorSupported)return false;hdr=ac.advancedColorEnabled!=0;return true;
  }return false;
 }return false;
}
static Frame sample(uint64_t p, bool beforePresent=false){
 Frame f{};f.now=GetTickCount64();f.present=p;f.generation=read64(0xABD50);f.settled=read64(0xABBF0);
 f.frees=read64(0xABC70);f.generated=read64(0xABB28);f.coreOff=read32(0xAB9D0)==0;f.selection=read32(0xAA0A0);
 f.bridgeHdr=read32(0xABCF0)!=0;f.coreResetIdle=read32(0xABC50)==0 && read32(0xABC1C)==0;
 f.foreground=OurForeground();f.presentOK=beforePresent || (actualPresent==p && actualResult==S_OK);
 auto d=currentDisplay();f.displayValid=d.valid && f.now>=d.tick && f.now-d.tick<500 && d.monitor==targetMonitor.load();f.hdr=d.hdr;
 size_t slot=0xABB58+static_cast<size_t>(p%6)*24;
 bool token=read64(slot)==p && read64(slot+8)!=0 && read32(slot+16)!=0;
 uint32_t tags=read32(slot+20);f.allInputs=token && (tags&15)==15 && read32(0xABA68)>0 && read32(0xABA6C)>0 && read32(0xABA70)!=0 && read32(0xABA74)!=0;
 f.colorWidth=read32(0xABA78);f.colorHeight=read32(0xABA7C);f.colorFormat=read32(0xABA80);f.hudlessFormat=read32(0xABA84);
 if(drainFence){const uint64_t done=drainFence->GetCompletedValue();if(done!=UINT64_MAX){f.stopFenceDone=stopFence && done>=stopFence;f.warmFenceDone=warmFence && done>=warmFence;}}
 return f;
}
static bool SignalFence(uint64_t& value){
 auto* q=reinterpret_cast<ID3D12CommandQueue*>(read64(0xAB998));if(!q){Log("ORDERED_HDR_FENCE_FAIL reason=no_direct_queue");return false;}
 if(q!=directQueue){if(drainFence){drainFence->Release();drainFence=nullptr;}if(directQueue)directQueue->Release();
  directQueue=q;directQueue->AddRef();ID3D12Device* device=nullptr;
  if(directQueue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT || FAILED(directQueue->GetDevice(IID_PPV_ARGS(&device))))return false;
  HRESULT hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&drainFence));device->Release();if(FAILED(hr)){Log("ORDERED_HDR_FENCE_FAIL hr=0x%08lX",hr);return false;}
 }
 value=++nextFence;const HRESULT hr=directQueue->Signal(drainFence,value);
 Log("ORDERED_HDR_FENCE_SIGNAL serial=%llu value=%llu queue=%p hr=0x%08lX",serial.load(),value,directQueue,hr);return SUCCEEDED(hr);
}
static void Publish(){
 bool h=control.hold();hold.store(h,std::memory_order_release);writeHold(h);
 busy.store(control.active(),std::memory_order_release);publishedPhase.store(static_cast<int>(control.phase),std::memory_order_release);
 if(control.phase!=lastLogged){Log("ORDERED_HDR_STATE serial=%llu phase=%s hold=%u error=%s",serial.load(),name(control.phase),h?1:0,control.error);lastLogged=control.phase;}
}
static void OnCommit(uint64_t p,HRESULT hr,const char* source){
 originalCommit(p,hr,source);
 if(!ready.load() || GetCurrentThreadId()!=renderThread)return;
 if(source && (strcmp(source,"present")==0 || strcmp(source,"present1")==0)){
  actualPresent=p;actualResult=hr;
  if(control.phase==Phase::Stopping && hr==S_OK && read32(0xAB9D0)==0 && read64(0xABC70)>control.baseFree){
   control.stopCommitted=true;offAckPresent=p;Log("ORDERED_HDR_OFF_COMMITTED serial=%llu present=%llu core_frees=%llu hdr_replayed=0",serial.load(),p,read64(0xABC70));
  }
 }
}
static void OnMode(uint64_t p,bool frameReady){
 if(ready.load() && hold.load(std::memory_order_acquire))writeHold(true);
 if(ready.load() && insideFrame && !startCall && GetCurrentThreadId()==renderThread && control.phase==Phase::ResumeReady && frameReady){
  Frame f=sample(p,true);if(control.releaseForCurrentFrame(f)){
   Log("ORDERED_HDR_RESTORE serial=%llu present=%llu selection=%u tags=0x%X color=%ux%u format=%u hudless_format=%u generation=%llu UI_path=original_v2",
       serial.load(),p,f.selection,read32(0xABB58+static_cast<size_t>(p%6)*24+20),f.colorWidth,f.colorHeight,f.colorFormat,f.hudlessFormat,f.generation);Publish();
  }
 }
 originalMode(p,frameReady); // Preserve the original complete UI freshness checks; never pass a fabricated ready value.
}
static void OnFrame(){
 if(!ready.load()){originalFrame();return;}
 DWORD me=GetCurrentThreadId();DWORD zero=0;renderThread.compare_exchange_strong(zero,me);
 if(me!=renderThread){originalFrame();return;}
 insideFrame=true;lastFrameTick.store(GetTickCount64());
 uint64_t next=read64(0xAB7B8)+1;
 if(request.exchange(false)){
  Frame f=sample(next,true);
  if(f.selection!=0 && control.start(f)){
   ++serial;stopFence=warmFence=0;offAckPresent=0;replayResult.store(0);replayWanted.store(0);lastLogged=Phase::Idle;Publish();
   Log("ORDERED_HDR_REQUEST serial=%llu present=%llu old_hdr=%u target_hdr=%u selection=%u preferences_untouched=1",serial.load(),next,control.oldHdr?1:0,control.targetHdr?1:0,f.selection);
   startCall=true;const bool armed=beginReset(next,"ordered_hdr_shortcut_before_os_toggle");startCall=false;
   if(!armed)control.fail("core_off_reset_not_armed");Publish();
  }else {busy.store(control.active());Log("ORDERED_HDR_REQUEST_CANCELLED reason=not_ready_or_user_already_off");}
 }
 int reply=replayResult.exchange(0);if(reply){control.replayResult(reply==1,replayTick.load());Publish();}
 control.manualOffRecovery(read32(0xAA0A0));if(control.active())control.timeout(GetTickCount64());Publish();
 originalFrame();
 Frame f=sample(read64(0xAB7B8));Phase before=control.phase;control.observe(f);
 if(control.phase==Phase::Drain && !stopFence){if(!SignalFence(stopFence))control.fail("stop_fence_signal_failed");}
 if(control.phase==Phase::ReplayReady && before!=Phase::ReplayReady){
  Log("ORDERED_HDR_REPLAY_AUTHORIZED serial=%llu off_present=%llu now_present=%llu completed_off_presents=%u fence=%llu hdr_still=%u",
      serial.load(),offAckPresent,f.present,control.offPresents,stopFence,f.hdr?1:0);replayOldHdr.store(control.oldHdr);replayWanted.store(serial.load(),std::memory_order_release);
 }
 if(control.phase==Phase::WarmFence && before!=Phase::WarmFence){warmFence=0;if(!SignalFence(warmFence))control.fail("warm_fence_signal_failed");}
 if(control.phase==Phase::AwaitDisplay || control.phase==Phase::Warming){if(before==Phase::WarmFence || before==Phase::ResumeReady)warmFence=0;}
 if(before==Phase::Confirming && control.phase==Phase::Idle){Log("ORDERED_HDR_COMPLETE serial=%llu present=%llu hdr=%u selection=%u actual_generation_confirmed=%u ui_recomposition=%u",
      serial.load(),f.present,f.hdr?1:0,f.selection,f.selection!=0?1:0,read32(0xABBEC));}
 if(control.active() && f.now-lastWaitLog>=1000){
  lastWaitLog=f.now;const auto slot=0xABB58+static_cast<size_t>(f.present%6)*24;
  Log("ORDERED_HDR_WAIT serial=%llu phase=%s present=%llu display_valid=%u hdr=%u target_hdr=%u bridge_hdr=%u core_off=%u present_ok=%u reset_idle=%u generation=%llu settled=%llu tags=0x%X all_inputs=%u color=%ux%u format=%u hudless_format=%u good_frames=%u stop_fence=%u warm_fence=%u",
   serial.load(),name(control.phase),f.present,unsigned(f.displayValid),unsigned(f.hdr),unsigned(control.targetHdr),unsigned(f.bridgeHdr),unsigned(f.coreOff),unsigned(f.presentOK),unsigned(f.coreResetIdle),f.generation,f.settled,read32(slot+20),unsigned(f.allInputs),f.colorWidth,f.colorHeight,f.colorFormat,f.hudlessFormat,control.goodFrames,unsigned(f.stopFenceDone),unsigned(f.warmFenceDone));
 }
 Publish();
 auto d=currentDisplay();const bool canStart=read32(0xABB10)!=0 && read32(0xABB14)==0 && read32(0xAA0A0)!=0 && d.valid && f.now-d.tick<500;
 eligible.store(canStart,std::memory_order_release);insideFrame=false;
}
static LRESULT CALLBACK Keyboard(int code,WPARAM wp,LPARAM lp){
 if(code!=HC_ACTION)return CallNextHookEx(nullptr,code,wp,lp);
 const auto& k=*reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);
 if(k.dwExtraInfo==kReplayMarker || (k.flags&LLKHF_INJECTED))return CallNextHookEx(nullptr,code,wp,lp);
 bool down=(wp==WM_KEYDOWN || wp==WM_SYSKEYDOWN),up=(wp==WM_KEYUP || wp==WM_SYSKEYUP);if(!down&&!up)return CallNextHookEx(nullptr,code,wp,lp);
 uint32_t bit=0;switch(k.vkCode){case VK_LWIN:bit=1;break;case VK_RWIN:bit=2;break;case VK_LMENU:case VK_MENU:bit=4;break;case VK_RMENU:bit=8;break;
 case VK_LCONTROL:case VK_RCONTROL:case VK_CONTROL:bit=16;break;case VK_LSHIFT:case VK_RSHIFT:case VK_SHIFT:bit=32;break;case 'B':bit=64;break;}
 uint32_t keys=physicalKeys.load();if(bit){if(down)keys|=bit;else keys&=~bit;physicalKeys.store(keys);}
 static bool swallowedB=false;
 if(k.vkCode=='B' && swallowedB){if(up)swallowedB=false;return 1;}
 if(k.vkCode=='B' && down && (keys&3) && (keys&12) && !(keys&48) && OurForeground()){
  if(busy.load()){swallowedB=true;return 1;}
  if(ready.load() && eligible.load() && GetTickCount64()-lastFrameTick.load()<500){
   bool expected=false;if(busy.compare_exchange_strong(expected,true)){request.store(true);swallowedB=true;return 1;}
  }
 }
 return CallNextHookEx(nullptr,code,wp,lp);
}
static bool KeysReleased(){
 if(physicalKeys.load()!=0)return false;
 for(int key:{VK_LWIN,VK_RWIN,VK_LMENU,VK_RMENU,VK_LCONTROL,VK_RCONTROL,VK_LSHIFT,VK_RSHIFT,static_cast<int>('B')})if(GetAsyncKeyState(key)&0x8000)return false;
 return true;
}
static void TryReplay(){
 uint64_t want=replayWanted.load(std::memory_order_acquire);
 if(!want || want==replaySent.load() || publishedPhase.load()!=static_cast<int>(Phase::ReplayReady))return;
 if(!OurForeground()){replaySent.store(want);replayTick.store(GetTickCount64());replayResult.store(2);return;}
 if(!KeysReleased())return;
 bool observed=false;
 if(!QueryHdr(targetMonitor.load(),observed))return;
 if(observed!=replayOldHdr.load() || read32(0xAB9D0)!=0 || !hold.load()){
  Log("ORDERED_HDR_REPLAY_ABORT serial=%llu reason=display_or_fg_changed_before_injection",want);
  replaySent.store(want);replayTick.store(GetTickCount64());replayResult.store(2);return;
 }
 if(!OurForeground() || !KeysReleased())return;
 replaySent.store(want);INPUT in[6]{};WORD key[6]={VK_LWIN,VK_LMENU,'B','B',VK_LMENU,VK_LWIN};
 for(int i=0;i<6;++i){in[i].type=INPUT_KEYBOARD;in[i].ki.wVk=key[i];in[i].ki.dwExtraInfo=kReplayMarker;
  if(i>=3)in[i].ki.dwFlags|=KEYEVENTF_KEYUP;if(key[i]==VK_LWIN)in[i].ki.dwFlags|=KEYEVENTF_EXTENDEDKEY;}
 SetLastError(ERROR_SUCCESS);UINT sent=SendInput(6,in,sizeof(INPUT));DWORD err=GetLastError();
 if(sent!=6){ // Release only modifiers from our attempted batch, never replay the toggle a second time.
  INPUT release[3]{};for(int i=0;i<3;++i){release[i]=in[i+3];}SendInput(3,release,sizeof(INPUT));
 }
 Log("ORDERED_HDR_REPLAY serial=%llu events_sent=%u expected=6 error=%lu replay_count=1 source=WinAltB",want,sent,err);
 replayTick.store(GetTickCount64());replayResult.store(sent==6?1:2,std::memory_order_release);
}
static std::string HashFile(const std::wstring& path){
 HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return {};
 BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};std::string out;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 && BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
  unsigned char buffer[65536];DWORD count{};bool good=true;while(true){if(!ReadFile(f,buffer,sizeof(buffer),&count,nullptr)){good=false;break;}if(!count)break;
   if(BCryptHashData(hash,buffer,count,0)<0){good=false;break;}}
  if(good && BCryptFinishHash(hash,digest,32,0)>=0){char hex[65];for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);out=hex;}
 }
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return out;
}
static bool InstallHooks(){
 struct Site{size_t rva;const unsigned char* sig;size_t n;void* replacement;void** original;};
 static const unsigned char frameSig[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
 static const unsigned char modeSig[]={0x48,0x8b,0xc4,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
 static const unsigned char commitSig[]={0x40,0x53,0x48,0x83,0xec,0x30,0x4d,0x8b,0xc8,0x48,0x8b,0xd9};
 Site sites[]={{0x1DF60,frameSig,sizeof(frameSig),(void*)&OnFrame,(void**)&originalFrame},{0x41130,modeSig,sizeof(modeSig),(void*)&OnMode,(void**)&originalMode},
 {0x10950,commitSig,sizeof(commitSig),(void*)&OnCommit,(void**)&originalCommit}};
 for(const auto& s:sites)if(memcmp(core+s.rva,s.sig,s.n)!=0){Log("ORDERED_HDR_INSTALL_FAIL reason=core_signature rva=0x%zX",s.rva);return false;}
 if(MH_Initialize()!=MH_OK)return false;
 for(const auto& s:sites){auto r=MH_CreateHook(core+s.rva,s.replacement,s.original);if(r!=MH_OK){Log("ORDERED_HDR_INSTALL_FAIL rva=0x%zX minhook=%s",s.rva,MH_StatusToString(r));MH_Uninitialize();return false;}}
 beginReset=reinterpret_cast<bool(*)(uint64_t,const char*)>(core+0xCDC0);
 for(const auto& s:sites)if(MH_QueueEnableHook(core+s.rva)!=MH_OK){MH_Uninitialize();return false;}
 if(MH_ApplyQueued()!=MH_OK){MH_Uninitialize();return false;}return true;
}
static DWORD WINAPI Worker(void*){
 try{
  wchar_t exe[32768]{},own[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto* leaf=wcsrchr(exe,L'\\');leaf=leaf?leaf+1:exe;
  if(_wcsicmp(leaf,L"Control_DX12.exe")!=0)return 0;
  GetModuleFileNameW(selfModule,own,32768);std::wstring directory(own);directory=directory.substr(0,directory.find_last_of(L"\\/"));
  wchar_t appdata[32768]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",appdata,32768))return 0;
  std::wstring logDir=std::wstring(appdata)+L"\\ControlFGProbe";CreateDirectoryW(logDir.c_str(),nullptr);
  SYSTEMTIME st{};GetSystemTime(&st);wchar_t nameBuffer[180];swprintf_s(nameBuffer,L"\\hdr-ordered-R18-%04u%02u%02u-%02u%02u%02u-%lu.log",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,GetCurrentProcessId());
  logFile=CreateFileW((logDir+nameBuffer).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  Log("ORDERED_HDR_BUILD version=R18 compiled_controller=1 ui_path=original_v2 generation_patches=0 expected_core_sha256=%s",kExpectedCoreSha256);
  const std::wstring corePath=directory+L"\\dxgi.dll";std::string digest=HashFile(corePath);
  if(digest!=kExpectedCoreSha256){Log("ORDERED_HDR_INSTALL_FAIL reason=core_hash actual=%s",digest.c_str());return 0;}
  core=reinterpret_cast<unsigned char*>(GetModuleHandleW(corePath.c_str()));if(!core){Log("ORDERED_HDR_INSTALL_FAIL reason=core_module");return 0;}
  if(!InstallHooks()){Log("ORDERED_HDR_INSTALL_FAIL reason=hook_install");return 0;}
  MSG msg{};PeekMessageW(&msg,nullptr,WM_USER,WM_USER,PM_NOREMOVE);
  HHOOK keyboard=SetWindowsHookExW(WH_KEYBOARD_LL,Keyboard,selfModule,0);
  if(!keyboard){Log("ORDERED_HDR_INSTALL_FAIL reason=keyboard_hook error=%lu",GetLastError());return 0;}
  ready.store(true,std::memory_order_release);Log("ORDERED_HDR_HOTKEY_HOOK installed=1 original_display_recovery_unchanged=1");
  uint64_t lastPoll=0;
  while(true){DWORD wait=MsgWaitForMultipleObjectsEx(0,nullptr,25,QS_ALLINPUT,MWMO_INPUTAVAILABLE);if(wait==WAIT_FAILED)break;
   while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)return 0;TranslateMessage(&msg);DispatchMessageW(&msg);}
   uint64_t now=GetTickCount64();if(now-lastPoll>=100){
    if(!busy.load() && OurForeground())targetMonitor.store(MonitorFromWindow(GetForegroundWindow(),MONITOR_DEFAULTTONEAREST));
    Display d{};d.monitor=targetMonitor.load();d.valid=QueryHdr(d.monitor,d.hdr);d.tick=GetTickCount64();
    AcquireSRWLockExclusive(&displayLock);display=d;ReleaseSRWLockExclusive(&displayLock);lastPoll=now;
   }
   TryReplay();
  }
  UnhookWindowsHookEx(keyboard);
 }catch(...){Log("ORDERED_HDR_WORKER_ERROR controller_disabled=1");eligible.store(false);}
 return 0;
}
extern "C" __declspec(dllexport) void WINAPI ControlFGHDRGuard_Bootstrap(){}
extern "C" __declspec(dllexport) unsigned WINAPI ControlFGHDRGuard_Version(){return 0x00120001;}
BOOL WINAPI DllMain(HINSTANCE mod,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){selfModule=mod;DisableThreadLibraryCalls(mod);HANDLE t=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);if(t)CloseHandle(t);}return TRUE;}
