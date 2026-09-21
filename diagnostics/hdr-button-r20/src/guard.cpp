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
static bool (*beginHdrHardReset)(uint64_t,const char*){};
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
using GameHdrGet = bool (*)();
using GameHdrSetVoid = void (*)(bool);
using GameHdrSetBool = bool (*)(bool);
static HMODULE gameD3d{};
static GameHdrGet gameHdrGet{};
static GameHdrSetVoid gameHdrSetVoid{};
static GameHdrSetBool gameHdrSetBool{};
static std::atomic<bool> gameHdrControlReady{false};
static std::atomic<int> pendingGameHdrSet{-1};
static std::atomic<bool> systemToggleSent{false};
static std::atomic<bool> gameSetAttempted{false};
static std::atomic<bool> lastGameHdr{false};
static std::atomic<bool> lastSystemHdr{false};
static std::atomic<bool> lastBridgeHdr{false};

static bool ReadGameHdr(bool& value) noexcept {
    if(!gameHdrGet)return false;
    __try { value=gameHdrGet(); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { Log("HDR_BUTTON_GAME_GET_EXCEPTION code=0x%08lX",GetExceptionCode()); return false; }
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

struct HdrScanRange {
    unsigned char* base{};
    size_t imageSize{};
    unsigned char* text{};
    size_t textSize{};
};
static bool GetHdrScanRange(HMODULE module, HdrScanRange& out) noexcept {
    out={};
    if(!module)return false;
    auto* base=reinterpret_cast<unsigned char*>(module);
    __try {
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;
        out.base=base;out.imageSize=nt->OptionalHeader.SizeOfImage;
        auto* sec=IMAGE_FIRST_SECTION(nt);
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
            char name[9]{};memcpy(name,sec[i].Name,8);
            if(strcmp(name,".text")==0) {
                out.text=base+sec[i].VirtualAddress;
                out.textSize=sec[i].Misc.VirtualSize;
                return out.text && out.textSize;
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("HDR_SETTER_SCAN_PE_FAIL code=0x%08lX",GetExceptionCode());
    }
    return false;
}
static void LogScanBytes(const char* label,const unsigned char* p,size_t count,const HdrScanRange& range) noexcept {
    if(!p || p<range.base || p>=range.base+range.imageSize)return;
    if(p+count>range.base+range.imageSize)count=static_cast<size_t>(range.base+range.imageSize-p);
    char hex[769]{};size_t used=0;
    for(size_t i=0;i<count && used+3<sizeof(hex);++i) {
        int n=snprintf(hex+used,sizeof(hex)-used,"%02X",p[i]);
        if(n<=0)break;used+=static_cast<size_t>(n);
        if(i+1<count && used+2<sizeof(hex))hex[used++]=' ';
    }
    hex[used]=0;
    Log("HDR_SETTER_SCAN_BYTES label=%s rva=0x%zX size=%zu bytes=%s",
        label,static_cast<size_t>(p-range.base),count,hex);
}
struct RipPattern { const unsigned char* prefix; size_t prefixLen; size_t dispOffset; size_t instructionLen; const char* kind; };
static bool MatchPrefix(const unsigned char* p,const unsigned char* end,const unsigned char* prefix,size_t n) noexcept {
    return p+n<=end && memcmp(p,prefix,n)==0;
}
static void AddUniqueTarget(std::vector<uintptr_t>& targets,uintptr_t target) {
    for(auto v:targets)if(v==target)return;
    targets.push_back(target);
}
static void ScanFunctionDataRefs(const unsigned char* fn,size_t len,const char* source,
                                 const HdrScanRange& range,std::vector<uintptr_t>& targets) noexcept {
    static const unsigned char p1[]={0x48,0x8B,0x05};
    static const unsigned char p2[]={0x48,0x8D,0x05};
    static const unsigned char p3[]={0x48,0x89,0x05};
    static const unsigned char p4[]={0x8A,0x05};
    static const unsigned char p5[]={0x88,0x05};
    static const unsigned char p6[]={0x0F,0xB6,0x05};
    static const unsigned char p7[]={0x80,0x3D};
    static const unsigned char p8[]={0x83,0x3D};
    static const unsigned char p9[]={0xC6,0x05};
    static const unsigned char p10[]={0xC7,0x05};
    static const unsigned char p11[]={0x39,0x05};
    static const unsigned char p12[]={0x38,0x05};
    const RipPattern patterns[]={
        {p1,3,3,7,"read64"},{p2,3,3,7,"lea"},{p3,3,3,7,"write64"},
        {p4,2,2,6,"read8"},{p5,2,2,6,"write8"},{p6,3,3,7,"read8zx"},
        {p7,2,2,7,"cmp8"},{p8,2,2,7,"cmp32imm8"},{p9,2,2,7,"write8imm"},
        {p10,2,2,10,"write32imm"},{p11,2,2,6,"cmp32"},{p12,2,2,6,"cmp8reg"}
    };
    const unsigned char* end=fn+len;
    for(size_t off=0;off<len;++off) {
        const unsigned char* p=fn+off;
        for(const auto& pat:patterns) {
            if(!MatchPrefix(p,end,pat.prefix,pat.prefixLen) || p+pat.instructionLen>end)continue;
            int32_t disp{};memcpy(&disp,p+pat.dispOffset,sizeof(disp));
            uintptr_t target=reinterpret_cast<uintptr_t>(p+pat.instructionLen)+static_cast<int64_t>(disp);
            uintptr_t lo=reinterpret_cast<uintptr_t>(range.base),hi=lo+range.imageSize;
            if(target<lo || target>=hi)continue;
            AddUniqueTarget(targets,target);
            Log("HDR_SETTER_SCAN_DATAREF source=%s site_rva=0x%zX kind=%s target_rva=0x%zX",
                source,static_cast<size_t>(p-range.base),pat.kind,static_cast<size_t>(target-lo));
        }
        if(p+5<=end && *p==0xE8) {
            int32_t rel{};memcpy(&rel,p+1,sizeof(rel));
            const unsigned char* dst=p+5+rel;
            if(dst>=range.text && dst<range.text+range.textSize) {
                Log("HDR_SETTER_SCAN_CALL source=%s site_rva=0x%zX target_rva=0x%zX",
                    source,static_cast<size_t>(p-range.base),static_cast<size_t>(dst-range.base));
            }
        }
    }
}
static const char* XrefKindAt(const unsigned char* p,const unsigned char* end,size_t& len,size_t& dispOff) noexcept {
    if(p+6<=end && p[0]==0x88 && p[1]==0x05){len=6;dispOff=2;return "WRITE8";}
    if(p+7<=end && p[0]==0xC6 && p[1]==0x05){len=7;dispOff=2;return "WRITE8_IMM";}
    if(p+7<=end && p[0]==0x48 && p[1]==0x89 && p[2]==0x05){len=7;dispOff=3;return "WRITE64";}
    if(p+6<=end && p[0]==0x89 && p[1]==0x05){len=6;dispOff=2;return "WRITE32";}
    if(p+10<=end && p[0]==0xC7 && p[1]==0x05){len=10;dispOff=2;return "WRITE32_IMM";}
    if(p+6<=end && p[0]==0x8A && p[1]==0x05){len=6;dispOff=2;return "READ8";}
    if(p+7<=end && p[0]==0x0F && p[1]==0xB6 && p[2]==0x05){len=7;dispOff=3;return "READ8ZX";}
    if(p+7<=end && p[0]==0x48 && p[1]==0x8B && p[2]==0x05){len=7;dispOff=3;return "READ64";}
    if(p+7<=end && p[0]==0x80 && p[1]==0x3D){len=7;dispOff=2;return "CMP8";}
    if(p+7<=end && p[0]==0x83 && p[1]==0x3D){len=7;dispOff=2;return "CMP32";}
    if(p+6<=end && p[0]==0x38 && p[1]==0x05){len=6;dispOff=2;return "CMP8REG";}
    if(p+6<=end && p[0]==0x39 && p[1]==0x05){len=6;dispOff=2;return "CMP32REG";}
    return nullptr;
}
static void ScanTextXrefsToTargets(const HdrScanRange& range,const std::vector<uintptr_t>& targets) noexcept {
    const unsigned char* end=range.text+range.textSize;
    unsigned logged=0,writes=0;
    for(const unsigned char* p=range.text;p<end && logged<256;++p) {
        size_t len=0,dispOff=0;const char* kind=XrefKindAt(p,end,len,dispOff);
        if(!kind)continue;
        int32_t disp{};memcpy(&disp,p+dispOff,sizeof(disp));
        uintptr_t target=reinterpret_cast<uintptr_t>(p+len)+static_cast<int64_t>(disp);
        bool wanted=false;for(auto v:targets)if(v==target){wanted=true;break;}if(!wanted)continue;
        DWORD64 imageBase=reinterpret_cast<DWORD64>(range.base);
        PRUNTIME_FUNCTION rf=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(p),&imageBase,nullptr);
        size_t begin=0,endRva=0;
        if(rf){begin=rf->BeginAddress;endRva=rf->EndAddress;}
        const bool write=strncmp(kind,"WRITE",5)==0;if(write)++writes;
        Log("HDR_SETTER_SCAN_XREF kind=%s site_rva=0x%zX target_rva=0x%zX function_begin=0x%zX function_end=0x%zX",
            kind,static_cast<size_t>(p-range.base),static_cast<size_t>(target-reinterpret_cast<uintptr_t>(range.base)),begin,endRva);
        const unsigned char* start=p>=range.text+16?p-16:range.text;
        LogScanBytes(write?"writer_context":"xref_context",start,48,range);
        ++logged;
    }
    Log("HDR_SETTER_SCAN_XREF_SUMMARY targets=%zu xrefs=%u writes=%u",targets.size(),logged,writes);
}
static void ScanHdrGetterAndWriters(HMODULE module,GameHdrGet getter) noexcept {
    HdrScanRange range{};
    if(!module || !getter || !GetHdrScanRange(module,range)) {
        Log("HDR_SETTER_SCAN_FAIL reason=module_or_getter_or_text");
        return;
    }
    auto* get=reinterpret_cast<unsigned char*>(getter);
    Log("HDR_SETTER_SCAN_BEGIN module=%p image_size=0x%zX text_rva=0x%zX text_size=0x%zX getter_rva=0x%zX",
        module,range.imageSize,static_cast<size_t>(range.text-range.base),range.textSize,static_cast<size_t>(get-range.base));
    LogScanBytes("isHDREnabled",get,96,range);
    std::vector<uintptr_t> targets;
    ScanFunctionDataRefs(get,96,"getter",range,targets);
    // Follow direct calls made by the getter one level so wrappers still expose their backing state.
    for(size_t off=0;off<96;++off) {
        auto* p=get+off;
        if(p+5>range.base+range.imageSize || *p!=0xE8)continue;
        int32_t rel{};memcpy(&rel,p+1,sizeof(rel));
        auto* dst=p+5+rel;
        if(dst<range.text || dst>=range.text+range.textSize)continue;
        LogScanBytes("getter_call_target",dst,96,range);
        ScanFunctionDataRefs(dst,96,"getter_call_target",range,targets);
    }
    ScanTextXrefsToTargets(range,targets);
    Log("HDR_SETTER_SCAN_END candidate_state_targets=%zu",targets.size());
}

static bool ResolveGameHdrApi() noexcept {
    gameD3d=GetModuleHandleW(L"d3d_rmdwin10_f.dll");
    if(!gameD3d)return false;
    gameHdrGet=reinterpret_cast<GameHdrGet>(GetProcAddress(gameD3d,"?isHDREnabled@DeviceUtil@d3d@@SA_NXZ"));
    struct Candidate { const char* name; bool boolReturn; };
    const Candidate candidates[]={
        {"?setHDREnabled@DeviceUtil@d3d@@SAX_N@Z",false},
        {"?setHDREnabled@DeviceUtil@d3d@@SA_N_N@Z",true},
        {"?setHdrEnabled@DeviceUtil@d3d@@SAX_N@Z",false},
        {"?setHdrEnabled@DeviceUtil@d3d@@SA_N_N@Z",true}
    };
    for(const auto& candidate:candidates) {
        auto p=GetProcAddress(gameD3d,candidate.name);
        if(!p)continue;
        if(candidate.boolReturn) gameHdrSetBool=reinterpret_cast<GameHdrSetBool>(p);
        else gameHdrSetVoid=reinterpret_cast<GameHdrSetVoid>(p);
        Log("HDR_BUTTON_GAME_SETTER_RESOLVED name=%s address=%p bool_return=%u",candidate.name,p,unsigned(candidate.boolReturn));
        break;
    }
    LogHdrExports(gameD3d);
    ScanHdrGetterAndWriters(gameD3d,gameHdrGet);
    bool current=false;
    const bool getterWorks=gameHdrGet && ReadGameHdr(current);
    const bool setterWorks=gameHdrSetVoid || gameHdrSetBool;
    gameHdrControlReady.store(false);
    lastGameHdr.store(current);
    Log("HDR_BUTTON_GAME_API getter=%p getter_works=%u setter_void=%p setter_bool=%p control_ready=%u initial_game_hdr=%u",
        gameHdrGet,unsigned(getterWorks),gameHdrSetVoid,gameHdrSetBool,0u,unsigned(current));
    (void)setterWorks;
    return false;
}
static bool SetGameHdr(bool enable) noexcept {
    if(!gameHdrControlReady.load())return false;
    bool callResult=true;
    __try {
        if(gameHdrSetVoid) gameHdrSetVoid(enable);
        else if(gameHdrSetBool) callResult=gameHdrSetBool(enable);
        else return false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("HDR_BUTTON_GAME_SET_EXCEPTION target=%u code=0x%08lX",unsigned(enable),GetExceptionCode());
        return false;
    }
    bool after=false;const bool readOk=ReadGameHdr(after);
    Log("HDR_BUTTON_GAME_SET target=%u call_result=%u read_ok=%u immediate_game_hdr=%u",
        unsigned(enable),unsigned(callResult),unsigned(readOk),unsigned(after));
    return callResult;
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

static bool HdrShortcutKeysReleased() noexcept {
    const int keys[] = {VK_LWIN,VK_RWIN,VK_LMENU,VK_RMENU,VK_MENU,'B'};
    for(int key : keys) if(GetAsyncKeyState(key)&0x8000) return false;
    return true;
}
static bool SendWindowsHdrShortcut() noexcept {
    if(!HdrShortcutKeysReleased()) return false;
    INPUT in[6]{};
    const WORD keys[6] = {VK_LWIN,VK_LMENU,'B','B',VK_LMENU,VK_LWIN};
    for(int i=0;i<6;++i) {
        in[i].type=INPUT_KEYBOARD;
        in[i].ki.wVk=keys[i];
        if(i>=3) in[i].ki.dwFlags|=KEYEVENTF_KEYUP;
        if(keys[i]==VK_LWIN) in[i].ki.dwFlags|=KEYEVENTF_EXTENDEDKEY;
    }
    SetLastError(ERROR_SUCCESS);
    const UINT sent=SendInput(6,in,sizeof(INPUT));
    const DWORD err=GetLastError();
    Log("HDR_BUTTON_WINDOWS_SHORTCUT sent=%u expected=6 error=%lu",sent,err);
    if(sent==6) return true;
    INPUT release[3]{};
    release[0]=in[3];release[1]=in[4];release[2]=in[5];
    SendInput(3,release,sizeof(INPUT));
    return false;
}

static void UpdateButtonVisual() noexcept {
    HWND b=buttonWindow.load();
    if(b)InvalidateRect(b,nullptr,TRUE);
}

static void SetPhase(Phase phase,const char* why=nullptr) noexcept {
    AcquireSRWLockExclusive(&stateLock);
    transition.phase=phase;
    if(why)transition.error=why;
    ReleaseSRWLockExclusive(&stateLock);
    Log("HDR_BUTTON_STATE phase=%s reason=%s",PhaseName(phase),why?why:"none");
    UpdateButtonVisual();
}

static State SnapshotState() noexcept {
    AcquireSRWLockShared(&stateLock);
    State s=transition;
    ReleaseSRWLockShared(&stateLock);
    return s;
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

    AcquireSRWLockExclusive(&stateLock);
    if(transition.active()) {
        ReleaseSRWLockExclusive(&stateLock);
        Log("HDR_BUTTON_REJECT reason=transition_busy");
        return false;
    }
    transition=State{};
    const bool effectiveHdr=d.hdrEnabled && gameHdr;
    transition.sourceHdr=effectiveHdr;
    transition.targetHdr=!effectiveHdr;
    transition.sourceSelection=read32(0xAA0A0);
    transition.sourceFgWasEnabled=transition.sourceSelection!=0 && read32(0xAB9D0)!=0;
    transition.startedMs=GetTickCount64();
    transition.basePresent=read64(0xAB7B8);
    transition.baseFreeCount=read64(0xABC70);
    transition.baseGeneratedCount=read64(0xABB28);
    transition.phase=transition.sourceSelection==0?Phase::SetSystemHdr:Phase::RequestOff;
    State s=transition;
    ReleaseSRWLockExclusive(&stateLock);

    currentHdr.store(effectiveHdr);
    currentHdrKnown.store(true);
    displayHdrSupported.store(true);
    lastSystemHdr.store(d.hdrEnabled);lastGameHdr.store(gameHdr);lastBridgeHdr.store(read32(0xABCF0)!=0);
    systemToggleSent.store(false);gameSetAttempted.store(false);pendingGameHdrSet.store(-1);
    Log("HDR_BUTTON_REQUEST source_effective_hdr=%u target_hdr=%u system_hdr=%u game_hdr=%u bridge_hdr=%u selection=%u fg_api_enabled=%u present=%llu free_count=%llu",
        unsigned(s.sourceHdr),unsigned(s.targetHdr),unsigned(d.hdrEnabled),unsigned(gameHdr),unsigned(read32(0xABCF0)!=0),
        s.sourceSelection,read32(0xAB9D0),s.basePresent,s.baseFreeCount);
    if(s.phase==Phase::RequestOff)pendingBegin.store(true);
    UpdateButtonVisual();
    return true;
}

static void OnFrame() noexcept {
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
            Log("HDR_BUTTON_FAIL reason=game_hdr_setter_failed target=%d",gameSet);
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
        Log("HDR_BUTTON_FAIL reason=timeout phase=%s enabled=%u present=%llu frees=%llu bridge=%u",
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
        if(!ResolveDisplayTarget(FindGameWindow(),d) || !d.hdrSupported || !ReadGameHdr(gameHdr)) {
            SetPhase(Phase::Failed,"hdr_state_lost");return;
        }
        const bool bridgeHdr=read32(0xABCF0)!=0;
        lastSystemHdr.store(d.hdrEnabled);lastGameHdr.store(gameHdr);lastBridgeHdr.store(bridgeHdr);
        currentHdr.store(d.hdrEnabled && gameHdr);currentHdrKnown.store(true);displayHdrSupported.store(true);

        if(s.targetHdr) {
            // Enabling: Windows HDR first, then Control HDR, then require bridge HDR.
            if(!d.hdrEnabled) {
                if(!systemToggleSent.load()) {
                    if(!HdrShortcutKeysReleased()) return;
                    if(!SendWindowsHdrShortcut()) { SetPhase(Phase::Failed,"WinAltB_SendInput_failed"); return; }
                    systemToggleSent.store(true);
                    Log("HDR_BUTTON_SYSTEM_ENABLE_SENT game_hdr=%u bridge_hdr=%u",unsigned(gameHdr),unsigned(bridgeHdr));
                }
                SetPhase(Phase::WaitGameHdr);return;
            }
            if(!gameHdr) {
                if(!gameSetAttempted.exchange(true)) {
                    pendingGameHdrSet.store(1);
                    Log("HDR_BUTTON_GAME_ENABLE_QUEUED system_hdr=1 bridge_hdr=%u",unsigned(bridgeHdr));
                }
                SetPhase(Phase::WaitGameHdr);return;
            }
            if(!bridgeHdr) { SetPhase(Phase::WaitGameHdr);return; }
            Log("HDR_BUTTON_COMBINED_HDR_CONFIRMED target=1 system=1 game=1 bridge=1 present=%llu",read64(0xAB7B8));
            SetPhase(read32(0xAA0A0)==0?Phase::Complete:Phase::WaitFgResume);return;
        } else {
            // Disabling: Control HDR first while Windows remains HDR, then Windows HDR off.
            if(gameHdr || bridgeHdr) {
                if(!gameSetAttempted.exchange(true)) {
                    pendingGameHdrSet.store(0);
                    Log("HDR_BUTTON_GAME_DISABLE_QUEUED system_hdr=%u game_hdr=%u bridge_hdr=%u",
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
            Log("HDR_BUTTON_COMBINED_HDR_CONFIRMED target=0 system=0 game=0 bridge=0 present=%llu",read64(0xAB7B8));
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
    swprintf_s(name,L"\\hdr-button-R24-%04u%02u%02u-%02u%02u%02u-%lu.log",
        st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,GetCurrentProcessId());
    logFile=CreateFileW((logDir+name).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);

    GetModuleFileNameW(selfModule,own,32768);
    std::wstring directory(own);directory=directory.substr(0,directory.find_last_of(L"\\/"));
    std::wstring corePath=directory+L"\\dxgi.dll";
    std::string hash=HashFile(corePath);
    Log("HDR_BUTTON_BUILD version=R24 expected_core_sha256=%s actual_core_sha256=%s ui=F10_overlay_child_button",
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
                currentHdr.store(gameKnown && d.hdrEnabled && gameHdr);
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
extern "C" __declspec(dllexport) unsigned WINAPI ControlFGHDRButton_Version(){return 0x00240001;}

BOOL WINAPI DllMain(HINSTANCE mod,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        selfModule=mod;DisableThreadLibraryCalls(mod);
        HANDLE t=CreateThread(nullptr,0,Worker,nullptr,0,nullptr);
        if(t)CloseHandle(t);
    }
    return TRUE;
}
