#pragma once
#include "theme.h"
#include "../relax_quality.h"
#include "render_fps.h"
#include "lighting_panel_window.h"
#include "gi_button_capture.h"
#include "../rr_gi_stage_policy.h"
#include "../rr_clamp_strength_policy.h"
#include "lighting_capture_suite.h"
#include "lighting_performance_capture.h"
#include "rr_reflection_suite.h"
#include "gi_path_capture.h"
#include "settings_check.h"
#include "performance_compare.h"

namespace native_lighting::ui {
inline constexpr wchar_t kClass[] = L"ControlFGLightingDiagnosticR54";
inline constexpr int kWidth = 490, kButtonFirst = 5400;
inline constexpr int kModeRows = (kModeCount + 2) / 3;
inline constexpr int kStatusY=746,kCaptureStatusY=766,kFooterY=812,kHeight=858;
// Preserve the original three-column controls and theme; group actions by purpose.
inline bool ButtonVisible(unsigned index) noexcept {
 if(index<kModeCount)return index!=kModeCount-1; // Native Blend moved to Visual settings.
 const unsigned local=index-kModeCount;
 return local!=7&&local!=17&&local!=20&&local!=25&&local!=26;
}
inline RECT ButtonRect(unsigned index) noexcept {
 if(index<kModeCount)return {14+int(index%3)*156,56+int(index/3)*42,162+int(index%3)*156,92+int(index/3)*42};
 constexpr unsigned features[]={8,11,9,10,12,13,14,15,16};
 constexpr unsigned comparisons[]={21,22,23,24};
 constexpr unsigned captures[]={0,1,2,3,4,5,6,18,19,27,28,29};
 const unsigned local=index-kModeCount;unsigned row=0,col=0;int top=0;
 for(unsigned i=0;i<9;++i)if(features[i]==local){row=i/3;col=i%3;top=220;}
 for(unsigned i=0;i<4;++i)if(comparisons[i]==local){row=i/3;col=i%3;top=420;}
 for(unsigned i=0;i<12;++i)if(captures[i]==local){row=i/3;col=i%3;top=552;}
 return {14+int(col)*156,top+int(row)*42,162+int(col)*156,top+int(row)*42+36};
}
#ifdef CONTROL_GI_LONG_SEQUENCE
inline constexpr unsigned kButtonCount = kModeCount + 30;
#else
inline constexpr unsigned kButtonCount = kModeCount + 16;
#endif
inline const wchar_t* ButtonLabel(unsigned index) noexcept {
    if(index==kModeCount+29)return L"PERF COMPARE";
    if(index==kModeCount+28)return settings_report::Busy()?L"CHECK: RUNNING":L"CHECK SETTINGS";
    if(index==kModeCount+27)return path_capture::active?L"GI PATH: RECORDING":L"CAPTURE GI PATH";
    if(index==kModeCount+26)return render_fps::enabled?L"FPS: ON":L"FPS: OFF";
#ifdef CONTROL_GI_LONG_SEQUENCE
    if(index==kModeCount+21)return rt_surface_source::manualGI.load()==1?L"GI REMOVE: ON":L"GI REMOVE: OFF";
    if(index==kModeCount+22)return rt_surface_source::manualGI.load()!=2?L"GI STABLE: OFF":gi_dual_bridge::status.load()==1?L"GI STABLE: RELAX":gi_dual_bridge::status.load()==2?L"GI STABLE: FALLBACK":L"GI STABLE: WAIT";
    if(index==kModeCount+23){const auto c=rt_surface_source::manualContact.load();return c==1?L"CONTACT: FILTERED":c==2?L"CONTACT: RAW":L"CONTACT: AUTO";}
    if(index==kModeCount+24)return L"RESET COMPARE";
    if(index==kModeCount+25)return rt_surface_source::manualGI.load()!=3?L"GI FILTER: OFF":gi_dual_bridge::status.load()==1?L"GI FILTER: RELAX":gi_dual_bridge::status.load()==2?L"GI: RAW FALLBACK":L"GI FILTER: WAIT";
    if(index==kModeCount+20){
        const unsigned quality=character_surface::quality.load();
        return !quality?L"RIG: NATIVE":!character_surface::ready.load()?L"RIG: WAIT":quality==1?L"RIG: R351":quality==2?L"RIG: R352":!volume_temporal::ready.load()?L"RIG: WAIT":L"RIG: STABLE";
    }
    if(index==kModeCount+19)return L"TEST VOLUME";
    if(index==kModeCount+18)return L"TEST JESSE";
    if(index==kModeCount+17){
        if(!control_rr::RRUserRequested())return L"GI RAYS: RR OFF";
        const unsigned rays=control_rr::GISampleRequested();
        if(!rays)return L"GI RAYS: NATIVE";
        static wchar_t label[32]{};
        swprintf_s(label,L"GI RAYS: %u",rays);
        return label;
    }
    if(index==kModeCount+16)return gi_evaluator::Enabled()?gi_evaluator::Label():gi_live_toggle::Label();
#endif
    if(index==kModeCount+15){
        const auto mode=control_rr_clamp::requestedMode.load();
        return mode==control_rr_clamp::Mode::Native?L"CLAMP: NATIVE":mode==control_rr_clamp::Mode::Bypass?L"REF CLAMP: BYPASS":L"REF CLAMP: 60%";
    }
    if(index==kModeCount+3&&gi_spot_trace::Enabled())return L"ARM GI TRACE";
#ifdef CONTROL_RR_FOLIAGE_AUDIT
    if(index==kModeCount+2)return L"CAPTURE MATERIALS";
#endif
    if(index==kModeCount+14)return control_rr_gi::stageMode.load()!=control_rr_gi::StageMode::ClampOnly?L"GI VIEW: N/A":control_rr_gi::ViewLabel(control_rr_gi::viewRequested.load());
    if(index==kModeCount+13)return control_rr_gi::stageMode.load()!=control_rr_gi::StageMode::ClampOnly||control_rr_gi::viewRequested.load()!=control_rr_gi::DiagnosticView::Normal?L"SMOOTH: N/A":control_rr_gi::outlierRequested.load()?L"SMOOTH: ON":L"SMOOTH: OFF";
    if(index==kModeCount+12){
        if(!control_rr::RRUserRequested())return L"RELAX: RR OFF";
        if(rt_surface_source::manualGI.load()==4)return L"RELAX: OFF";
        if(rt_surface_source::manualGI.load()==1)return L"RELAX: GI REMOVED";
        return control_relax::requested.load()==control_relax::Profile::Balanced?L"RELAX: BALANCED":L"RELAX: ORIGINAL";
    }
    if (index < kModeCount) return kModeChoices[index].label;
    if (index == kModeCount + 11) {
        const auto mode=LiveSelection::GetMode(selection.Read());
        return UsesNativeBlendProjection(mode)?L"RAYS: PROJECTION":L"RAYS: EMITTER";
    }
    if (index == kModeCount + 9) return L"FREE CAMERA";
    if (index == kModeCount + 10) return L"NORMAL CAMERA";
    constexpr const wchar_t* actions[] = {L"ISSUE REMAINS", L"ISSUE GONE", L"SAVE LOG", L"CAPTURE RR", L"PACKAGE RR", L"TRACE SHADOW", L"PERF CAPTURE"};
    if(index==kModeCount+6)return control_gi_button::Active()?L"RECORDING...":L"RECORD GI";
    if(index==kModeCount+7) {
        static wchar_t label[32]{};
        const unsigned samples=specularSampleChoice.load();
        if(samples)swprintf_s(label,L"SPEC: %u",samples);
        else wcscpy_s(label,L"SPEC: NATIVE");
        return label;
    }
    if(index==kModeCount+8) {
        const auto mode=LiveSelection::GetMode(selection.Read());
        return NativeBlendMapGuard(mode)?L"MAP GUARD: ON":IsNativeBlend(mode)?L"MAP GUARD: OFF":L"MAP GUARD";
    }
    return actions[index - kModeCount];
}
inline const wchar_t* ModeHint(Mode mode) noexcept {
    if(rt_surface_source::manualGI.load() || rt_surface_source::manualContact.load())return L"Manual comparison active. CONTACT RAW disables filtering, not shadows. RESET COMPARE restores.";
    if(gi_source_probe::Requested()) {
        if(!gi_source_probe::bound.load())return L"GI source probe requested; no matching shader bound yet. Check log.";
        if(gi_source_probe::Requested()==5)return L"GI SPOT: WHITE COOKIE. Native shadows; diagnostic only.";
        if(gi_source_probe::Requested()==6)return L"GI SPOT: FLAT FALLOFF. Native shadows; diagnostic only.";
        if(gi_source_probe::Requested()==7)return L"GI SPOT: NO FRESNEL. Native shadows; diagnostic only.";
        if(gi_source_probe::Requested()==4)return L"GI SHADOW: BYPASS. Spotlight bounce retained; diagnostic only.";
        if(gi_source_probe::Requested()==1)return L"GI SOURCE: NO POINT. Direct lights retained. Diagnostic only.";
        if(gi_source_probe::Requested()==2)return L"GI SOURCE: NO SPOT. Direct lights retained. Diagnostic only.";
        return L"GI SOURCE: NO LOCAL. Baked GI retained. Diagnostic only.";
    }
    if(IsNativeBlend(mode)&&gi_spot_pcf_probe::Enabled())return gi_spot_pcf_probe::bound.load()?L"GI SHADOW: PCF TEST. Full spotlight GI retained.":L"GI PCF test requested; no matching shader bound yet. Check log.";
    if(IsNativeBlend(mode)&&control_rr_gi::stageMode.load()==control_rr_gi::StageMode::ClampOnly&&control_rr_gi::viewRequested.load()!=control_rr_gi::DiagnosticView::Normal)return L"GI diagnostic active. Return GI VIEW to NORMAL after testing.";
    if(UsesNativeBlendProjection(mode))return L"Native Blend with projection rays. RR retained; MAP GUARD is optional.";
    if(mode==Mode::Native)return L"Native lighting. SPEC is shared with other modes; NATIVE removes its override.";
    if(mode==Mode::RoomNativeBlend)return L"Native map blockers retained with RT. MAP GUARD toggles the R199 comparison.";
    if(mode==Mode::RoomNativeBlendLegacy)return L"R199 comparison: RT with native fade. Native map blockers are not retained.";
    if(mode==Mode::RoomMapProbe)return L"Diagnostic colors. Click TRACE SHADOW; hold still. Restore DEFAULT afterward.";
    if(mode==Mode::RoomCorrectedMapPriority)return L"Test: rank RT lights using map-visible energy. RR off.";
    if(mode==Mode::RoomCorrectedOpen)return L"Test: selected RT spotlight unblocked; other shadows retained. RR off.";
    if(DisablesCorrectedBakedGi(mode))return L"Baked-brick samples removed from RT diffuse GI; shadows retained.";
    if(ShowsSpotLightIds(mode))return L"Colors=selected spotlight; magenta=no selected spotlight.";
    if(IsCorrectedSignal(mode))return L"Dark=shadow; white=lit; magenta=no selected spotlight.";
    return L"RR OFF comparisons preserve your saved RR preference.";
}
inline HFONT headingFont = nullptr, bodyFont = nullptr, detailFont = nullptr;
inline std::atomic<bool> started{false};
inline std::atomic<unsigned> savedNotice{0};
inline bool selectionRejected = false;
inline bool panelOpen = false; // Owned exclusively by the diagnostic UI thread.
inline ULONGLONG cameraRequestedAt = 0;
inline int cursorVisibilityIncrements = 0;

inline bool PanelHasForeground(HWND panel) noexcept {
    const HWND foreground = GetForegroundWindow();
    return foreground == panel || IsChild(panel, foreground);
}
inline void RestoreCursorVisibility() noexcept {
    while (cursorVisibilityIncrements > 0) {
        ShowCursor(FALSE);
        --cursorVisibilityIncrements;
    }
}
inline void ClosePanel(HWND panel) noexcept {
    const bool returnToGame = PanelHasForeground(panel);
    panelOpen = false;
    RestoreCursorVisibility();
    ShowWindow(panel, SW_HIDE);
    if (returnToGame) SetForegroundWindow(GetWindow(panel, GW_OWNER));
    Log("F10_PANEL_R337 closed=1 return_to_game=%u",returnToGame?1u:0u);
    Trace("overlay_closed return_to_game=%u", returnToGame ? 1u : 0u);
}
inline void OpenPanel(HWND panel) noexcept {
    panelOpen = true;
    // Position before first show in the timer tick to avoid a flash at (0,0).
    // Keep Control active so opening this tool does not trigger its pause menu.
    ClipCursor(nullptr);
    if (!cursorVisibilityIncrements) {
        int count;
        do { count=ShowCursor(TRUE); ++cursorVisibilityIncrements; } while(count<0);
    }
    Log("F10_PANEL_R337 open=1");
    Trace("overlay_open activation=0 preserve_game_focus=1");
}

inline void DrawLabel(HDC dc, HFONT font, const wchar_t* text, RECT bounds, COLORREF color) noexcept {
    const auto previous = SelectObject(dc, font);
    SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text, -1, &bounds, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, previous);
}
#include "gi_spot_trace_status.h"
inline void Paint(HWND window) noexcept {
    namespace theme = control_ui::theme;
    PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps);
    RECT bounds{}; GetClientRect(window, &bounds);
    HBRUSH background = CreateSolidBrush(theme::kBackground); FillRect(dc, &bounds, background); DeleteObject(background);
    DrawLabel(dc,headingFont,L"LIGHTING DIAGNOSTIC R420",{14,8,kWidth-12,32},theme::kAccent);
    DrawLabel(dc,detailFont,L"LIGHTING MODES",{14,32,kWidth-12,52},theme::kMutedText);
    DrawLabel(dc,detailFont,L"ADVANCED COMPARISONS",{14,192,kWidth-12,216},theme::kAccent);
    DrawLabel(dc,detailFont,L"FEATURE COMPARISONS",{14,392,kWidth-12,416},theme::kAccent);
    DrawLabel(dc,detailFont,L"CAPTURE",{14,524,kWidth-12,548},theme::kAccent);
    const auto mode=LiveSelection::GetMode(selection.Read());
    DrawLabel(dc,detailFont,ModeAvailable(mode,readyMask.load())?L"Selected lighting mode: ready":L"Selected lighting mode: waiting for shader setup",{14,kStatusY,kWidth-12,kStatusY+20},theme::kMutedText);
    wchar_t captureStatus[128]{};
    swprintf_s(captureStatus,L"RR capture: %s   Saved pairs: %u",control_rr_capture_ui::Label(control_rr_capture_ui::state.load()),control_rr_capture_ui::completedPairs.load());
    if(gi_spot_trace::Enabled())swprintf_s(captureStatus,L"GI trace: %s",gi_spot_trace::Status());
    DrawLabel(dc,detailFont,captureStatus,{14,kCaptureStatusY,kWidth-12,kCaptureStatusY+22},theme::kText);
    DrawLabel(dc,detailFont,performance_capture::Active()?performance_capture::Status():control_rr_capture_ui::detail.load(),{14,kCaptureStatusY+22,kWidth-12,kCaptureStatusY+46},theme::kMutedText);
    DrawLabel(dc,detailFont,L"F10: close    Captures start from buttons",{14,kFooterY,kWidth-12,kFooterY+20},theme::kMutedText);
    DrawLabel(dc,detailFont,selectionRejected?L"Mode is not ready yet; current lighting is unchanged.":L"Render FPS excludes generated frames.",{14,kFooterY+20,kWidth-12,kFooterY+40},theme::kText);
    EndPaint(window, &ps);
}
inline void ReportSpecIsolation(const char* reason) noexcept {
#if defined(CONTROL_RR_COMPACT_DISTANCE)
    Trace("rr_compact_distance build=R195 reason=%s requested=%u installed=%u evaluations=%llu bound_evaluations=%llu last_bound=%u copied_layers=1 status=%s capture_status=%s capture_bytes=%llu",
        reason,unsigned(rr_compact_distance::enabled.load()),unsigned(rr_compact_distance::installed.load()),rr_compact_distance::evaluations.load(),rr_compact_distance::boundEvaluations.load(),unsigned(rr_compact_distance::lastBound.load()),rr_compact_distance::reason.load(),rr_compact_distance::captureReason.load(),rr_compact_distance::captureBytes.load());
#elif defined(CONTROL_SPEC_VOLUME_ROUGHNESS)
    Trace("spec_volume_status build=R195 reason=%s enabled=%u diagnostic_gain=1 direct_created=%llu library_created=%llu failures=%llu evidence=creation_counters",
        reason,unsigned(spec_serial::VolumeRoughnessEnabled()),spec_serial::directCreated.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
#elif defined(CONTROL_SPEC_INDIRECT_CAPTURE)
    const auto mode=spec_serial::ComponentMode();
    Trace("spec_indirect_status build=R195 reason=%s requested=%s diagnostic_gain=%u direct_created=%llu library_created=%llu failures=%llu evidence=creation_counters",
        reason,spec_indirect::Name(mode),spec_indirect::Gain(mode),spec_serial::directCreated.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
#elif defined(CONTROL_SPEC_LOCAL_CAPTURE)
    Trace("spec_local_status reason=%s requested=%s direct_created=%llu library_created=%llu failures=%llu evidence=creation_counters",
        reason,spec_local::Name(spec_serial::ComponentMode()),spec_serial::directCreated.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
#else
    (void)reason;
#endif
}
inline void SaveReport(HWND window) noexcept {
#ifdef CONTROL_RR_FOLIAGE_AUDIT
    if(!control_rr::RRUserRuntimeWorkRequested()) {
        MessageBoxW(window,L"Enable Ray Reconstruction, return to the plant, then capture materials.",L"Material capture",MB_OK);return;
    }
    rrFoliageShaderAuditRequested.store(true,std::memory_order_release);
    Trace("RR_FOLIAGE_AUDIT_R258 requested=1 max_frames=120 max_pairs=24 image_readback=0 replay_admission_changed=0");
    control_rr_capture_ui::detail.store(L"Materials requested. Close F10, wait 10 seconds, then run Collect-ControlTOH-Materials.cmd.");
#endif
    Trace("free_camera_status installed=%u mode=%d generation=%llu pending=%u result=%u updates=%llu thread=%lu",
        unsigned(free_camera::installed.load()),free_camera::commands.Mode(),static_cast<unsigned long long>(free_camera::commands.Generation()),
        unsigned(free_camera::commands.Pending()),unsigned(free_camera::commands.Status()),free_camera::updates.load(),free_camera::updateThread.load());
    Trace("gi_view_save build=R217 requested=%s",control_rr_gi::ViewName(control_rr_gi::viewRequested.load()));
    Trace("gi_stage_save build=R217 requested=%s smooth_requested=%u",control_rr_gi::StageName(control_rr_gi::stageMode.load()),unsigned(control_rr_gi::outlierRequested.load()));
    ReportSpecIsolation("save_log");
    ReportRRPaths("save_log");
    Trace("spec_retry_save matched=%llu created=%llu failures=%llu creation_only=1",spec_retry::matched.load(),spec_retry::created.load(),spec_retry::failures.load());
    Trace("spec_serial_save direct_matched=%llu direct_created=%llu library_matched=%llu library_created=%llu failures=%llu creation_only=1",
        spec_serial::directMatched.load(),spec_serial::directCreated.load(),spec_serial::libraryMatched.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
    Trace("spec_parallel_save enabled=%u ready=%u dispatches=%llu requires_spec=16",unsigned(specularParallelEnabled.load()),unsigned(specularParallelReady.load()),specularParallelDispatches.load());
    rt_shadow_capture::Summary();
    const auto state = selection.Read();
    Trace("save_log seq=%llu mode=%s recorded_mask=%u", LiveSelection::Sequence(state),
        ModeName(LiveSelection::GetMode(state)), LiveSelection::Observed(state));
    Trace("coverage graphics_pso=%llu compute_pso=%llu bindings=%llu same_size_mismatches=%u ready_mask=%u", graphicsSeen.load(), computeSeen.load(), bindingsSeen.load(), candidateMismatches.load(), readyMask.load());
    Trace("library_coverage graphics_loads=%llu compute_loads=%llu identities=%u", libraryGraphicsSeen.load(), libraryComputeSeen.load(), identityReports.load());
    DrainDiagnosticLog(true);
#if defined(CONTROL_CAPTURE_DIAGNOSTIC)
    // Export immutable CPU shader copies here only. Creation/binding callbacks
    // never write files or wait on this report lock.
    AcquireSRWLockExclusive(&reportLock);
    const bool snapshotSaved=reportHealthy.load() && shaderSnapshot.Export(
        [](const char* bytes,size_t length){return report.Write(bytes,static_cast<DWORD>(length));});
    reportHealthy.store(snapshotSaved && report.Flush());
    ReleaseSRWLockExclusive(&reportLock);
#endif
    wchar_t message[1024]{};
    AcquireSRWLockShared(&reportLock);
    const bool saved = reportHealthy.load();
    if (saved) swprintf_s(message, L"Log saved and flushed to disk:\n\n%s\n\nSend this file. Earlier session logs are kept.", report.Path());
    else swprintf_s(message, L"The log could not be saved. Windows error: %lu\n\nAttempted path: %s\n\nPlease send a screenshot of this message.", report.Error(), *report.Path() ? report.Path() : L"LocalAppData and temporary diagnostics folders");
    ReleaseSRWLockShared(&reportLock);
    savedNotice = saved ? 1u : 2u;
    MessageBoxW(window, message, saved ? L"Lighting log saved" : L"Lighting log failed", MB_OK | (saved ? MB_ICONINFORMATION : MB_ICONERROR));
}
inline void PackageRR(HWND window) noexcept {
#if defined(CONTROL_RR_F9_SUITE) && !defined(CONTROL_GI_BOUNDARY_CAPTURE)
    if(rr_suite::session.load()){reflection_suite::Package();return;}
#endif
    wchar_t module[MAX_PATH]{};
    if(!GetModuleFileNameW(selfModule,module,MAX_PATH)){MessageBoxW(window,L"Could not locate the capture helper.",L"RR material packaging",MB_OK|MB_ICONERROR);return;}
    auto* slash=wcsrchr(module,L'\\');if(!slash)return;*slash=0;
    wchar_t command[2048]{};
    // Never inspect partially written image payloads while a worker is saving.
    // DiagnosticsOnly collects logs/metadata without waiting for another pair.
    const bool diagnosticsOnly=control_rr_capture_ui::Busy();
    swprintf_s(command,L"powershell.exe -NoExit -NoProfile -ExecutionPolicy Bypass -File \"%s\\ControlTOH\\Diagnostics\\Capture-RR-Boundary.ps1\" %s -RequireRR -GameProcessId %lu",module,diagnosticsOnly?L"-DiagnosticsOnly":L"-CollectExisting",GetCurrentProcessId());
#ifdef CONTROL_RR_GI_SEQUENCE
    wcscat_s(command,L" -GiSequence");
#elif defined(CONTROL_GI_BOUNDARY_CAPTURE)
    wcscat_s(command,L" -GiBoundary");
#endif
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,module,&startup,&process)){
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
    } else MessageBoxW(window,L"Could not start packaging. Check Capture-RR-Boundary.ps1 is installed in ControlTOH/Diagnostics.",L"RR material packaging",MB_OK|MB_ICONERROR);
}
inline void RequestLightCapture(HWND panel) noexcept {
    if(gi_spot_trace::Enabled()){if(gi_spot_trace::Request("capture_action"))ClosePanel(panel);return;}
    if(free_camera::commands.Pending())return;
#if defined(CONTROL_RR_F9_SUITE) && !defined(CONTROL_GI_BOUNDARY_CAPTURE)
    ClosePanel(panel);
    if(!reflection_suite::Start(GetTickCount64()))MessageBoxW(panel,L"Use RR ON and FG OFF. Wait for gameplay readiness, or restart after an incomplete sequence.",L"F9 reflection sequence",MB_OK);
    return;
#endif
    const auto mode=LiveSelection::GetMode(selection.Read());
#ifdef CONTROL_RR_GI_SEQUENCE
    if(control_rr_gi::stageMode.load()!=control_rr_gi::StageMode::RawBypass){
      MessageBoxW(panel,L"Select GI RAW BYPASS, then CAPTURE RR once. Keep the camera still until all four pairs are saved.",L"Stationary GI sequence",MB_OK);return;
    }
#elif defined(CONTROL_GI_BOUNDARY_CAPTURE)
    const auto expected=control_rr_capture_ui::completedPairs.load()==0?control_rr_gi::StageMode::ClampOnly:control_rr_gi::StageMode::Both;
    if(control_rr_gi::stageMode.load()!=expected || (expected==control_rr_gi::StageMode::ClampOnly&&!control_rr_gi::outlierRequested.load()) || control_rr_gi::viewRequested.load()!=control_rr_gi::DiagnosticView::Normal) {
        MessageBoxW(panel,L"Capture 1: GI CLAMP ONLY. Capture 2: GI BOTH. Use SMOOTH ON for CLAMP ONLY and GI VIEW NORMAL for both; use the same camera position.",L"GI buffer capture",MB_OK);return;
    }
#endif
    if(!performance_capture::Active() && !capture_suite::active && IsNativeBlend(mode) && control_rr::RRUserRuntimeWorkRequested() && control_rr_capture_ui::Request()) {
        Trace("rr_material_capture requested=1 mode=%s spec_requested=%u",ModeName(mode),specularSampleChoice.load());
        ClosePanel(panel);
    } else MessageBoxW(panel,L"Select NATIVE BLEND with RR enabled. Keep FG OFF. If busy, failed, or both pairs are saved, PACKAGE RR collects diagnostics.",L"Lighting capture",MB_OK);
}
inline void RequestShadowTrace(HWND panel) noexcept {
    if(performance_capture::Active())return;
    if(free_camera::commands.Pending()) {
        MessageBoxW(panel,L"Wait for the camera change to finish, then start TRACE SHADOW.",L"Shadow trace",MB_OK);return;
    }
    using State=control_rr_capture_ui::State;
    const auto light=control_rr_capture_ui::state.load();
    if(control_rr_capture_ui::requested.load() ||
       (light!=State::Idle && light!=State::Complete)) {
        MessageBoxW(panel,L"Finish the light capture first. If it failed, start a fresh game session before tracing.",L"Shadow trace",MB_OK);return;
    }
    if(capture_suite::Start(capture_suite::Kind::NativeBlend))ClosePanel(panel);
    else MessageBoxW(panel,native_blend_capture::Message(capture_suite::lastFailure),L"Shadow trace",MB_OK);
}
inline void RequestPerformanceCapture(HWND panel) noexcept {
    if(control_gi_button::Active())return;
    if(capture_suite::active || performance_capture::Active()) {
        MessageBoxW(panel,L"Finish the current capture first.",L"GI recording",MB_OK);return;
    }
    if(control_gi_button::Start(selfModule,GetWindow(panel,GW_OWNER))) {
        Log("GI_BUTTON_CAPTURE requested=1 owned_requested=%u rr_requested=%u fg_selection=%u single_producer=%u single_ray_dispatches=%llu single_eval_dispatches=%llu single_ray_active=%u temporal_dispatches=%llu",unsigned(GIOwnedPipeline::requested.load()),unsigned(control_rr::RRUserRequested()),GetFGUserMultiplier(),unsigned(gi_reuse::SingleProducerEnabled()),gi_reuse::singleRayDispatches.load(),gi_reuse::singleEvaluatorDispatches.load(),unsigned(gi_reuse::singleRayApplied.load()),gi_reuse::temporalDispatches.load());
        ClosePanel(panel);
    } else MessageBoxW(panel,L"Could not start the recorder. Make sure ControlTOH/Diagnostics/Capture-GI-Button.ps1 is installed.",L"GI recording",MB_OK|MB_ICONERROR);
}
inline void RequestCamera(HWND panel,int mode) noexcept {
    using State=control_rr_capture_ui::State;
    const auto light=control_rr_capture_ui::state.load();
    if(capture_suite::active || performance_capture::Active() || control_rr_capture_ui::requested.load() ||
       (light!=State::Idle && light!=State::Complete)) {
        MessageBoxW(panel,L"Finish or cancel the active capture before changing cameras.",L"Camera",MB_OK);return;
    }
    if(!free_camera::installed.load() || !free_camera::updates.load() || !free_camera::commands.Request(mode)) {
        MessageBoxW(panel,L"Camera controls are unavailable or a change is pending. Return to gameplay and try again. SAVE LOG records hook status.",L"Camera",MB_OK);return;
    }
    cameraRequestedAt=GetTickCount64();
    Trace("free_camera_request target=%d previous=%d",mode,free_camera::commands.Mode());
    ClosePanel(panel);
}
inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    namespace theme = control_ui::theme;
    switch (message) {
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) RestoreCursorVisibility();
        else {
            ClipCursor(nullptr);
            // Balance every ShowCursor call when focus leaves this panel.
            if (!cursorVisibilityIncrements) {
                int count;
                do { count = ShowCursor(TRUE); ++cursorVisibilityIncrements; } while (count < 0);
            }
        }
        break;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR: SetCursor(LoadCursorW(nullptr, IDC_ARROW)); return TRUE;
    case WM_PAINT: Paint(window); return 0;
    case WM_CREATE:
        for (unsigned i=0;i<kButtonCount;++i) {
            if(!ButtonVisible(i))continue;
            const RECT bounds=ButtonRect(i);
            HWND button=CreateWindowExW(0,L"STATIC",ButtonLabel(i),WS_CHILD|WS_VISIBLE|SS_NOTIFY|SS_OWNERDRAW,
                bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,window,reinterpret_cast<HMENU>(INT_PTR(kButtonFirst+i)),selfModule,nullptr);
            SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(bodyFont),TRUE);
        }
        return 0;
    case WM_DRAWITEM: {
        const auto* item=reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        const unsigned index=item->CtlID-kButtonFirst;
        if(index>=kButtonCount)return FALSE;
        const auto selected=LiveSelection::GetMode(selection.Read());
        const bool active=(index<kModeCount && (selected==kModeChoices[index].mode || (IsNativeBlend(kModeChoices[index].mode) && IsNativeBlend(selected)))) ||
            (index==kModeCount+21 && rt_surface_source::manualGI.load()==1) ||
            (index==kModeCount+22 && rt_surface_source::manualGI.load()==2) ||
            (index==kModeCount+23 && rt_surface_source::manualContact.load()!=0) || (index==kModeCount+25 && rt_surface_source::manualGI.load()==3) || (index==kModeCount+26 && render_fps::enabled);
        HBRUSH brush=CreateSolidBrush(active?theme::kAccent:RGB(35,35,35)); FillRect(item->hDC,&item->rcItem,brush); DeleteObject(brush);
        HBRUSH border=CreateSolidBrush(theme::kBorder); FrameRect(item->hDC,&item->rcItem,border); DeleteObject(border);
        const auto previous=SelectObject(item->hDC,bodyFont); SetBkMode(item->hDC,TRANSPARENT); SetTextColor(item->hDC,theme::kText);
        RECT rect=item->rcItem; DrawTextW(item->hDC,ButtonLabel(index),-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE); SelectObject(item->hDC,previous);
        return TRUE;
    }
    case WM_COMMAND:
        if(HIWORD(wParam)==BN_CLICKED) {
            const unsigned index=LOWORD(wParam)-kButtonFirst;
            if(path_capture::active||settings_report::Busy()||compare_capture::Active()||!ButtonVisible(index))return 0;
            if(index==kModeCount+29){
                if(compare_capture::Start(GetTickCount64()))ClosePanel(window);
                else MessageBoxW(window,L"Enable RR, turn FG off and finish other captures. Load the scene until enhanced lighting is ready. Keep the camera still with Jesse visible for about five minutes. The suite changes temporary feature selections automatically and restores them. Two beeps mean the ZIP is ready. Restart if the profiler query budget is exhausted.",L"RR performance comparison",MB_OK);
                return 0;
            }
            if(index==kModeCount+28){
                if(settings_report::Start(GetTickCount64()))ClosePanel(window);
                else MessageBoxW(window,L"Finish other captures first. The report needs a writable LOCALAPPDATA/ControlFGProbe folder. Click CHECK SETTINGS, return to gameplay, and keep Jesse visible. One beep after five seconds starts the ten-second check. Two beeps mean the ZIP is ready.",L"Settings check",MB_OK);
                return 0;
            }
            if(index==kModeCount+27){
                if(path_capture::Start(GetTickCount64()))ClosePanel(window);
                else MessageBoxW(window,L"Use Native Blend, RR ON and FG OFF. Finish other captures first. After two earlier RR pairs or a failed capture, restart the game. Hold still for both conditions; two beeps mean the ZIP is ready.",L"GI path capture",MB_OK);
                return 0;
            }
            if(index==kModeCount+26){render_fps::enabled=!render_fps::enabled;InvalidateRect(GetDlgItem(window,kButtonFirst+index),nullptr,FALSE);return 0;}
            if((index>=kModeCount&&index<=kModeCount+6)||index==kModeCount+18||index==kModeCount+19)control_diagnostics::captureActive=true;
            const bool traceAction=index==kModeCount+3&&gi_spot_trace::Enabled();
            if(traceAction){if(gi_spot_trace::Request("overlay_button"))ClosePanel(window);InvalidateRect(window,nullptr,FALSE);return 0;}
            const bool recoveryAction=index==kModeCount+2||index==kModeCount+4;
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
            if(!control_rr_capture_ui::ActionAllowed(recoveryAction))return 0;
#endif
#ifdef CONTROL_RR_F9_SUITE
            if(reflection_suite::active&&!recoveryAction)return 0;
#endif
            if(performance_capture::Active() && index!=kModeCount+6&&!recoveryAction)return 0;
            if(index<kModeCount){selectionRejected=!SelectMode(kModeChoices[index].mode);savedNotice=0;}
            else if(index==kModeCount||index==kModeCount+1) {
                if (!ReportVisual(index==kModeCount))
                    MessageBoxW(window, L"This mode has not applied yet. Press F10 to return to the game, wait for the image to settle, then reopen the panel and record your result.", L"Wait for mode to apply", MB_OK);
            }
#ifdef CONTROL_GI_LONG_SEQUENCE
            else if(index>=kModeCount+21 && index<=kModeCount+25 && !capture_suite::active && !control_gi_button::Active()){
                if(index==kModeCount+25){if(control_rr_gi::ownedReady.load())rt_surface_source::manualGI.store(rt_surface_source::manualGI.load()==3?0:3);}
                else if(index==kModeCount+24){rt_surface_source::ResetManual();control_relax::requested=control_relax::Profile::Balanced;}
                else if(index==kModeCount+23)rt_surface_source::manualContact.store((rt_surface_source::manualContact.load()+1)%3);
                else {const unsigned target=index==kModeCount+21?1:2;rt_surface_source::manualGI.store(rt_surface_source::manualGI.load()==target?0:target);}
                Trace("manual_rt_compare gi=%u contact=%u",rt_surface_source::manualGI.load(),rt_surface_source::manualContact.load());
            }
            else if(index==kModeCount+20 && !capture_suite::active && !control_gi_button::Active()){
                character_surface::quality.store((character_surface::quality.load()+1)%4);
                Trace("character_rig_filter quality=%u recorded_binds=%llu",character_surface::quality.load(),character_surface::filteredBinds.load());
            }
            else if((index==kModeCount+18 || index==kModeCount+19) && !capture_suite::active && !control_gi_button::Active()){
                if(capture_suite::Start(index==kModeCount+19?capture_suite::Kind::Volume:capture_suite::Kind::Character))ClosePanel(window);
                else MessageBoxW(window,L"Finish the active capture and return to gameplay before starting.",L"Character shadows",MB_OK);
            }
            else if(index==kModeCount+17 && !capture_suite::active && !control_gi_button::Active()){
                std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
                const auto& option=control_settings::Definition(control_settings::Key::GlobalIlluminationSamples);
                const unsigned next=control_rr::NextGISampleRequest(control_rr::GISampleRequested());
                if(control_settings::Write(option,next))FGSettingsMarkSettingsDirty();
            }
            else if(index==kModeCount+16 && !capture_suite::active) {
                if(gi_evaluator::Enabled())gi_evaluator::Toggle();
                else if(gi_live_toggle::ready.load())gi_live_toggle::Toggle();
            }
#endif
            else if(index==kModeCount+15 && !capture_suite::active) {
                control_rr_clamp::requestedMode.store(control_rr_clamp::Next(control_rr_clamp::requestedMode.load()));
                Trace("REF_CLAMP requested=%u",control_rr_clamp::RuntimeRequested());
            }
            else if(index==kModeCount+2) SaveReport(window);
            else if(index==kModeCount+3) RequestLightCapture(window);
            else if(index==kModeCount+4) PackageRR(window);
            else if(index==kModeCount+5) RequestShadowTrace(window);
            else if(index==kModeCount+6) RequestPerformanceCapture(window);
            else if(index==kModeCount+7 && !capture_suite::active) {
                const unsigned next=NextSpecularSampleChoice(specularSampleChoice.load());
                if(next && !specularSamplesReady.load()) {
                    MessageBoxW(window,L"Specular sampling is not ready. Return to gameplay and try again.",L"Specular quality",MB_OK);
                } else {
                    SetSpecularSampleChoice(next);
                    Trace("specular_quality user_request=%u independent_of_lighting=1",next);
                }
            }
            else if(index==kModeCount+8 && !capture_suite::active) {
                const auto current=LiveSelection::GetMode(selection.Read());
                const auto next=NativeBlendMode(!NativeBlendMapGuard(current),IsNativeBlend(current)?UsesNativeBlendProjection(current):UsesNativeBlendProjection(kNativeBlendDefaultMode));
                selectionRejected=!SelectMode(next);savedNotice=0;
            }
            else if(index==kModeCount+14 && !capture_suite::active && control_rr_gi::stageMode.load()==control_rr_gi::StageMode::ClampOnly) {
                const auto next=static_cast<control_rr_gi::DiagnosticView>((unsigned(control_rr_gi::viewRequested.load())+1)%3);
                control_rr_gi::viewRequested.store(next,std::memory_order_release);
                Trace("gi_view_request build=R217 view=%s",control_rr_gi::ViewName(next));savedNotice=0;
            }
            else if(index==kModeCount+13 && control_rr_gi::viewRequested.load()==control_rr_gi::DiagnosticView::Normal && !capture_suite::active && control_rr_gi::stageMode.load()==control_rr_gi::StageMode::ClampOnly) {
                const bool next=!control_rr_gi::outlierRequested.load();
                control_rr_gi::outlierRequested.store(next,std::memory_order_release);
                Trace("gi_outlier_request build=R217 enabled=%u clamp_only=1",unsigned(next));
                savedNotice=0;
            }
            else if(index==kModeCount+12 && !capture_suite::active && !control_gi_button::Active() && control_rr::RRUserRequested()) {
                rt_surface_source::manualGI.store(control_relax::Cycle(rt_surface_source::manualGI.load()));
                control_rr_gi::stageMode=control_rr_gi::StageMode::RawBypass;
                Trace("RELAX_R420 requested=%s bypass=%u",control_relax::Name(control_relax::requested.load()),unsigned(rt_surface_source::manualGI.load()==4));
                savedNotice=0;
            }
            else if(index==kModeCount+9) RequestCamera(window,1);
            else if(index==kModeCount+10) RequestCamera(window,0);
            else if(index==kModeCount+11 && !capture_suite::active) {
                const auto current=LiveSelection::GetMode(selection.Read());
                const auto next=NativeBlendMode(IsNativeBlend(current)?NativeBlendMapGuard(current):NativeBlendMapGuard(kNativeBlendDefaultMode),!UsesNativeBlendProjection(current));
                selectionRejected=!SelectMode(next);savedNotice=0;
                Trace("native_blend_ray_request projection=%u accepted=%u",unsigned(UsesNativeBlendProjection(next)),unsigned(!selectionRejected));
            }
            RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
            return 0;
        }
        break;
    case WM_CLOSE: ClosePanel(window); return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
inline void UpdateCaptureActivity() noexcept {
 const auto tracePhase=gi_spot_trace::phase.load();
 const bool traceActive=tracePhase!=gi_spot_trace::Phase::Idle && tracePhase!=gi_spot_trace::Phase::Complete && tracePhase!=gi_spot_trace::Phase::Failed;
 const bool active=path_capture::active||traceActive||capture_suite::active||performance_capture::Active()||control_gi_button::Active()||control_rr_capture_ui::Busy()||control_rr_capture_ui::requested.load()
#ifdef CONTROL_RR_F9_SUITE
 ||reflection_suite::active
#endif
 ;
 if(!active&&control_diagnostics::captureActive.load())DrainDiagnosticLog(true);
 control_diagnostics::captureActive.store(active,std::memory_order_relaxed);
}
inline DWORD WINAPI ThreadProc(LPVOID) noexcept {
    const HRESULT comResult=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    namespace theme=control_ui::theme;
    headingFont=theme::CreateFontForStyle(theme::kHeadingFont);
    bodyFont=theme::CreateFontForStyle(theme::kHeadingFont);
    detailFont=theme::CreateFontForStyle(theme::kDetailFont);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=selfModule;wc.lpfnWndProc=WindowProc;wc.lpszClassName=kClass;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){Trace("overlay_failed reason=class");started=false;return 0;}
    UINT_PTR timer=SetTimer(nullptr,0,50,nullptr);
    if(!timer){Trace("overlay_failed reason=timer");started=false;return 0;}
    HWND panel=nullptr,owner=nullptr;bool held[kModeCount+1]{};
    ULONGLONG nextRefresh=0, nextWindowSearch=0, nextLogDrain=0, nextRRReport=0;
    Log("F10_PANEL_R337 thread_ready=1");
    HWND cachedGame=nullptr;
    constexpr int f10Id=0x5337;
    bool f10Registered=false;
    auto paintedMode=LiveSelection::GetMode(selection.Read());
    POINT lastOrigin{-1,-1};
    Trace("session_start build=R195-overlay-rr-capture pid=%lu default=authored_clip_surface_normals_current_spot_when_ready log=user-diagnostics-folder",GetCurrentProcessId());
#ifdef CONTROL_RR_COMPONENT_CAPTURE
    Trace("spec_component_session requested=%s direct_created=%llu library_created=%llu failures=%llu",
        spec_component::Name(spec_serial::ComponentMode()),
        spec_serial::directCreated.load(),spec_serial::libraryCreated.load(),spec_serial::failures.load());
#endif
    MSG msg{};
    while(GetMessageW(&msg,nullptr,0,0)>0){
        if(msg.message==WM_HOTKEY && msg.hwnd==nullptr && msg.wParam==f10Id){
            if(lighting_panel::Valid(panel,owner,kClass) &&
               lighting_panel::Foreground(owner,panel,GetForegroundWindow()) && !capture_suite::active && !settings_report::active && !compare_capture::Active()
#ifdef CONTROL_RR_F9_SUITE
               && !reflection_suite::active
#endif
            ){
                if(panelOpen)ClosePanel(panel);else OpenPanel(panel);
            }
            continue;
        }
        if (msg.hwnd && (!panel || !IsDialogMessageW(panel, &msg))) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if(msg.message!=WM_TIMER||msg.wParam!=timer)continue;
        const ULONGLONG now=GetTickCount64();
        if(control_diagnostics::Active())gi_spot_trace::InputTick();
        if(cameraRequestedAt && now-cameraRequestedAt>=3000 && free_camera::commands.Timeout())
            Trace("free_camera_request_expired reason=no_camera_update target_not_applied=1");
        if(control_diagnostics::Active() && now>=nextRRReport){ReportSpecIsolation("heartbeat");ReportRRPaths("heartbeat");nextRRReport=now+5000;}
        if(control_diagnostics::Active() && now>=nextLogDrain){DrainDiagnosticLog();nextLogDrain=now+((capture_suite::active||performance_capture::Active())?100:500);}
        if(cachedGame&&!IsWindow(cachedGame))cachedGame=nullptr;
        if(now>=nextWindowSearch){
            cachedGame=FindFGGameWindow();
            nextWindowSearch=now+1000;
        }
        HWND game=cachedGame;
        render_fps::Tick(game,now);
        settings_report::Tick(game,now);
        compare_capture::Tick(game,now);
        UpdateCaptureActivity();
        gi_trace_status::Tick(game);
        if(panel && (owner!=game || !lighting_panel::Valid(panel,owner,kClass))){
            if(lighting_panel::Valid(panel,owner,kClass))DestroyWindow(panel);
            Log("F10_PANEL_R337 replacing_window=1");
            panel=nullptr;owner=nullptr;panelOpen=false;RestoreCursorVisibility();
            lastOrigin={-1,-1};
        }
        if(game&&!panel){panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,kClass,L"Control lighting diagnostic",WS_POPUP|WS_CLIPCHILDREN,
            0,0,kWidth,kHeight,game,nullptr,selfModule,nullptr);owner=game;}
#ifdef CONTROL_RR_F9_SUITE
        if(!panel)reflection_suite::Tick(nullptr,now);
#endif
        const HWND foregroundWindow=GetForegroundWindow();
        const bool foreground=panel && lighting_panel::Foreground(game,panel,foregroundWindow);
        if(foreground && !f10Registered)f10Registered=RegisterHotKey(nullptr,f10Id,MOD_NOREPEAT,VK_F10)!=FALSE;
        if(!foreground && f10Registered){UnregisterHotKey(nullptr,f10Id);f10Registered=false;}
        if(!panel){path_capture::Tick(nullptr,now);if(performance_capture::Active())performance_capture::Tick(nullptr,now);if(capture_suite::active)capture_suite::Finish(nullptr,"game_window_lost");continue;}
        // Visual settings own startup lighting selection on the render thread.
#ifndef CONTROL_ON_DEMAND_DIAGNOSTICS
        static bool f9Held=false;
        const bool f9Down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
        const bool capturePress=f9Down&&!f9Held && !(GetAsyncKeyState(VK_CONTROL)&0x8000) && !(GetAsyncKeyState(VK_MENU)&0x8000);
        f9Held=f9Down;
        if(!free_camera::commands.Pending() && !performance_capture::Active() && capturePress && (GetForegroundWindow()==game || PanelHasForeground(panel))) {
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
            RequestLightCapture(panel);
#elif defined(CONTROL_RR_F9_SUITE)
            if(capture_suite::active)capture_suite::Finish(game,"cancelled");
            else if(reflection_suite::active)reflection_suite::Cancel("cancelled");
            else {if(panelOpen)ClosePanel(panel);
                if(!reflection_suite::Start(now))MessageBoxW(game,L"Return to gameplay with RR ON and FG OFF. Wait for reflection shaders to be ready. Two complete locations are supported per launch; restart after an incomplete sequence.",L"F9 reflection sequence",MB_OK);}
#else
            if(GetAsyncKeyState(VK_SHIFT)&0x8000) RequestLightCapture(panel);
            else if(capture_suite::active)capture_suite::Finish(game,"cancelled");
            else {if(panelOpen)ClosePanel(panel);
                if(!capture_suite::Start())MessageBoxW(game,L"Could not start capture. Return to gameplay and wait for Default and GPU capture readiness. If a previous capture was cancelled, let its export finish. Use Save Log for details.",L"F9 capture",MB_OK|MB_ICONERROR);}
#endif
        }
#endif
#ifdef CONTROL_RR_F9_SUITE
        reflection_suite::Tick(game,now);
#endif
        path_capture::Tick(game,now);
        capture_suite::Tick(game,now);
        performance_capture::Tick(game,now);

        const bool modifier=(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_SHIFT)&0x8000);
        static_assert(kModeCount <= 9, "Numeric shortcuts support nine choices");
        for(int i=0;i<int(kModeCount+1);++i){const bool down=(GetAsyncKeyState(i == 0 ? VK_F10 : '0' + i)&0x8000)!=0;
            if(
#ifdef CONTROL_RR_F9_SUITE
               !reflection_suite::active&&
#endif
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
               (!control_rr_capture_ui::Busy()||i==0)&&
#endif
               !capture_suite::active&&!settings_report::active&&!compare_capture::Active()&&(i!=0||!f10Registered)&&foreground&&down&&!held[i]&&(i==0||modifier)&&(!performance_capture::Active()||i==0)){
                if(i==0){if(panelOpen)ClosePanel(panel);else OpenPanel(panel);}else {selectionRejected=!SelectMode(kModeChoices[i-1].mode);savedNotice=0;}
            }held[i]=down;
        }
        if(foreground&&panelOpen){
            RECT client{};GetClientRect(game,&client);POINT origin{client.right-kWidth-18,18};if(origin.x<0)origin.x=0;
            ClientToScreen(game,&origin);
            if(!IsWindowVisible(panel)||origin.x!=lastOrigin.x||origin.y!=lastOrigin.y){
                SetWindowPos(panel,HWND_TOP,origin.x,origin.y,kWidth,kHeight,SWP_NOACTIVATE|SWP_SHOWWINDOW);
                lastOrigin=origin;
            }
            // Release confinement only while this panel owns input, never over other apps.
            if (GetForegroundWindow()==game || PanelHasForeground(panel)) ClipCursor(nullptr);
            if(GetTickCount64()>=nextRefresh){InvalidateRect(panel,nullptr,FALSE);
                const auto mode=LiveSelection::GetMode(selection.Read());
                for(unsigned i=0;i<kButtonCount;++i){
                    HWND button=GetDlgItem(panel,kButtonFirst+i);
                    wchar_t previous[64]{};
                    GetWindowTextW(button,previous,64);
                    const wchar_t* label=ButtonLabel(i);
                    if(lstrcmpW(previous,label)!=0){SetWindowTextW(button,label);InvalidateRect(button,nullptr,FALSE);}
                    else if(i<kModeCount && mode!=paintedMode)InvalidateRect(button,nullptr,FALSE);
                }
                paintedMode=mode;nextRefresh=GetTickCount64()+250;}
        }else {RestoreCursorVisibility();if(IsWindowVisible(panel))ShowWindow(panel,SW_HIDE);if(!foreground && foregroundWindow)panelOpen=false;}
    }
    DrainDiagnosticLog(true);
    gi_trace_status::Close();
    if(f10Registered)UnregisterHotKey(nullptr,f10Id);
    KillTimer(nullptr,timer);RestoreCursorVisibility();if(panel)DestroyWindow(panel);
    DeleteObject(headingFont);DeleteObject(bodyFont);DeleteObject(detailFont);started=false;if(SUCCEEDED(comResult))CoUninitialize();return 0;
}
} // namespace native_lighting::ui
namespace native_lighting {
inline void StartOverlay() noexcept {
    if(ui::started.exchange(true))return;
    HANDLE thread=CreateThread(nullptr,0,ui::ThreadProc,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);else {ui::started=false;Trace("overlay_failed reason=thread");}
}
}

