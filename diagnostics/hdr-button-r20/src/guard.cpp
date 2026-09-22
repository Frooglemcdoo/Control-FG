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
#include "MinHook.h"
#include "transition.h"
#include "expected_core.h"

#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"user32.lib")

using namespace hdrbutton;

static HMODULE selfModule{};
static unsigned char* core{};
static void (*originalFrame)(){};
static HANDLE logFile=INVALID_HANDLE_VALUE;
static SRWLOCK logLock=SRWLOCK_INIT;
static SRWLOCK stateLock=SRWLOCK_INIT;
static State transition{};
static std::atomic<bool> ready{false};
static std::atomic<bool> pendingBegin{false};
static std::atomic<bool> currentHdr{false};
static std::atomic<bool> currentHdrKnown{false};
static std::atomic<bool> displayHdrSupported{false};
static std::atomic<HWND> overlayWindow{nullptr};
static std::atomic<HWND> buttonWindow{nullptr};
static WNDPROC originalOverlayProc{};
static constexpr UINT kAttachButtonMessage = WM_APP + 0x420;
static constexpr int kHdrButtonId = 0x4844;
static void Log(const char* fmt,...) noexcept;
static HWND FindGameWindow() noexcept;
using GameHdrGet = bool (*)();
using GameHdrDisplayFn = bool (*)(HWND,bool);
using GameHdrSettingsFn = void (*)(bool,float,bool);
static HMODULE gameD3d{};
static GameHdrGet gameHdrGet{};
static GameHdrDisplayFn gameHdrDisplay{};
static GameHdrSettingsFn gameHdrSettings{};
static GameHdrSettingsFn originalGameHdrSettings{};
static std::atomic<bool> gameHdrControlReady{false};
static std::atomic<bool> protectGameHdr{false};
static std::atomic<int> pendingGameHdrSet{-1};
static std::atomic<bool> systemSetAttempted{false};
static std::atomic<bool> gameSetAttempted{false};
static std::atomic<bool> gameSetCompleted{false};
static std::atomic<bool> gameSetSucceeded{false};
static std::atomic<bool> lastGameHdr{false};
static std::atomic<bool> lastSystemHdr{false};
static std::atomic<bool> lastBridgeHdr{false};
static std::atomic<HWND> transitionGameWindow{nullptr};

static constexpr size_t kRvaFgSelection = 0xAA0A0;
static constexpr size_t kRvaAaCounter = 0xAB7B0;
static constexpr size_t kRvaPresentCounter = 0xAB7B8;
static constexpr size_t kRvaFgEnabledByApi = 0xAB9D0;
static constexpr size_t kRvaFgGeneratedSamples = 0xABB28;
static constexpr size_t kRvaHdrBridgeActive = 0xABCF0;

static unsigned int ReadFGSelectionRaw() noexcept {
    return core ? static_cast<unsigned int>(InterlockedCompareExchange(
        reinterpret_cast<volatile LONG*>(core+kRvaFgSelection),0,0)) : 0u;
}

static void WriteFGSelectionRaw(unsigned int selection,const char* reason) noexcept {
    if(!core)return;
    const unsigned int previous=static_cast<unsigned int>(InterlockedExchange(
        reinterpret_cast<volatile LONG*>(core+kRvaFgSelection),static_cast<LONG>(selection)));
    Log("HDR_BUTTON_FG_SELECTION_WRITE previous=%u target=%u reason=%s persisted=0",previous,selection,reason?reason:"none");
}

static bool ReadGameHdr(bool& value) noexcept {
    if(!gameHdrGet)return false;
    __try { value=gameHdrGet(); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { Log("HDR_BUTTON_GAME_GET_EXCEPTION code=0x%08lX",GetExceptionCode()); return false; }
}

static void HookedGameHdrSettings(bool enable,float value,bool force) noexcept {
    const bool protect=protectGameHdr.load(std::memory_order_acquire);
    const bool suppress=!enable && !force && protect;
    Log("HDR_BUTTON_GAME_SETTINGS_HOOK enable=%u value=%.3f force=%u protect=%u action=%s",
        unsigned(enable),double(value),unsigned(force),unsigned(protect),
        suppress?"suppress_nonforced_disable":"forward");
    if(suppress)return;
    if(originalGameHdrSettings)originalGameHdrSettings(enable,value,force);
}
static void LogHdrExports(HMODULE module) noexcept {
    if(!module)return;
    auto* base=reinterpret_cast<unsigned char*>(module);
    __try {
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        const auto& dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if(!dir.VirtualAddress || !dir.Size)return;
        auto* exp=reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base+dir.VirtualAddress);
        auto* names=reinterpret_cast<DWORD*>(base+exp->AddressOfNames);
        auto* ords=reinterpret_cast<WORD*>(base+exp->AddressOfNameOrdinals);
        auto* funcs=reinterpret_cast<DWORD*>(base+exp->AddressOfFunctions);
        unsigned logged=0;
        for(DWORD i=0;i<exp->NumberOfNames && logged<64;++i) {
            const char* name=reinterpret_cast<const char*>(base+names[i]);
            if(!name)continue;
            if(strstr(name,"HDR") || strstr(name,"Hdr") || strstr(name,"hdr")) {
                const DWORD rva=funcs[ords[i]];
                Log("HDR_BUTTON_GAME_EXPORT rva=0x%X name=%s",rva,name);
                ++logged;
            }
        }
        Log("HDR_BUTTON_GAME_EXPORT_SCAN names=%u hdr_exports=%u",exp->NumberOfNames,logged);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("HDR_BUTTON_GAME_EXPORT_SCAN_FAIL code=0x%08lX",GetExceptionCode());
    }
}
static bool ResolveGameHdrApi() noexcept {
    gameD3d=GetModuleHandleW(L"d3d_rmdwin10_f.dll");
    if(!gameD3d)return false;
    gameHdrGet=reinterpret_cast<GameHdrGet>(GetProcAddress(gameD3d,"?isHDREnabled@DeviceUtil@d3d@@SA_NXZ"));
    gameHdrDisplay=reinterpret_cast<GameHdrDisplayFn>(
        GetProcAddress(gameD3d,"?setHDREnabledDisplay@DeviceUtilDXGI@d3d@@SA_NPEAUHWND__@@_N@Z"));
    gameHdrSettings=reinterpret_cast<GameHdrSettingsFn>(
        GetProcAddress(gameD3d,"?setHDRSettings@DeviceUtil@d3d@@SAX_NM0@Z"));
    bool current=false;
    LogHdrExports(gameD3d);
    const bool getterWorks=gameHdrGet && ReadGameHdr(current);
    const bool settersWork=gameHdrDisplay && gameHdrSettings;
    bool hookReady=false;
    if(settersWork) {
        MH_STATUS hs=MH_CreateHook(reinterpret_cast<LPVOID>(gameHdrSettings),
            reinterpret_cast<LPVOID>(&HookedGameHdrSettings),
            reinterpret_cast<LPVOID*>(&originalGameHdrSettings));
        if(hs==MH_OK)hs=MH_EnableHook(reinterpret_cast<LPVOID>(gameHdrSettings));
        hookReady=hs==MH_OK;
        Log("HDR_BUTTON_GAME_SETTINGS_HOOK_INSTALL status=%s ready=%u target=%p trampoline=%p",
            MH_StatusToString(hs),unsigned(hookReady),gameHdrSettings,originalGameHdrSettings);
    }
    gameHdrControlReady.store(getterWorks && settersWork && hookReady);
    lastGameHdr.store(current);
    Log("HDR_BUTTON_GAME_API getter=%p getter_works=%u display_setter=%p settings_setter=%p settings_hook=%u control_ready=%u initial_game_hdr=%u",
        gameHdrGet,unsigned(getterWorks),gameHdrDisplay,gameHdrSettings,unsigned(hookReady),
        unsigned(getterWorks&&settersWork&&hookReady),unsigned(current));
    return getterWorks && settersWork && hookReady;
}
static bool SetGameHdr(bool enable) noexcept {
    if(!gameHdrControlReady.load() || !gameHdrSettings || !gameHdrDisplay)return false;
    protectGameHdr.store(enable,std::memory_order_release);
    HWND game=transitionGameWindow.load();
    if(!game || !IsWindow(game))game=FindGameWindow();
    bool displayResult=true;
    __try {
        // Exact native sequence observed from Control's own Display menu in R26:
        // enable  -> setHDRSettings(true,-1.0f,true), then setHDREnabledDisplay(hwnd,true)
        // disable -> setHDRSettings(false,-1.0f,true); no display setter call
        gameHdrSettings(enable,-1.0f,true);
        if(enable) displayResult=game && gameHdrDisplay(game,true);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("HDR_BUTTON_GAME_SET_EXCEPTION target=%u code=0x%08lX",unsigned(enable),GetExceptionCode());
        return false;
    }
    bool after=false;const bool readOk=ReadGameHdr(after);
    Log("HDR_BUTTON_GAME_NATIVE_SET target=%u hwnd=%p display_result=%u read_ok=%u immediate_game_hdr=%u value=-1 force=1",
        unsigned(enable),game,unsigned(displayResult),unsigned(readOk),unsigned(after));
    return displayResult;
}

struct DisplayTarget {
    bool valid{};
    bool hdrSupported{};
    bool hdrEnabled{};
    LUID adapter{};
    UINT32 targetId{};
    HMONITOR monitor{};
};

static uint32_t read32(size_t rva) noexcept {
    return static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(core+rva),0,0));
}
static uint64_t read64(size_t rva) noexcept {
    return static_cast<uint64_t>(InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(core+rva),0,0));
}

static void Log(const char* fmt,...) noexcept {
    if(logFile==INVALID_HANDLE_VALUE)return;
    char body[3072],line[3350];
    va_list ap;va_start(ap,fmt);vsnprintf(body,sizeof(body),fmt,ap);va_end(ap);
    LARGE_INTEGER q{};QueryPerformanceCounter(&q);
    int n=snprintf(line,sizeof(line),"qpc=%lld tick=%llu tid=%lu %s\r\n",
        q.QuadPart,GetTickCount64(),GetCurrentThreadId(),body);
    if(n<=0)return;
    DWORD wrote{};
    AcquireSRWLockExclusive(&logLock);
    WriteFile(logFile,line,static_cast<DWORD>(n<(int)sizeof(line)?n:sizeof(line)-1),&wrote,nullptr);
    ReleaseSRWLockExclusive(&logLock);
}

static bool IsProcessWindow(HWND hwnd) noexcept {
    if(!hwnd)return false;
    DWORD pid{};GetWindowThreadProcessId(hwnd,&pid);
    return pid==GetCurrentProcessId();
}

static HWND FindGameWindow() noexcept {
    HWND fg=GetForegroundWindow();
    if(IsProcessWindow(fg) && fg!=overlayWindow.load())return fg;
    struct Search { DWORD pid; HWND best; long long area; } s{GetCurrentProcessId(),nullptr,0};
    EnumWindows([](HWND w,LPARAM p)->BOOL{
        auto* s=reinterpret_cast<Search*>(p);
        DWORD pid{};GetWindowThreadProcessId(w,&pid);
        if(pid!=s->pid || !IsWindowVisible(w) || GetWindow(w,GW_OWNER))return TRUE;
        wchar_t cls[96]{};GetClassNameW(w,cls,96);
        if(wcscmp(cls,L"ControlFGOverlay_v1000")==0)return TRUE;
        RECT r{};if(!GetClientRect(w,&r))return TRUE;
        long long a=static_cast<long long>(r.right-r.left)*(r.bottom-r.top);
        if(a>s->area){s->best=w;s->area=a;}return TRUE;
    },reinterpret_cast<LPARAM>(&s));
    return s.best;
}

static bool ResolveDisplayTarget(HWND game,DisplayTarget& out) noexcept {
    out={};
    if(!game)return false;
    HMONITOR mon=MonitorFromWindow(game,MONITOR_DEFAULTTONEAREST);
    if(!mon)return false;
    MONITORINFOEXW mi{};mi.cbSize=sizeof(mi);
    if(!GetMonitorInfoW(mon,&mi))return false;

    UINT32 pathCount{},modeCount{};
    for(unsigned retry=0;retry<4;++retry) {
        if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&pathCount,&modeCount)!=ERROR_SUCCESS ||
           pathCount==0 || pathCount>128 || modeCount>4096)return false;
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
        std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
        LONG q=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&pathCount,paths.data(),&modeCount,modes.data(),nullptr);
        if(q==ERROR_INSUFFICIENT_BUFFER)continue;
        if(q!=ERROR_SUCCESS)return false;
        for(UINT32 i=0;i<pathCount;++i) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME src{};
            src.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            src.header.size=sizeof(src);
            src.header.adapterId=paths[i].sourceInfo.adapterId;
            src.header.id=paths[i].sourceInfo.id;
            if(DisplayConfigGetDeviceInfo(&src.header)!=ERROR_SUCCESS)continue;
            if(_wcsicmp(src.viewGdiDeviceName,mi.szDevice)!=0)continue;

            DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO ac{};
            ac.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
            ac.header.size=sizeof(ac);
            ac.header.adapterId=paths[i].targetInfo.adapterId;
            ac.header.id=paths[i].targetInfo.id;
            if(DisplayConfigGetDeviceInfo(&ac.header)!=ERROR_SUCCESS)return false;
            out.valid=true;
            out.hdrSupported=ac.advancedColorSupported!=0;
            out.hdrEnabled=ac.advancedColorEnabled!=0;
            out.adapter=paths[i].targetInfo.adapterId;
            out.targetId=paths[i].targetInfo.id;
            out.monitor=mon;
            return true;
        }
        return false;
    }
    return false;
}

struct SetAdvancedColorStatePacket {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header{};
    union {
        struct {
            UINT32 enableAdvancedColor : 1;
            UINT32 reserved : 31;
        };
        UINT32 value;
    };
};

static bool SetWindowsHdrDirect(const DisplayTarget& d,bool enable) noexcept {
    if(!d.valid || !d.hdrSupported)return false;
    SetAdvancedColorStatePacket packet{};
    packet.header.type=static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(10); // DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE
    packet.header.size=sizeof(packet);
    packet.header.adapterId=d.adapter;
    packet.header.id=d.targetId;
    packet.value=enable?1u:0u;
    const LONG result=DisplayConfigSetDeviceInfo(&packet.header);
    Log("HDR_BUTTON_WINDOWS_DIRECT_SET target=%u result=%ld adapter=%08lX:%08lX target_id=%u api=DisplayConfigSetDeviceInfo type=10 synthetic_hotkey=0",
        unsigned(enable),result,static_cast<unsigned long>(d.adapter.HighPart),d.adapter.LowPart,d.targetId);
    return result==ERROR_SUCCESS;
}

static void UpdateButtonVisual() noexcept {
    HWND b=buttonWindow.load();
    if(b)InvalidateRect(b,nullptr,TRUE);
}

static void SetPhase(Phase phase,const char* why=nullptr) noexcept {
    const uint64_t now=GetTickCount64();
    AcquireSRWLockExclusive(&stateLock);
    transition.phase=phase;
    transition.phaseStartedMs=now;
    if(why)transition.error=why;
    ReleaseSRWLockExclusive(&stateLock);
    Log("HDR_BUTTON_STATE phase=%s reason=%s tick=%llu",PhaseName(phase),why?why:"none",now);
    UpdateButtonVisual();
}

static State SnapshotState() noexcept {
    AcquireSRWLockShared(&stateLock);
    State s=transition;
    ReleaseSRWLockShared(&stateLock);
    return s;
}

static void FailTransitionAndRestoreFG(const char* why) noexcept {
    const uint64_t now=GetTickCount64();
    State s=SnapshotState();
    protectGameHdr.store(false,std::memory_order_release);
    pendingGameHdrSet.store(-1,std::memory_order_release);
    gameSetAttempted.store(false,std::memory_order_release);
    gameSetCompleted.store(false,std::memory_order_release);
    gameSetSucceeded.store(false,std::memory_order_release);
    WriteFGSelectionRaw(s.sourceSelection,"failure_unwind_restore_saved_selection");
    AcquireSRWLockExclusive(&stateLock);
    transition.fail(why,now);
    transition.failureUnwindStarted=true;
    ReleaseSRWLockExclusive(&stateLock);
    Log("HDR_BUTTON_FAIL reason=%s source_selection=%u restored_selection=%u present=%llu fg_api_enabled=%u system_hdr=%u game_hdr=%u bridge_hdr=%u unwind=fail_open_no_fg_latch",
        why?why:"unknown",s.sourceSelection,ReadFGSelectionRaw(),read64(kRvaPresentCounter),read32(kRvaFgEnabledByApi),
        unsigned(lastSystemHdr.load()),unsigned(lastGameHdr.load()),unsigned(lastBridgeHdr.load()));
    UpdateButtonVisual();
}

static bool StartButtonTransition() noexcept {
    if(!ready.load())return false;
    if(read32(0xABB10)==0 || read32(0xABB14)!=0) {
        Log("HDR_BUTTON_REJECT reason=rtx50_only gpu_known=%u rtx40=%u",read32(0xABB10),read32(0xABB14));
        return false;
    }
    if(!gameHdrControlReady.load()) {
        Log("HDR_BUTTON_REJECT reason=game_hdr_setter_unresolved safe_no_side_effects=1");
        return false;
    }

    HWND game=FindGameWindow();
    DisplayTarget d{};
    bool gameHdr=false;
    if(!ResolveDisplayTarget(game,d) || !d.hdrSupported || !ReadGameHdr(gameHdr)) {
        currentHdrKnown.store(d.valid);
        displayHdrSupported.store(d.hdrSupported);
        Log("HDR_BUTTON_REJECT reason=hdr_state_unavailable game=%p display_valid=%u display_supported=%u",
            game,unsigned(d.valid),unsigned(d.hdrSupported));
        UpdateButtonVisual();
        return false;
    }

    const uint64_t now=GetTickCount64();
    AcquireSRWLockExclusive(&stateLock);
    if(transition.active()) {
        ReleaseSRWLockExclusive(&stateLock);
        Log("HDR_BUTTON_REJECT reason=transition_busy");
        return false;
    }

    transition=State{};
    transition.sourceHdr=d.hdrEnabled;
    transition.targetHdr=!d.hdrEnabled;
    transition.sourceSelection=ReadFGSelectionRaw();
    transition.sourceFgWasEnabled=transition.sourceSelection!=0 && read32(kRvaFgEnabledByApi)!=0;
    transition.startedMs=now;
    transition.phaseStartedMs=now;
    transition.basePresent=read64(kRvaPresentCounter);
    transition.baseGeneratedCount=read64(kRvaFgGeneratedSamples);
    transition.freshAaBase=read64(kRvaAaCounter);
    transition.freshPresentBase=transition.basePresent;
    transition.phase=transition.sourceSelection==0?Phase::SetSystemHdr:Phase::RequestOff;
    transitionGameWindow.store(game);
    State s=transition;
    ReleaseSRWLockExclusive(&stateLock);

    protectGameHdr.store(false,std::memory_order_release);
    currentHdr.store(d.hdrEnabled && gameHdr && (read32(kRvaHdrBridgeActive)!=0));
    currentHdrKnown.store(true);
    displayHdrSupported.store(true);
    lastSystemHdr.store(d.hdrEnabled);
    lastGameHdr.store(gameHdr);
    lastBridgeHdr.store(read32(kRvaHdrBridgeActive)!=0);
    systemSetAttempted.store(false);
    gameSetAttempted.store(false);
    gameSetCompleted.store(false);
    gameSetSucceeded.store(false);
    pendingGameHdrSet.store(-1);

    Log("HDR_BUTTON_REQUEST source_windows_hdr=%u target_hdr=%u game_hdr=%u bridge_hdr=%u saved_fg_selection=%u fg_api_enabled=%u present=%llu aa=%llu authority=windows_display direct_display_api=1 synthetic_hotkey=0 sidecar_fg_hold=0",
        unsigned(d.hdrEnabled),unsigned(s.targetHdr),unsigned(gameHdr),unsigned(read32(kRvaHdrBridgeActive)!=0),
        s.sourceSelection,read32(kRvaFgEnabledByApi),s.basePresent,s.freshAaBase);
    UpdateButtonVisual();
    return true;
}

static void OnFrame() noexcept {
    if(ready.load()) {
        State active=SnapshotState();
        if((active.active() && active.phase!=Phase::WaitFgResume) || active.phase==Phase::Failed)
            WriteTransitionFgHold(true);
    }
    if(ready.load() && pendingBegin.exchange(false)) {
        AcquireSRWLockExclusive(&stateLock);
        if(transition.phase==Phase::RequestOff) {
            const uint64_t next=read64(0xAB7B8)+1;
            const bool ok=beginHdrHardReset && beginHdrHardReset(next,"overlay_hdr_button_pre_system_change");
            if(ok) {
                transition.phase=Phase::WaitOffCommit;
                Log("HDR_BUTTON_FG_OFF_ARMED present=%llu selection_preserved=%u",next,transition.sourceSelection);
            } else {
                transition.fail("hard_reset_arm_failed");
                Log("HDR_BUTTON_FAIL reason=hard_reset_arm_failed");
            }
        }
        ReleaseSRWLockExclusive(&stateLock);
        UpdateButtonVisual();
    }
    const int gameSet=pendingGameHdrSet.exchange(-1);
    if(gameSet>=0) {
        const bool ok=SetGameHdr(gameSet!=0);
        if(!ok) {
            AcquireSRWLockExclusive(&stateLock);
            if(transition.active())transition.fail("game_hdr_setter_failed");
            ReleaseSRWLockExclusive(&stateLock);
            Log("HDR_BUTTON_FAIL reason=game_hdr_setter_failed target=%d hold_fg_off=1",gameSet);
            UpdateButtonVisual();
        }
    }
    originalFrame();
}

static LRESULT DrawHdrButton(const DRAWITEMSTRUCT* di) noexcept {
    if(!di || di->CtlID!=kHdrButtonId)return FALSE;
    State s=SnapshotState();
    const bool supported=displayHdrSupported.load() && gameHdrControlReady.load();
    const bool hdr=currentHdr.load();
    RECT r=di->rcItem;
    HDC dc=di->hDC;
    const bool pending=s.active();
    const bool failed=s.phase==Phase::Failed;
    HBRUSH fill=CreateSolidBrush(pending?RGB(52,20,20):(hdr?RGB(246,246,246):RGB(0,0,0)));
    FillRect(dc,&r,fill);DeleteObject(fill);
    HPEN pen=CreatePen(PS_SOLID,1,failed?RGB(243,45,45):(pending?RGB(243,45,45):RGB(92,92,92)));
    HGDIOBJ oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc,r.left,r.top,r.right,r.bottom);
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
    SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,hdr&&!pending?RGB(12,12,12):RGB(238,238,238));
    HFONT font=CreateFontW(-16,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Bahnschrift SemiCondensed");
    HGDIOBJ oldFont=SelectObject(dc,font);
    const wchar_t* text=ButtonText(s.phase,hdr,supported);
    DrawTextW(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc,oldFont);DeleteObject(font);
    if(di->itemState&ODS_FOCUS)DrawFocusRect(dc,&r);
    return TRUE;
}

static LRESULT CALLBACK OverlaySubclass(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) noexcept {
    if(msg==kAttachButtonMessage) {
        if(!buttonWindow.load()) {
            HWND b=CreateWindowExW(0,L"BUTTON",L"HDR",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                448,128,108,40,hwnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHdrButtonId)),selfModule,nullptr);
            if(b) {
                buttonWindow.store(b);
                Log("HDR_BUTTON_OVERLAY_ATTACH overlay=%p button=%p rect=448,128,108,40",hwnd,b);
                UpdateButtonVisual();
            } else Log("HDR_BUTTON_OVERLAY_ATTACH_FAIL error=%lu",GetLastError());
        }
        return 0;
    }
    if(msg==WM_COMMAND && LOWORD(wp)==kHdrButtonId && HIWORD(wp)==BN_CLICKED) {
        StartButtonTransition();
        return 0;
    }
    if(msg==WM_DRAWITEM) {
        auto* di=reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if(di && di->CtlID==kHdrButtonId)return DrawHdrButton(di);
    }
    return CallWindowProcW(originalOverlayProc,hwnd,msg,wp,lp);
}

static BOOL CALLBACK FindOverlayEnum(HWND hwnd,LPARAM param) noexcept {
    DWORD pid{};GetWindowThreadProcessId(hwnd,&pid);
    if(pid!=GetCurrentProcessId())return TRUE;
    wchar_t cls[96]{};GetClassNameW(hwnd,cls,96);
    if(wcscmp(cls,L"ControlFGOverlay_v1000")!=0)return TRUE;
    *reinterpret_cast<HWND*>(param)=hwnd;return FALSE;
}

static void AttachOverlayButton() noexcept {
    if(overlayWindow.load())return;
    HWND overlay{};
    EnumWindows(FindOverlayEnum,reinterpret_cast<LPARAM>(&overlay));
    if(!overlay)return;
    SetLastError(ERROR_SUCCESS);
    auto old=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(overlay,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&OverlaySubclass)));
    if(!old) {
        DWORD e=GetLastError();
        if(e){Log("HDR_BUTTON_SUBCLASS_FAIL overlay=%p error=%lu",overlay,e);return;}
    }
    originalOverlayProc=old;
    overlayWindow.store(overlay);
    PostMessageW(overlay,kAttachButtonMessage,0,0);
    Log("HDR_BUTTON_SUBCLASS_OK overlay=%p original_proc=%p",overlay,old);
}

static std::string HashFile(const std::wstring& path) {
    HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE)return {};
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};std::string out;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
       BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0) {
        unsigned char buffer[65536];DWORD count{};bool good=true;
        while(true){if(!ReadFile(f,buffer,sizeof(buffer),&count,nullptr)){good=false;break;}if(!count)break;
            if(BCryptHashData(hash,buffer,count,0)<0){good=false;break;}}
        if(good && BCryptFinishHash(hash,digest,32,0)>=0) {
            char hex[65]{};
            for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);
            out=hex;
        }
    }
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return out;
}

static bool InstallCoreHook() noexcept {
    static const unsigned char frameSig[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    if(memcmp(core+0x1DF60,frameSig,sizeof(frameSig))!=0) {
        Log("HDR_BUTTON_INSTALL_FAIL reason=frame_signature");
        return false;
    }
    if(MH_Initialize()!=MH_OK)return false;
    auto r=MH_CreateHook(core+0x1DF60,reinterpret_cast<void*>(&OnFrame),reinterpret_cast<void**>(&originalFrame));
    if(r!=MH_OK){Log("HDR_BUTTON_INSTALL_FAIL reason=minhook_create status=%s",MH_StatusToString(r));return false;}
    if(MH_EnableHook(core+0x1DF60)!=MH_OK)return false;
    beginHdrHardReset=reinterpret_cast<bool(*)(uint64_t,const char*)>(core+0xCDC0);
    return true;
}

static void PollTransition() noexcept {
    State s=SnapshotState();
    if(!s.active())return;
    const uint64_t now=GetTickCount64();
    if(now-s.startedMs>15000) {
        AcquireSRWLockExclusive(&stateLock);
        if(transition.active())transition.fail("timeout");
        ReleaseSRWLockExclusive(&stateLock);
        WriteTransitionFgHold(true);
        Log("HDR_BUTTON_FAIL reason=timeout phase=%s enabled=%u present=%llu frees=%llu bridge=%u hold_fg_off=1",
            PhaseName(s.phase),read32(0xAB9D0),read64(0xAB7B8),read64(0xABC70),read32(0xABCF0));
        UpdateButtonVisual();return;
    }

    if(s.phase==Phase::WaitOffCommit) {
        const bool off=read32(0xAB9D0)==0;
        const uint64_t present=read64(0xAB7B8),frees=read64(0xABC70);
        if(off && frees>s.baseFreeCount && present>s.basePresent) {
            Log("HDR_BUTTON_FG_OFF_CONFIRMED present=%llu free_count=%llu stage_ready_for_system_hdr=1",present,frees);
            SetPhase(Phase::SetSystemHdr);
        }
        return;
    }

    if(s.phase==Phase::SetSystemHdr || s.phase==Phase::WaitGameHdr) {
        DisplayTarget d{};
        bool gameHdr=false;
        if(!ResolveDisplayTarget(transitionGameWindow.load(),d) || !d.hdrSupported || !ReadGameHdr(gameHdr)) {
            SetPhase(Phase::Failed,"hdr_state_lost");return;
        }
        const bool bridgeHdr=read32(0xABCF0)!=0;
        lastSystemHdr.store(d.hdrEnabled);lastGameHdr.store(gameHdr);lastBridgeHdr.store(bridgeHdr);
        currentHdr.store(d.hdrEnabled && gameHdr && bridgeHdr);
        currentHdrKnown.store(true);displayHdrSupported.store(true);

        if(s.targetHdr) {
            // Enable combined HDR:
            // 1) FG remains forcibly held off.
            // 2) Windows HDR on.
            // 3) Native Control sequence observed in R26.
            // 4) Require Windows + game getter + HDR bridge all ON before releasing FG.
            if(!d.hdrEnabled) {
                if(!systemToggleSent.load()) {
                    if(!HdrShortcutKeysReleased()) return;
                    if(!SendWindowsHdrShortcut()) { SetPhase(Phase::Failed,"WinAltB_SendInput_failed"); return; }
                    systemToggleSent.store(true);
                    Log("HDR_BUTTON_SYSTEM_ENABLE_SENT");
                }
                SetPhase(Phase::WaitGameHdr);return;
            }
            if(!gameHdr) {
                if(!gameSetAttempted.exchange(true)) {
                    pendingGameHdrSet.store(1);
                    Log("HDR_BUTTON_GAME_ENABLE_QUEUED system_hdr=1 bridge_hdr=%u native_sequence=setHDRSettings_then_setHDREnabledDisplay",
                        unsigned(bridgeHdr));
                }
                SetPhase(Phase::WaitGameHdr);return;
            }
            if(!bridgeHdr) { SetPhase(Phase::WaitGameHdr);return; }

            Log("HDR_BUTTON_COMBINED_HDR_CONFIRMED target=1 system=1 game=1 bridge=1 present=%llu",
                read64(0xAB7B8));
            WriteTransitionFgHold(false);
            Log("HDR_BUTTON_FG_HOLD enabled=0 reason=combined_hdr_ready target=1");
            SetPhase(read32(0xAA0A0)==0?Phase::Complete:Phase::WaitFgResume);return;
        } else {
            // Disable combined HDR:
            // 1) Native Control HDR off while Windows HDR remains available.
            // 2) Require game getter + bridge OFF.
            // 3) Windows HDR off.
            // 4) Require all three OFF before releasing FG.
            if(gameHdr || bridgeHdr) {
                if(!gameSetAttempted.exchange(true)) {
                    pendingGameHdrSet.store(0);
                    Log("HDR_BUTTON_GAME_DISABLE_QUEUED system_hdr=%u game_hdr=%u bridge_hdr=%u native_sequence=setHDRSettings_only",
                        unsigned(d.hdrEnabled),unsigned(gameHdr),unsigned(bridgeHdr));
                }
                SetPhase(Phase::WaitGameHdr);return;
            }
            if(d.hdrEnabled) {
                if(!systemToggleSent.load()) {
                    if(!HdrShortcutKeysReleased()) return;
                    if(!SendWindowsHdrShortcut()) { SetPhase(Phase::Failed,"WinAltB_SendInput_failed"); return; }
                    systemToggleSent.store(true);
                    Log("HDR_BUTTON_SYSTEM_DISABLE_SENT game_hdr=0 bridge_hdr=0");
                }
                SetPhase(Phase::WaitGameHdr);return;
            }

            Log("HDR_BUTTON_COMBINED_HDR_CONFIRMED target=0 system=0 game=0 bridge=0 present=%llu",
                read64(0xAB7B8));
            WriteTransitionFgHold(false);
            Log("HDR_BUTTON_FG_HOLD enabled=0 reason=combined_hdr_ready target=0");
            SetPhase(read32(0xAA0A0)==0?Phase::Complete:Phase::WaitFgResume);return;
        }
    }

    if(s.phase==Phase::WaitFgResume) {
        const uint32_t selected=read32(0xAA0A0);
        if(selected==0) {SetPhase(Phase::Complete);return;}
        if(read32(0xAB9D0)!=0 && read64(0xABB28)>s.baseGeneratedCount) {
            Log("HDR_BUTTON_FG_RESUMED selection=%u present=%llu generated_samples=%llu bridge_hdr=%u",
                selected,read64(0xAB7B8),read64(0xABB28),read32(0xABCF0));
            SetPhase(Phase::Complete);
        }
        return;
    }
}

static DWORD WINAPI Worker(void*) noexcept {
    wchar_t exe[32768]{},own[32768]{};
    GetModuleFileNameW(nullptr,exe,32768);
    const wchar_t* leaf=wcsrchr(exe,L'\\');leaf=leaf?leaf+1:exe;
    if(_wcsicmp(leaf,L"Control_DX12.exe")!=0)return 0;

    wchar_t appdata[32768]{};
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA",appdata,32768))return 0;
    std::wstring logDir=std::wstring(appdata)+L"\\ControlFGProbe";
    CreateDirectoryW(logDir.c_str(),nullptr);
    SYSTEMTIME st{};GetSystemTime(&st);
    wchar_t name[180]{};
    swprintf_s(name,L"\\hdr-button-R27-%04u%02u%02u-%02u%02u%02u-%lu.log",
        st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,GetCurrentProcessId());
    logFile=CreateFileW((logDir+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);

    GetModuleFileNameW(selfModule,own,32768);
    std::wstring directory(own);directory=directory.substr(0,directory.find_last_of(L"\\/"));
    std::wstring corePath=directory+L"\\dxgi.dll";
    std::string hash=HashFile(corePath);
    Log("HDR_BUTTON_BUILD version=R27 expected_core_sha256=%s actual_core_sha256=%s ui=F10_overlay_child_button",
        kExpectedCoreSha256,hash.c_str());
    if(hash!=kExpectedCoreSha256){Log("HDR_BUTTON_INSTALL_FAIL reason=core_hash");return 0;}
    core=reinterpret_cast<unsigned char*>(GetModuleHandleW(corePath.c_str()));
    if(!core){Log("HDR_BUTTON_INSTALL_FAIL reason=core_module");return 0;}
    if(!InstallCoreHook()){Log("HDR_BUTTON_INSTALL_FAIL reason=core_hook");return 0;}
    ResolveGameHdrApi();
    ready.store(true);
    Log("HDR_BUTTON_READY controller=combined_Windows_and_Control_HDR game_hdr_control_ready=%u sequence=FG_off_then_ordered_system_and_game_HDR_then_FG_resume",
        unsigned(gameHdrControlReady.load()));

    uint64_t completeSince=0;
    while(true) {
        AttachOverlayButton();

        if(!SnapshotState().active()) {
            DisplayTarget d{};
            bool gameHdr=false;
            if(ResolveDisplayTarget(FindGameWindow(),d)) {
                const bool gameKnown=ReadGameHdr(gameHdr);
                const bool bridgeHdr=read32(0xABCF0)!=0;
                lastSystemHdr.store(d.hdrEnabled);lastGameHdr.store(gameHdr);lastBridgeHdr.store(bridgeHdr);
                currentHdr.store(gameKnown && d.hdrEnabled && gameHdr && bridgeHdr);
                currentHdrKnown.store(gameKnown);
                displayHdrSupported.store(d.hdrSupported);
            }
        }

        PollTransition();

        State s=SnapshotState();
        if(s.phase==Phase::Complete) {
            if(!completeSince)completeSince=GetTickCount64();
            if(GetTickCount64()-completeSince>800) {
                AcquireSRWLockExclusive(&stateLock);
                transition=State{};
                ReleaseSRWLockExclusive(&stateLock);
                WriteTransitionFgHold(false);
                transitionGameWindow.store(nullptr);
                completeSince=0;
                UpdateButtonVisual();
                Log("HDR_BUTTON_IDLE restored=1");
            }
        } else completeSince=0;

        UpdateButtonVisual();
        Sleep(50);
    }
}

extern "C" __declspec(dllexport) void WINAPI ControlFGHDRButton_Bootstrap(){}
extern "C" __declspec(dllexport) unsigned WINAPI ControlFGHDRButton_Version(){return 0x00270001;}

BOOL WINAPI DllMain(HINSTANCE mod,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        selfModule=mod;DisableThreadLibraryCalls(mod);
        HANDLE t=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);
        if(t)CloseHandle(t);
    }
    return TRUE;
}
