#pragma once
#include "../lighting_capture_plan.h"
#include "lighting_screenshot.h"
namespace native_lighting::capture_suite {
// All suite state and file I/O belong to the diagnostic UI thread.
inline bool active=false,entered=false,gpuAccepted=false;
inline unsigned step=0,imageIndex=0,failures=0;
inline Mode restore=Mode::Native;
inline uint64_t began=0,firstPresent=0,nextImage=0,selectedState=0;
inline wchar_t folder[MAX_PATH]{};
inline HANDLE manifest=INVALID_HANDLE_VALUE;
inline LARGE_INTEGER logOffset{};
inline void Note(const char* format,...) noexcept {
    char line[1024]{};va_list args;va_start(args,format);
    int n=vsnprintf_s(line,sizeof(line)-3,_TRUNCATE,format,args);va_end(args);
    if(n<0)n=int(strlen(line));line[n++]='\r';line[n++]='\n';DWORD written=0;
    if(manifest!=INVALID_HANDLE_VALUE)WriteFile(manifest,line,n,&written,nullptr);
}
inline bool CopyLog(const wchar_t* destination) noexcept {
    HANDLE in=CreateFileW(report.Path(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    if(in==INVALID_HANDLE_VALUE)return false;
    bool ok=SetFilePointerEx(in,logOffset,nullptr,FILE_BEGIN)!=FALSE;
    HANDLE out=CreateFileW(destination,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);
    ok=ok && out!=INVALID_HANDLE_VALUE;
    char data[16384];DWORD read=0,written=0;
    while(ok){if(!ReadFile(in,data,sizeof(data),&read,nullptr)){ok=false;break;}if(!read)break;
        ok=WriteFile(out,data,read,&written,nullptr) && written==read;}
    if(out!=INVALID_HANDLE_VALUE)CloseHandle(out);CloseHandle(in);return ok;
}
inline void Finish(HWND game,const char* reason) noexcept {
    rt_spot_capture_runtime::CancelPending();
    SelectMode(restore);ReportRRPaths("f9_restore");rt_shadow_capture::Summary();
    Trace("capture_suite_end reason=%s failures=%u restored=%s",reason,failures,ModeName(restore));
    DrainDiagnosticLog(true);
    wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\lighting.log",folder);
    if(!CopyLog(path))++failures;
    Note("end=%s failures=%u restored=%s",reason,failures,ModeName(restore));
    if(manifest!=INVALID_HANDLE_VALUE){FlushFileBuffers(manifest);CloseHandle(manifest);manifest=INVALID_HANDLE_VALUE;}
    active=false;
    wchar_t message[768]{};swprintf_s(message,L"Shadow capture %s. %u skipped/failed steps or files.\n\n%s\n\nYour previous mode has been restored. Zip and send this folder.\nPNG images record the displayed game; lighting.log contains GPU capture status.",
        strcmp(reason,"complete")==0?L"finished":L"stopped",failures,folder);
    MessageBoxW(game,message,L"Control-FG F9 capture",MB_OK);
}
inline bool Start() noexcept {
    if(active)return false;
    using CapturePhase=rt_spot_capture_runtime::Phase;
    const auto capturePhase=rt_spot_capture_runtime::phase.load();
    if(capturePhase!=CapturePhase::Idle && capturePhase!=CapturePhase::Failed &&
       !(capturePhase==CapturePhase::Complete && rt_spot_capture_runtime::exportStage==3)) {
        Trace("capture_suite_start_rejected reason=previous_gpu_capture_pending");return false;
    }
    if(!DrainDiagnosticLog(true))return false;
    wchar_t root[MAX_PATH]{};wcscpy_s(root,report.Path());auto* slash=wcsrchr(root,L'\\');if(!slash)return false;*slash=0;
    SYSTEMTIME time{};GetLocalTime(&time);static unsigned serial=0;
    if(swprintf_s(folder,L"%s\\Shadow-R135-%04u%02u%02u-%02u%02u%02u-%lu-%u",root,time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,GetCurrentProcessId(),serial++)<0)return false;
    if(!CreateDirectoryW(folder,nullptr))return false;
    wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\manifest.txt",folder);
    manifest=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);
    if(manifest==INVALID_HANDLE_VALUE)return false;
    HANDLE log=CreateFileW(report.Path(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    if(log==INVALID_HANDLE_VALUE || !GetFileSizeEx(log,&logOffset)){
        if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);CloseHandle(manifest);manifest=INVALID_HANDLE_VALUE;return false;}
    CloseHandle(log);
    restore=LiveSelection::GetMode(selection.Read());active=true;entered=false;step=imageIndex=failures=0;
    Note("build=R135 restore=%s steps=%u PNG=display_referred_not_raw_HDR\nKeep camera still. F9 cancels. Focus loss cancels. No GPU waits.\nUnavailable shader variants are explicitly skipped. Hit coordinates/visibility use unclipped reference rays.\nGPU capture is collected in BOTH, with native per-light context and hit identities.",ModeName(restore),capture_plan::count);
    Trace("capture_suite_start build=R135 restore=%s steps=%u",ModeName(restore),capture_plan::count);
    ReportRRPaths("f9_start");return true;
}
inline void Tick(HWND game,uint64_t now) noexcept {
    if(!active)return;
    if(GetForegroundWindow()!=game || IsIconic(game)){Finish(game,"focus_lost");return;}
    rt_spot_capture_runtime::Poll();
    if(step>=capture_plan::count){Finish(game,"complete");return;}
    const auto& current=capture_plan::steps[step];
    if(!entered){
        if(!SelectMode(current.mode)){Note("step=%u mode=%s skipped=unavailable",step,ModeName(current.mode));++failures;++step;return;}
        selectedState=selection.Read();began=now;firstPresent=presentCount.load();imageIndex=0;nextImage=0;entered=true;gpuAccepted=false;
        Note("step=%u mode=%s start_ms=%llu present=%llu gpu_requested=%u",step,ModeName(current.mode),now,firstPresent,unsigned(gpuAccepted));
        Trace("capture_suite_step index=%u mode=%s",step,ModeName(current.mode));return;
    }
    if(LiveSelection::Sequence(selection.Read())!=LiveSelection::Sequence(selectedState)){Finish(game,"selection_changed");return;}
    const auto elapsed=now-began;
    const auto observed=LiveSelection::Observed(selection.Read());
    // Native has no replacement hook; count presents instead of claiming a GPU binding.
    const bool bound=current.mode==Mode::Native || (observed&kDirectBit)!=0;
    const bool settled=capture_plan::Settled(elapsed,presentCount.load()-firstPresent,bound);
    if(current.gpu && settled && !gpuAccepted)gpuAccepted=rt_spot_capture_runtime::RequestFresh();
    if(settled && now>=nextImage && imageIndex<capture_plan::imagesPerStep){
        wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\%02u-%s-%u.png",folder,step,current.name,imageIndex);
        const HRESULT hr=screenshots::Save(game,path);
        Note("image step=%u sample=%u ms=%llu present=%llu hr=0x%08lX rr=%u effects=0x%X",step,imageIndex,now,presentCount.load(),static_cast<unsigned long>(hr),unsigned(control_rr_diagnostic::lastSelected.load()),control_rr_diagnostic::lastEffects.load());
        if(FAILED(hr))++failures;++imageIndex;nextImage=GetTickCount64()+capture_plan::burstIntervalMs;
    }
    const bool gpuDone=!current.gpu || (gpuAccepted && rt_spot_capture_runtime::Exported());
    if((imageIndex==capture_plan::imagesPerStep && gpuDone) || elapsed>=capture_plan::timeoutMs){
        if(imageIndex<capture_plan::imagesPerStep || !gpuDone){++failures;Note("step=%u timeout=1 images=%u gpu_exported=%u",step,imageIndex,unsigned(gpuDone));}
        Note("step=%u end_ms=%llu gpu_exported=%u",step,now,unsigned(current.gpu && gpuDone));
        ++step;entered=false;
    }
}
}
