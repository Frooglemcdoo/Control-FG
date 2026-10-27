#pragma once
// UI-thread coordinator. Rendering policy is never changed by this capture.
namespace native_lighting::performance_capture {
enum class Phase { Idle, Delay, Measure, Drain, Context, Package };
inline Phase phase=Phase::Idle;
inline uint64_t began=0,session=0,state=0,startPresent=0,phaseTime=0;
inline unsigned rows=0,errors=0,capturedSpecularChoice=4;
inline uint64_t measureStartMs=0,measureStartPresent=0;
inline bool contextRequested=false,capturedParallel=false;
inline uint64_t measureParallelStart=0;
inline wchar_t folder[MAX_PATH]{};
inline FILE* csv=nullptr;
inline FILE* manifest=nullptr;
inline HANDLE packager=nullptr;
inline LARGE_INTEGER logOffset{},mainOffset{};
inline double sums[performance::kStageCount]{};inline unsigned counts[performance::kStageCount]{};
inline uint64_t nextStatus=0;
inline void Settings(const char* label) noexcept {
    if(!manifest)return;
    fprintf(manifest,"spec_serial settings=%s direct_matched=%llu direct_created=%llu library_matched=%llu library_created=%llu failures=%llu creation_only=1 gpu_execution_proven=0\n",
        label,spec_serial::directMatched.load(),spec_serial::directCreated.load(),spec_serial::libraryMatched.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
    fprintf(manifest,"spec_retry settings=%s matched=%llu created=%llu failures=%llu creation_only=1 gpu_execution_proven=0\n",label,spec_retry::matched.load(),spec_retry::created.load(),spec_retry::failures.load());
    fprintf(manifest,"spec_parallel settings=%s enabled=%u ready=%u recorded_dispatches=%llu\n",label,unsigned(specularParallelEnabled.load()),unsigned(specularParallelReady.load()),specularParallelDispatches.load());
    fprintf(manifest,"settings=%s ms=%llu present=%llu rr_requested=%u rr_selected=%u fg_selection=%u fg_enabled=%u dynamic_target=%u gi_request=%u specular_request=%u glass=%u streaming_ms=%u hdr_known=%u hdr=%u vram_sample_valid=%u vram_usage=%llu vram_budget=%llu\n",
        label,GetTickCount64(),presentCount.load(),unsigned(control_rr::RRUserRequested()),unsigned(control_rr_diagnostic::lastSelected.load()),
        GetFGUserMultiplier(),unsigned(slFgEnabledByApi.load()),GetFGDynamicManualTargetFps(),
        GiSampleRequest(LiveSelection::GetMode(selection.Read()),control_rr::GISampleRequested()),SpecularSampleRequest(LiveSelection::GetMode(selection.Read())),
        unsigned(PTReflectionUserRequested()),GetFGTextureStreamingBudgetMs(),vramProbeHdrKnown.load(),vramProbeHdrEnabled.load(),vramProbeSampleValid.load(),vramProbeLocalUsageBytes.load(),vramProbeLocalBudgetBytes.load());
}
inline bool Active() noexcept {return phase!=Phase::Idle;}
inline const wchar_t* Status() noexcept {
    switch(phase){case Phase::Delay:return L"Performance: return to gameplay; five-second settling delay.";
    case Phase::Measure:return L"Performance: recording 20 seconds; move through the affected area.";
    case Phase::Drain:return L"Performance: collecting completed GPU timings.";
    case Phase::Context:return L"Performance: collecting one light-context snapshot.";
    case Phase::Package:return L"Performance: packaging results.";
    default:return L"PERF CAPTURE: five-second delay, 20-second recording, automatic ZIP.";}
}
inline bool CopyTail(const wchar_t* source,const wchar_t* target,LARGE_INTEGER offset) noexcept {
    HANDLE in=CreateFileW(source,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    if(in==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(in,&size) && size.QuadPart>=offset.QuadPart && size.QuadPart-offset.QuadPart<=64*1024*1024;
    HANDLE out=INVALID_HANDLE_VALUE;
    if(ok)out=CreateFileW(target,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);
    ok=ok && out!=INVALID_HANDLE_VALUE && SetFilePointerEx(in,offset,nullptr,FILE_BEGIN);
    char bytes[16384];DWORD n=0,written=0;LONGLONG left=size.QuadPart-offset.QuadPart;
    while(ok && left>0){const DWORD amount=DWORD((std::min)(left,LONGLONG(sizeof(bytes))));
        ok=ReadFile(in,bytes,amount,&n,nullptr) && n && WriteFile(out,bytes,n,&written,nullptr) && n==written;left-=n;}
    if(out!=INVALID_HANDLE_VALUE)CloseHandle(out);CloseHandle(in);return ok;
}
inline bool Offset(const wchar_t* path,LARGE_INTEGER& offset) noexcept {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;const bool ok=GetFileSizeEx(file,&offset)!=FALSE;CloseHandle(file);return ok;
}
inline wchar_t mainPath[32768]{};
inline void Emit(const performance::Sample& s,double ms) noexcept {
    if(!csv)return;
    const auto stage=static_cast<unsigned>(s.stage);
    if(fprintf(csv,"%llu,%llu,%s,%.9f,%u,%u,%u,%llu,%s,%u,%u,%llu,%s,%u,%llu,%llu,%llu,%llu,%llu,%llu\n",s.capture,s.present,performance::Name(s.stage),ms,s.x,s.y,s.z,s.tableBytes,
        ModeName(LiveSelection::GetMode(s.selection)),unsigned(s.rr),s.effects,s.frequency,s.sourceShader,s.recordIndex,s.raygenStart,s.raygenBytes,s.hitStart,s.hitStride,s.missStart,s.missStride)<0)++errors;
    sums[stage]+=ms;++counts[stage];++rows;
}
inline void Complete(HWND game,bool packed) noexcept {
    wchar_t message[1024]{};
    swprintf_s(message,L"Performance capture %s. GPU samples: %u. Reported problems: %u.\n\n%s%s\n\nRendering settings were not changed. Send the ZIP; if packaging failed, send the files in the folder.",
        packed?L"saved":L"incomplete",rows,errors,folder,packed?L".zip":L"");
    phase=Phase::Idle;
    MessageBoxW(game,message,L"Control-FG performance capture",MB_OK|(packed&&!errors?MB_ICONINFORMATION:MB_ICONWARNING));
}
inline void Save(HWND game,const char* reason) noexcept {
    performance::enabled=false;
    performance::watch=false;
    rt_spot_capture_runtime::CancelPending();
    performance::Poll(session,Emit);
    ReportRRPaths("performance_capture_end");
    Trace("performance_capture_end session=%llu reason=%s rows=%u skipped=%llu timing_fault=%u",session,reason,rows,performance::skipped.load(),unsigned(performance::fault.load()));
    if(performance::fault.load() || !rows)++errors;
    if(!DrainDiagnosticLog(true))++errors;
    wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\lighting.log",folder);
    if(!CopyTail(report.Path(),path,logOffset))++errors;
    swprintf_s(path,L"%s\\runtime.log",folder);
    if(!*mainPath || !CopyTail(mainPath,path,mainOffset))++errors;
    if(csv){if(fclose(csv)!=0)++errors;csv=nullptr;}
    if(manifest){
        Settings("end");
        fprintf(manifest,"end=%s\ngpu_samples=%u\nskipped=%llu\ntiming_fault=%u\nerrors=%u\n",reason,rows,performance::skipped.load(),unsigned(performance::fault.load()),errors);
        for(unsigned i=1;i<performance::kStageCount;++i)fprintf(manifest,"stage=%s samples=%u mean_dispatch_ms=%.9f\n",performance::Name(static_cast<performance::Stage>(i)),counts[i],counts[i]?sums[i]/counts[i]:0);
        if(fclose(manifest)!=0)++errors;manifest=nullptr;
    }
    wchar_t module[MAX_PATH]{};GetModuleFileNameW(selfModule,module,MAX_PATH);auto* slash=wcsrchr(module,L'\\');if(!slash){Complete(game,false);return;}*slash=0;
    wchar_t command[2048]{};
    swprintf_s(command,L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%s\\Package-Lighting-Performance.ps1\" -Folder \"%s\" -ModuleFolder \"%s\"",module,folder,module);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,module,&startup,&process)){++errors;Complete(game,false);return;}
    CloseHandle(process.hThread);packager=process.hProcess;phase=Phase::Package;phaseTime=GetTickCount64();
}
inline bool Start() noexcept {
    if(Active() || capture_suite::active || !performance::Prepare())return false;
    using P=rt_spot_capture_runtime::Phase;
    const auto p=rt_spot_capture_runtime::phase.load();
    if(p!=P::Idle && !(p==P::Complete && rt_spot_capture_runtime::Exported()))return false;
    if(!DrainDiagnosticLog(true) || !Offset(report.Path(),logOffset))return false;
    wchar_t root[MAX_PATH]{};wcscpy_s(root,report.Path());auto* slash=wcsrchr(root,L'\\');if(!slash)return false;*slash=0;
    SYSTEMTIME t{};GetLocalTime(&t);static unsigned serial=0;
    swprintf_s(folder,L"%s\\Performance-R180-%04u%02u%02u-%02u%02u%02u-%lu-%u",root,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId(),serial++);
    if(!CreateDirectoryW(folder,nullptr))return false;
    wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\gpu-timings.csv",folder);_wfopen_s(&csv,path,L"wb");
    swprintf_s(path,L"%s\\manifest.txt",folder);_wfopen_s(&manifest,path,L"wb");
    if(!csv || !manifest){if(csv)fclose(csv);if(manifest)fclose(manifest);csv=manifest=nullptr;return false;}
    fprintf(csv,"capture,present,stage,gpu_ms,width_or_groups,height_or_groups,depth_or_groups,hit_table_bytes,mode,rr_selected,rt_effects,timestamp_frequency,source_shader_sha256,record_index,raygen_start,raygen_bytes,hit_start,hit_stride,miss_start,miss_stride\n");
    AcquireSRWLockExclusive(&performance::lock);
    for(unsigned i=0;i<performance::count;++i){performance::samples[i].current=false;performance::samples[i].state=performance::State::Discarded;}
    ReleaseSRWLockExclusive(&performance::lock);
    rows=errors=0;for(unsigned i=0;i<performance::kStageCount;++i){sums[i]=0;counts[i]=0;}
    capturedParallel=specularParallelEnabled.load();
    capturedSpecularChoice=specularSampleChoice.load();measureStartMs=measureStartPresent=0;
    session=performance::capture.fetch_add(1)+1;performance::skipped=0;state=selection.Read();contextRequested=false;
    *mainPath=0;mainOffset={};
    if(logFile==INVALID_HANDLE_VALUE || !GetFinalPathNameByHandleW(logFile,mainPath,32768,FILE_NAME_NORMALIZED) || !Offset(mainPath,mainOffset))++errors;
    fprintf(manifest,"ray_identity=GPU_virtual_addresses_capture_local_not_shader_hashes\nrecord_index=CPU_recording_order_not_GPU_execution_order\nray_tables_may_be_reused_or_updated=1\nother_rays_mean=pools_distinct_dispatches_use_CSV\n");
    fprintf(manifest,"build=R180\nsession=%llu\nmode=%s\nsettle_ms=5000\nmeasure_ms=20000\nsample_every_presents=128\nsettings_changed=0\ngpu_waits=0\n",session,ModeName(LiveSelection::GetMode(state)));
    fprintf(manifest,"CSV dimensions are incoming native launch arguments, not TraceRay counts. Serial SPEC retains native dispatch dimensions. Shadow ray timing includes the combined native spotlight/point pass. GPU stages are per dispatch; do not sum means across different sample populations. Context capture happens after timing, uses reference rays, and cannot establish room ownership.\n");
    Settings("start");nextStatus=0;
    phase=Phase::Delay;began=phaseTime=GetTickCount64();startPresent=presentCount.load();
    ReportRRPaths("performance_capture_start");Trace("performance_capture_start session=%llu mode=%s",session,ModeName(LiveSelection::GetMode(state)));
    return true;
}
inline void Tick(HWND game,uint64_t now) noexcept {
    if(!Active())return;
    if(phase==Phase::Package){
        DWORD code=STILL_ACTIVE;
        if(GetExitCodeProcess(packager,&code) && code!=STILL_ACTIVE){CloseHandle(packager);packager=nullptr;if(code)++errors;Complete(game,code==0);}
        return;
    }
    if(specularParallelEnabled.load()!=capturedParallel){++errors;Save(game,"spec_parallel_changed");return;}
    if(specularSampleChoice.load()!=capturedSpecularChoice){++errors;Save(game,"specular_quality_changed");return;}
    if(LiveSelection::Sequence(selection.Read())!=LiveSelection::Sequence(state)){++errors;Save(game,"mode_changed");return;}
    if(phase==Phase::Delay){
        if(!game || GetForegroundWindow()!=game || IsIconic(game)){phaseTime=now;startPresent=presentCount.load();}
        if(now-began>30000){++errors;Save(game,"game_not_resumed");return;}
        if(now-phaseTime>=5000 && presentCount.load()-startPresent>=30){phase=Phase::Measure;phaseTime=now;measureStartMs=now;measureStartPresent=presentCount.load();measureParallelStart=specularParallelDispatches.load();performance::enabled=true;Trace("performance_measure_begin session=%llu",session);}
        return;
    }
    if(!game || GetForegroundWindow()!=game || IsIconic(game)){++errors;Save(game,"focus_lost");return;}
    if(phase==Phase::Measure && now>=nextStatus){Settings("sample");nextStatus=now+500;}
    const auto pending=performance::Poll(session,Emit);
    if(performance::fault.load()){++errors;Save(game,"timing_invalid");return;}
    if(phase==Phase::Measure && now-phaseTime>=20000){
        performance::enabled=false;
        const uint64_t elapsed=now-measureStartMs, presents=presentCount.load()-measureStartPresent;
        fprintf(manifest,"measure_elapsed_ms=%llu\nmeasure_presents=%llu\nmeasure_present_fps=%.6f\n",elapsed,presents,elapsed?1000.0*presents/elapsed:0.0);
        fprintf(manifest,"measure_spec_parallel_dispatches=%llu\n",specularParallelDispatches.load()-measureParallelStart);
        phase=Phase::Drain;phaseTime=now;return;
    }
    if(phase==Phase::Drain){
        if(pending && now-phaseTime<5000)return;
        if(pending){++errors;Save(game,"gpu_retirement_timeout");return;}
        phase=Phase::Context;phaseTime=now;
        contextRequested=SupportsLightContextCapture(LiveSelection::GetMode(state)) && rt_spot_capture_runtime::RequestFresh();
        fprintf(manifest,"context_requested=%u\n",unsigned(contextRequested));
        wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\scene.png",folder);
        const auto hr=screenshots::Save(game,path);fprintf(manifest,"screenshot_hr=0x%08lX\n",static_cast<unsigned long>(hr));if(FAILED(hr))++errors;
        if(!contextRequested){Save(game,"complete_timing_context_unavailable");return;}
    }
    if(phase==Phase::Context){
        rt_spot_capture_runtime::Poll();
        if(rt_spot_capture_runtime::Exported()){Save(game,"complete");return;}
        if(now-phaseTime>15000 || rt_spot_capture_runtime::phase.load()==rt_spot_capture_runtime::Phase::Failed){++errors;Save(game,"context_incomplete");}
    }
}
} // namespace native_lighting::performance_capture
