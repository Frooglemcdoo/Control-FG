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
using BeginHdrHardResetFn = bool (*)(uint64_t,const char*);
static BeginHdrHardResetFn beginHdrHardReset{};
static BeginHdrHardResetFn originalBeginHdrHardReset{};
static HANDLE logFile=INVALID_HANDLE_VALUE;
static SRWLOCK logLock=SRWLOCK_INIT;
static SRWLOCK stateLock=SRWLOCK_INIT;
static State transition{};
static std::atomic<bool> ready{false};
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
static std::atomic<bool> hardResetAttempted{false};
static std::atomic<int> lastHardResetDirection{0};
static std::atomic<unsigned long long> overlapResetRebases{0};
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
static constexpr size_t kRvaHdrHardResetStage = 0xABC50;
static constexpr size_t kRvaHdrHardResetFreeCount = 0xABC70;
static constexpr size_t kRvaHdrHardResetResetBridgeGeneration = 0xABC58;
static constexpr size_t kRvaHdrHardResetSerial = 0xABC60;
static constexpr size_t kRvaBridgeGeneration = 0xABD50;
static constexpr size_t kRvaSettledBridgeGeneration = 0xABBF0;
static constexpr size_t kRvaBeginHdrHardReset = 0xCDC0;

static constexpr size_t kRvaFreshCountCap = 0x414B4;
static constexpr size_t kRvaFreshRequiredLog = 0x41513;
static constexpr size_t kRvaFreshSettleThreshold = 0x41569;
static constexpr size_t kRvaNoBridgeFreshCountCap = 0x4169B;
static constexpr size_t kRvaNoBridgeFreshSettleThreshold = 0x41742;

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

static bool PatchThreeFrameTransitionWarmup() noexcept {
    struct Patch {
        size_t rva;
        const unsigned char* expected;
        const unsigned char* replacement;
        size_t size;
        const char* label;
    };
    static const unsigned char e1[]={0x83,0xF9,0x02}, r1[]={0x83,0xF9,0x03};
    static const unsigned char e2[]={0xC7,0x44,0x24,0x20,0x02,0x00,0x00,0x00},
                               r2[]={0xC7,0x44,0x24,0x20,0x03,0x00,0x00,0x00};
    static const unsigned char e3[]={0x41,0x83,0xFE,0x02}, r3[]={0x41,0x83,0xFE,0x03};
    static const unsigned char e4[]={0x83,0xF9,0x02}, r4[]={0x83,0xF9,0x03};
    static const unsigned char e5[]={0x41,0x83,0xFF,0x02}, r5[]={0x41,0x83,0xFF,0x03};
    const Patch patches[]={
        {kRvaFreshCountCap,e1,r1,sizeof(e1),"transition_fresh_count_cap_3"},
        {kRvaFreshRequiredLog,e2,r2,sizeof(e2),"transition_required_frames_log_3"},
        {kRvaFreshSettleThreshold,e3,r3,sizeof(e3),"transition_settle_threshold_3"},
        {kRvaNoBridgeFreshCountCap,e4,r4,sizeof(e4),"no_bridge_fresh_count_cap_3"},
        {kRvaNoBridgeFreshSettleThreshold,e5,r5,sizeof(e5),"no_bridge_settle_threshold_3"}
    };
    for(const auto& p:patches) {
        if(memcmp(core+p.rva,p.expected,p.size)!=0) {
            Log("R33_WARMUP_PATCH_FAIL reason=signature label=%s rva=0x%zX",p.label,p.rva);
            return false;
        }
    }
    for(const auto& p:patches) {
        DWORD oldProtect=0;
        unsigned char* target=core+p.rva;
        if(!VirtualProtect(target,p.size,PAGE_EXECUTE_READWRITE,&oldProtect)) {
            Log("R33_WARMUP_PATCH_FAIL reason=VirtualProtect label=%s error=%lu",p.label,GetLastError());
            return false;
        }
        memcpy(target,p.replacement,p.size);
        FlushInstructionCache(GetCurrentProcess(),target,p.size);
        DWORD ignored=0;
        VirtualProtect(target,p.size,oldProtect,&ignored);
        if(memcmp(target,p.replacement,p.size)!=0) {
            Log("R33_WARMUP_PATCH_FAIL reason=verify label=%s",p.label);
            return false;
        }
        Log("R33_WARMUP_PATCH_OK label=%s rva=0x%zX",p.label,p.rva);
    }
    Log("R33_TRANSITION_POLICY hard_dlssg_free=1 recomposition=restored ring_slots=3 fresh_frames_required=3 camera_reset=control_reset_signal steady_state_hdr_unchanged=1 steady_state_sdr_unchanged=1");
    return true;
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

static int HdrTransitionDirection(const char* reason) noexcept {
    if(!reason)return 0;
    if(strstr(reason,"sdr_to_hdr"))return 1;
    if(strstr(reason,"hdr_to_sdr"))return -1;
    return 0;
}

static bool HookedBeginHdrHardReset(uint64_t present,const char* reason) noexcept {
    if(!originalBeginHdrHardReset)return false;

    const unsigned int stageBefore=read32(kRvaHdrHardResetStage);
    const uint64_t bridgeGeneration=read64(kRvaBridgeGeneration);
    const uint64_t resetGeneration=read64(kRvaHdrHardResetResetBridgeGeneration);
    const uint64_t settledGeneration=read64(kRvaSettledBridgeGeneration);
    const uint64_t serialBefore=read64(kRvaHdrHardResetSerial);
    const int direction=HdrTransitionDirection(reason);
    const int previousDirection=lastHardResetDirection.load(std::memory_order_acquire);

    bool rebased=false;
    if(stageBefore==2 && bridgeGeneration!=resetGeneration &&
       settledGeneration!=bridgeGeneration &&
       direction!=0 && previousDirection!=0 && direction!=previousDirection) {
        const LONG previousStage=InterlockedCompareExchange(
            reinterpret_cast<volatile LONG*>(core+kRvaHdrHardResetStage),0,2);
        rebased=previousStage==2;
        if(rebased) {
            const auto ordinal=++overlapResetRebases;
            Log("R33_OVERLAP_RESET_REBASE present=%llu reason=%s direction=%d previous_direction=%d serial_before=%llu stage_before=2 reset_generation=%llu current_generation=%llu settled_generation=%llu action=restart_hard_reset_and_refree ordinal=%llu",
                present,reason?reason:"none",direction,previousDirection,serialBefore,
                resetGeneration,bridgeGeneration,settledGeneration,ordinal);
        }
    }

    const unsigned int callStage=read32(kRvaHdrHardResetStage);
    const bool result=originalBeginHdrHardReset(present,reason);
    const unsigned int stageAfter=read32(kRvaHdrHardResetStage);
    const uint64_t serialAfter=read64(kRvaHdrHardResetSerial);
    const uint64_t resetAfter=read64(kRvaHdrHardResetResetBridgeGeneration);

    if(result && direction!=0 && callStage==0 && stageAfter==1) {
        lastHardResetDirection.store(direction,std::memory_order_release);
    }

    if(rebased || callStage==0) {
        Log("R33_HARD_RESET_BEGIN_OBSERVED present=%llu reason=%s result=%u direction=%d stage_before=%u stage_after=%u serial_before=%llu serial_after=%llu reset_generation=%llu current_generation=%llu overlap_rebase=%u",
            present,reason?reason:"none",unsigned(result),direction,callStage,stageAfter,
            serialBefore,serialAfter,resetAfter,bridgeGeneration,unsigned(rebased));
    }
    return result;
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
    UINT32 value{};
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
    transition.baseFreeCount=read64(kRvaHdrHardResetFreeCount);
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
    hardResetAttempted.store(false);
    pendingGameHdrSet.store(-1);

    Log("HDR_BUTTON_REQUEST source_windows_hdr=%u target_hdr=%u game_hdr=%u bridge_hdr=%u saved_fg_selection=%u fg_api_enabled=%u present=%llu aa=%llu free_count=%llu authority=windows_display direct_display_api=1 synthetic_hotkey=0 hard_reset_before_domain=1 user_selection_preserved=1",
        unsigned(d.hdrEnabled),unsigned(s.targetHdr),unsigned(gameHdr),unsigned(read32(kRvaHdrBridgeActive)!=0),
        s.sourceSelection,read32(kRvaFgEnabledByApi),s.basePresent,s.freshAaBase,s.baseFreeCount);
    UpdateButtonVisual();
    return true;
}

static void OnFrame() noexcept {
    const int gameSet=pendingGameHdrSet.exchange(-1);
    if(gameSet>=0) {
        const bool ok=SetGameHdr(gameSet!=0);
        gameSetSucceeded.store(ok,std::memory_order_release);
        gameSetCompleted.store(true,std::memory_order_release);
        Log("HDR_BUTTON_GAME_SET_COMPLETE target=%d success=%u present=%llu",
            gameSet,unsigned(ok),read64(kRvaPresentCounter));
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
    static const unsigned char hardResetSig[]={0x48,0x89,0x6c,0x24,0x20,0x56,0x48,0x83,0xec,0x30};
    if(memcmp(core+0x1DF60,frameSig,sizeof(frameSig))!=0) {
        Log("HDR_BUTTON_INSTALL_FAIL reason=frame_signature");
        return false;
    }
    if(memcmp(core+kRvaBeginHdrHardReset,hardResetSig,sizeof(hardResetSig))!=0) {
        Log("HDR_BUTTON_INSTALL_FAIL reason=hard_reset_signature");
        return false;
    }
    if(!PatchThreeFrameTransitionWarmup())return false;
    if(MH_Initialize()!=MH_OK)return false;
    auto r=MH_CreateHook(core+kRvaBeginHdrHardReset,reinterpret_cast<void*>(&HookedBeginHdrHardReset),
        reinterpret_cast<void**>(&originalBeginHdrHardReset));
    if(r!=MH_OK){Log("HDR_BUTTON_INSTALL_FAIL reason=hard_reset_hook_create status=%s",MH_StatusToString(r));return false;}
    r=MH_CreateHook(core+0x1DF60,reinterpret_cast<void*>(&OnFrame),reinterpret_cast<void**>(&originalFrame));
    if(r!=MH_OK){Log("HDR_BUTTON_INSTALL_FAIL reason=frame_hook_create status=%s",MH_StatusToString(r));return false;}
    if(MH_EnableHook(core+kRvaBeginHdrHardReset)!=MH_OK)return false;
    if(MH_EnableHook(core+0x1DF60)!=MH_OK)return false;
    beginHdrHardReset=&HookedBeginHdrHardReset;
    Log("R33_HARD_RESET_READY entry_rva=0x%zX stage_rva=0x%zX free_count_rva=0x%zX bridge_generation_rva=0x%zX settled_generation_rva=0x%zX policy=opposite_unsettled_transition_restarts_reset_and_refrees",
        kRvaBeginHdrHardReset,kRvaHdrHardResetStage,kRvaHdrHardResetFreeCount,
        kRvaBridgeGeneration,kRvaSettledBridgeGeneration);
    return true;
}

static void PollTransition() noexcept {
    State s=SnapshotState();
    if(!s.active())return;

    const uint64_t now=GetTickCount64();
    if(TransitionTimedOut(s.startedMs,now)) {
        FailTransitionAndRestoreFG("timeout_60s_saved_fg_restored");
        return;
    }

    if(s.phase==Phase::RequestOff) {
        if(!beginHdrHardReset) {
            FailTransitionAndRestoreFG("hard_reset_entry_unavailable");
            return;
        }
        if(!hardResetAttempted.exchange(true)) {
            const uint64_t present=read64(kRvaPresentCounter);
            const bool started=beginHdrHardReset(present,"r33_button_pre_domain");
            Log("R33_HARD_RESET_REQUEST started=%u saved_selection=%u present=%llu api_enabled=%u stage=%u free_count=%llu action=wait_for_slFreeResources",
                unsigned(started),s.sourceSelection,present,read32(kRvaFgEnabledByApi),read32(kRvaHdrHardResetStage),
                read64(kRvaHdrHardResetFreeCount));
            if(!started && read32(kRvaHdrHardResetStage)==0) {
                FailTransitionAndRestoreFG("hard_reset_request_rejected");
                return;
            }
        }
        SetPhase(Phase::WaitOffCommit);
        return;
    }

    if(s.phase==Phase::WaitOffCommit) {
        const unsigned int selection=ReadFGSelectionRaw();
        const unsigned int enabled=read32(kRvaFgEnabledByApi);
        const unsigned int stage=read32(kRvaHdrHardResetStage);
        const uint64_t frees=read64(kRvaHdrHardResetFreeCount);
        const uint64_t present=read64(kRvaPresentCounter);
        if(selection!=s.sourceSelection) {
            FailTransitionAndRestoreFG("fg_selection_changed_during_hard_reset");
            return;
        }
        if(stage==2 && enabled==0 && frees>s.baseFreeCount && present>s.basePresent) {
            Log("R33_HARD_RESET_CONFIRMED selection_preserved=%u api_enabled=0 stage=2 present=%llu free_count=%llu freed_delta=%llu action=set_windows_hdr_direct",
                selection,present,frees,frees-s.baseFreeCount);
            SetPhase(Phase::SetSystemHdr);
        }
        return;
    }

    if(s.phase==Phase::SetSystemHdr) {
        DisplayTarget d{};
        if(!ResolveDisplayTarget(transitionGameWindow.load(),d) || !d.hdrSupported) {
            FailTransitionAndRestoreFG("display_target_lost_before_direct_set");
            return;
        }
        lastSystemHdr.store(d.hdrEnabled);
        if(d.hdrEnabled==s.targetHdr) {
            Log("HDR_BUTTON_WINDOWS_ALREADY_TARGET target=%u present=%llu",unsigned(s.targetHdr),read64(kRvaPresentCounter));
            SetPhase(Phase::SetGameHdr);
            return;
        }
        if(!systemSetAttempted.exchange(true)) {
            if(!SetWindowsHdrDirect(d,s.targetHdr)) {
                FailTransitionAndRestoreFG("DisplayConfigSetDeviceInfo_failed");
                return;
            }
        }
        SetPhase(Phase::WaitSystemHdr);
        return;
    }

    if(s.phase==Phase::WaitSystemHdr) {
        DisplayTarget d{};
        if(!ResolveDisplayTarget(transitionGameWindow.load(),d) || !d.hdrSupported) {
            FailTransitionAndRestoreFG("display_target_lost_waiting_windows_hdr");
            return;
        }
        lastSystemHdr.store(d.hdrEnabled);
        if(d.hdrEnabled==s.targetHdr) {
            Log("HDR_BUTTON_WINDOWS_TARGET_CONFIRMED target=%u present=%llu synthetic_hotkey=0",
                unsigned(s.targetHdr),read64(kRvaPresentCounter));
            SetPhase(Phase::SetGameHdr);
        }
        return;
    }

    if(s.phase==Phase::SetGameHdr) {
        if(!gameSetAttempted.exchange(true)) {
            gameSetCompleted.store(false,std::memory_order_release);
            gameSetSucceeded.store(false,std::memory_order_release);
            pendingGameHdrSet.store(s.targetHdr?1:0,std::memory_order_release);
            Log("HDR_BUTTON_GAME_SET_QUEUED target=%u present=%llu native_sequence=%s",
                unsigned(s.targetHdr),read64(kRvaPresentCounter),
                s.targetHdr?"setHDRSettings_then_setHDREnabledDisplay":"setHDRSettings_only");
        }
        SetPhase(Phase::WaitDomain);
        return;
    }

    if(s.phase==Phase::WaitDomain) {
        if(gameSetCompleted.load(std::memory_order_acquire) &&
           !gameSetSucceeded.load(std::memory_order_acquire)) {
            FailTransitionAndRestoreFG("Control_native_HDR_setter_failed");
            return;
        }

        DisplayTarget d{};
        bool gameHdr=false;
        if(!ResolveDisplayTarget(transitionGameWindow.load(),d) || !d.hdrSupported || !ReadGameHdr(gameHdr)) {
            FailTransitionAndRestoreFG("hdr_domain_state_lost");
            return;
        }
        const bool bridgeHdr=read32(kRvaHdrBridgeActive)!=0;
        lastSystemHdr.store(d.hdrEnabled);
        lastGameHdr.store(gameHdr);
        lastBridgeHdr.store(bridgeHdr);
        currentHdr.store(d.hdrEnabled && gameHdr && bridgeHdr);
        currentHdrKnown.store(true);
        displayHdrSupported.store(true);

        const bool domainMatches=
            d.hdrEnabled==s.targetHdr &&
            gameHdr==s.targetHdr &&
            bridgeHdr==s.targetHdr;
        if(domainMatches) {
            const uint64_t aa=read64(kRvaAaCounter);
            const uint64_t present=read64(kRvaPresentCounter);
            AcquireSRWLockExclusive(&stateLock);
            transition.freshAaBase=aa;
            transition.freshPresentBase=present;
            transition.phase=Phase::WaitFreshDomain;
            transition.phaseStartedMs=now;
            ReleaseSRWLockExclusive(&stateLock);
            Log("HDR_BUTTON_DOMAIN_MATCH target=%u system=%u game=%u bridge=%u aa_base=%llu present=%llu action=wait_two_fresh_engine_frames",
                unsigned(s.targetHdr),unsigned(d.hdrEnabled),unsigned(gameHdr),unsigned(bridgeHdr),aa,present);
            UpdateButtonVisual();
        }
        return;
    }

    if(s.phase==Phase::WaitFreshDomain) {
        DisplayTarget d{};
        bool gameHdr=false;
        if(!ResolveDisplayTarget(transitionGameWindow.load(),d) || !d.hdrSupported || !ReadGameHdr(gameHdr)) {
            FailTransitionAndRestoreFG("fresh_domain_state_lost");
            return;
        }
        const bool bridgeHdr=read32(kRvaHdrBridgeActive)!=0;
        const uint64_t aa=read64(kRvaAaCounter);
        const uint64_t present=read64(kRvaPresentCounter);
        lastSystemHdr.store(d.hdrEnabled);
        lastGameHdr.store(gameHdr);
        lastBridgeHdr.store(bridgeHdr);

        const bool domainMatches=
            d.hdrEnabled==s.targetHdr &&
            gameHdr==s.targetHdr &&
            bridgeHdr==s.targetHdr;
        if(!domainMatches) {
            AcquireSRWLockExclusive(&stateLock);
            transition.freshAaBase=aa;
            transition.freshPresentBase=present;
            ReleaseSRWLockExclusive(&stateLock);
            return;
        }

        if(aa>=s.freshAaBase+kFreshDomainFramesRequired && present>s.freshPresentBase) {
            Log("HDR_BUTTON_FRESH_DOMAIN_CONFIRMED target=%u aa_base=%llu aa_now=%llu fresh_frames=%llu present_base=%llu present=%llu action=restore_saved_fg_selection",
                unsigned(s.targetHdr),s.freshAaBase,aa,aa-s.freshAaBase,s.freshPresentBase,present);
            SetPhase(Phase::RestoreFg);
        }
        return;
    }

    if(s.phase==Phase::RestoreFg) {
        protectGameHdr.store(s.targetHdr,std::memory_order_release);
        const unsigned int selection=ReadFGSelectionRaw();
        if(selection!=s.sourceSelection) {
            FailTransitionAndRestoreFG("saved_fg_selection_not_preserved");
            return;
        }
        Log("R33_FG_REARM_WAIT saved_selection=%u target_hdr=%u present=%llu bridge=%u hard_reset_stage=%u free_count=%llu action=core_rearm_after_three_fresh_recomposition_frames",
            s.sourceSelection,unsigned(s.targetHdr),read64(kRvaPresentCounter),read32(kRvaHdrBridgeActive),
            read32(kRvaHdrHardResetStage),read64(kRvaHdrHardResetFreeCount));
        if(s.sourceSelection==0)SetPhase(Phase::Complete);
        else SetPhase(Phase::WaitFgResume);
        return;
    }

    if(s.phase==Phase::WaitFgResume) {
        const unsigned int selection=ReadFGSelectionRaw();
        const unsigned int enabled=read32(kRvaFgEnabledByApi);
        const uint64_t present=read64(kRvaPresentCounter);
        if(selection==s.sourceSelection && enabled!=0) {
            Log("HDR_BUTTON_FG_RESUMED selection=%u api_enabled=%u present=%llu generated_samples=%llu target_hdr=%u",
                selection,enabled,present,read64(kRvaFgGeneratedSamples),unsigned(s.targetHdr));
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
    swprintf_s(name,L"\\hdr-button-R33-%04u%02u%02u-%02u%02u%02u-%lu.log",
        st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,GetCurrentProcessId());
    logFile=CreateFileW((logDir+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);

    GetModuleFileNameW(selfModule,own,32768);
    std::wstring directory(own);directory=directory.substr(0,directory.find_last_of(L"\\/"));
    std::wstring corePath=directory+L"\\dxgi.dll";
    std::string hash=HashFile(corePath);
    Log("HDR_BUTTON_BUILD version=R33 expected_core_sha256=%s actual_core_sha256=%s ui=F10_overlay_child_button architecture=single_owner direct_windows_hdr=1 synthetic_hotkey=0 hard_dlssg_free_before_domain=1 overlap_transition_refree=1 user_selection_preserved=1 full_recomposition=1 fresh_ring_frames=3 external_shield=0 timeout_ms=%llu fail_open_restore_fg=1",
        kExpectedCoreSha256,hash.c_str(),kTransitionTimeoutMs);
    if(hash!=kExpectedCoreSha256){Log("HDR_BUTTON_INSTALL_FAIL reason=core_hash");return 0;}
    core=reinterpret_cast<unsigned char*>(GetModuleHandleW(corePath.c_str()));
    if(!core){Log("HDR_BUTTON_INSTALL_FAIL reason=core_module");return 0;}
    if(!InstallCoreHook()){Log("HDR_BUTTON_INSTALL_FAIL reason=core_hook");return 0;}
    ResolveGameHdrApi();
    ready.store(true);
    Log("HDR_BUTTON_READY controller=single_owner_windows_and_control_hdr game_hdr_control_ready=%u sequence=save_fg_selection_then_hard_dlssg_free_then_direct_windows_hdr_then_control_hdr_then_three_fresh_recomposition_frames_then_core_rearm overlap_policy=opposite_unsettled_transition_new_serial_refree hotkey_injection=0 selection_mutation=0 sidecar_hold=0 shield=0",
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
extern "C" __declspec(dllexport) unsigned WINAPI ControlFGHDRButton_Version(){return 0x00330001;}

BOOL WINAPI DllMain(HINSTANCE mod,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        selfModule=mod;DisableThreadLibraryCalls(mod);
        HANDLE t=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);
        if(t)CloseHandle(t);
    }
    return TRUE;
}
