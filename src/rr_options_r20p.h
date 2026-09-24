#pragma once
// r20r RR option-state extension (retains the r20p lifecycle implementation). The frozen Streamline frame bridge remains
// byte-for-byte unchanged; this header owns only the later RR option lifecycle.
// It reuses the validated Phase-7 input readers/base option builder but keeps a
// separate state machine so AA-mode/extent changes and explicit RR teardown can
// be handled without modifying the frozen FG/HDR frame bridge.

struct RR20POptionsSignature {
    unsigned int renderWidth{};
    unsigned int renderHeight{};
    unsigned int outputWidth{};
    unsigned int outputHeight{};
    unsigned int preset{};
};

static std::atomic<unsigned int> rr20pOptionsApplied{0};
static std::atomic<unsigned int> rr20pOptionsQueryFinished{0};
static std::atomic<unsigned int> rr20pOptionsAttempts{0};
static std::atomic<unsigned int> rr20pOptionsRebuilds{0};
static SRWLOCK rr20pOptionsLock = SRWLOCK_INIT;
static RR20POptionsSignature rr20pOptionsSignature{};
static bool rr20pOptionsSignatureValid=false;
static sl::DLSSMode rr20pOptionsResolvedMode=sl::DLSSMode::eOff;

static RR20POptionsSignature MakeRR20POptionsSignature(const RRPhase7NgxOptionsInputs& in) noexcept {
    return {in.renderWidth,in.renderHeight,in.outputWidth,in.outputHeight,control_rr::RRUserPresetValue()};
}
static bool RR20POptionsSignatureEqual(const RR20POptionsSignature&a,const RR20POptionsSignature&b) noexcept {
    return a.renderWidth==b.renderWidth&&a.renderHeight==b.renderHeight&&
           a.outputWidth==b.outputWidth&&a.outputHeight==b.outputHeight&&a.preset==b.preset;
}
static void ResetRR20POptionsState(const char* reason,unsigned long long call,unsigned long long present,
                                   const RR20POptionsSignature* before,const RR20POptionsSignature* after) noexcept {
    const auto rebuild=rr20pOptionsRebuilds.fetch_add(1,std::memory_order_relaxed)+1;
    rr20pOptionsApplied.store(0,std::memory_order_release);
    rr20pOptionsQueryFinished.store(0,std::memory_order_release);
    rr20pOptionsAttempts.store(0,std::memory_order_release);
    rr20pOptionsSignature={};rr20pOptionsSignatureValid=false;rr20pOptionsResolvedMode=sl::DLSSMode::eOff;
    Log("SL_RR_OPTIONS_REBUILD call=%llu present=%llu rebuild=%u reason=%s old_render=%ux%u new_render=%ux%u old_output=%ux%u new_output=%ux%u old_preset=%u new_preset=%u frozen_streamline_frame=1",
        call,present,rebuild,reason?reason:"unknown",
        before?before->renderWidth:0u,before?before->renderHeight:0u,
        after?after->renderWidth:0u,after?after->renderHeight:0u,
        before?before->outputWidth:0u,before?before->outputHeight:0u,
        after?after->outputWidth:0u,after?after->outputHeight:0u,
        before?before->preset:0u,after?after->preset:0u);
}
static bool RR20POptionsReadyForPreset(unsigned preset) noexcept {
    bool ready=false;
    AcquireSRWLockShared(&rr20pOptionsLock);
    ready=rr20pOptionsApplied.load(std::memory_order_relaxed)!=0&&rr20pOptionsSignatureValid&&
          rr20pOptionsSignature.preset==preset;
    ReleaseSRWLockShared(&rr20pOptionsLock);
    return ready;
}

static sl::DLSSDPreset RR20PSelectedPreset() noexcept {
    return static_cast<sl::DLSSDPreset>(control_rr::RRUserPresetValue());
}
static void RR20PApplySelectedPreset(sl::DLSSDOptions& options) noexcept {
    const auto preset=RR20PSelectedPreset();
    options.dlaaPreset=preset;
    options.qualityPreset=preset;
    options.balancedPreset=preset;
    options.performancePreset=preset;
    options.ultraPerformancePreset=preset;
    options.ultraQualityPreset=preset;
}

static void ConfigureRR20POptionsForAA(unsigned long long call,unsigned long long currentPresent,
                                       bool cameraKnown,const CameraSnapshot& camera) noexcept {
    if(!slDeviceConfigured.load()||!slRrFeatureLoaded.load()||!slRrFeatureSupported.load()||
       !slRrFunctionTableReady.load()||!slDLSSDGetOptimalSettingsApi||!slDLSSDSetOptionsApi||!slDLSSDGetStateApi)return;

    RRPhase7NgxOptionsInputs in{};
    const bool queried=ReadRRPhase7NgxOptionsInputs(&in);
    const bool complete=queried&&RRPhase7NgxOptionsComplete(in);

    AcquireSRWLockExclusive(&rr20pOptionsLock);
    const unsigned pauseReasons=control_rr::RRUserRuntimePauseReasons();
    const bool runtimePaused=pauseReasons!=0;
    const bool presetOptionsWindow=control_rr::RRUserPresetOptionsWindow();
    const bool presetOnlyPause=runtimePaused&&(pauseReasons&~control_rr::RRRuntimePausePreset)==0;
    const bool allowPresetRebuild=presetOnlyPause&&presetOptionsWindow;
    if(!control_rr::RRUserRequested()||(runtimePaused&&!allowPresetRebuild)){
        // Streamline RR must be explicitly disabled while OFF and while a
        // renderer-state epoch is quiescing. A preset-only transition gets one
        // controlled rebuild window after old GPU work has retired. Teardown deliberately does
        // not require a valid world camera because menus/quit can remove it.
        if(rr20pOptionsApplied.load(std::memory_order_relaxed)){
            sl::DLSSDOptions off{};
            off.mode=sl::DLSSMode::eOff;
            if(in.outputWidthStatus==1&&in.outputWidth)off.outputWidth=in.outputWidth;
            else if(rr20pOptionsSignatureValid)off.outputWidth=rr20pOptionsSignature.outputWidth;
            if(in.outputHeightStatus==1&&in.outputHeight)off.outputHeight=in.outputHeight;
            else if(rr20pOptionsSignatureValid)off.outputHeight=rr20pOptionsSignature.outputHeight;
            off.sharpness=0.0f;
            off.colorBuffersHDR=sl::Boolean::eTrue;
            const sl::Result offResult=slDLSSDSetOptionsApi(slFgViewport,off);
            Log("SL_RR_OPTIONS_DISABLED call=%llu present=%llu result=%lld previous_mode=%s previous_mode_value=%u render=%ux%u output=%ux%u queried=%u complete=%u explicit_eoff=1 runtime_paused=%u runtime_pause_reasons=0x%X",
                call,currentPresent,SLResultCode(offResult),RRDLSSModeName(rr20pOptionsResolvedMode),unsigned(rr20pOptionsResolvedMode),
                in.renderWidth,in.renderHeight,in.outputWidth,in.outputHeight,unsigned(queried),unsigned(complete),unsigned(runtimePaused),control_rr::RRUserRuntimePauseReasons());
            if(offResult!=sl::Result::eOk){ReleaseSRWLockExclusive(&rr20pOptionsLock);return;}
        }
        if(rr20pOptionsQueryFinished.load(std::memory_order_relaxed)||rr20pOptionsSignatureValid||
           rr20pOptionsApplied.load(std::memory_order_relaxed)){
            const RR20POptionsSignature before=rr20pOptionsSignature;
            const RR20POptionsSignature after=complete?MakeRR20POptionsSignature(in):RR20POptionsSignature{};
            ResetRR20POptionsState(runtimePaused?"runtime_epoch_pause_explicit_eoff":"rr_user_disabled_explicit_eoff",call,currentPresent,
                rr20pOptionsSignatureValid?&before:nullptr,complete?&after:nullptr);
        }
        ReleaseSRWLockExclusive(&rr20pOptionsLock);return;
    }

    if(!complete){
        const unsigned int attempt=rr20pOptionsAttempts.load(std::memory_order_acquire)+1u;
        Log("SL_RR_OPTIONS_SKIP call=%llu present=%llu attempt=%u reason=ngx_inputs_incomplete fault=0x%08lX statuses=%u,%u,%u,%u,%u,%u render=%ux%u output=%ux%u r20r=1",
            call,currentPresent,attempt,in.fault,in.renderWidthStatus,in.renderHeightStatus,in.outputWidthStatus,in.outputHeightStatus,
            in.preExposureStatus,in.exposureScaleStatus,in.renderWidth,in.renderHeight,in.outputWidth,in.outputHeight);
        ReleaseSRWLockExclusive(&rr20pOptionsLock);return;
    }

    const RR20POptionsSignature current=MakeRR20POptionsSignature(in);
    if(!cameraKnown){ReleaseSRWLockExclusive(&rr20pOptionsLock);return;}
    if(rr20pOptionsSignatureValid&&!RR20POptionsSignatureEqual(rr20pOptionsSignature,current)){
        const RR20POptionsSignature before=rr20pOptionsSignature;
        ResetRR20POptionsState("aa_mode_or_extent_changed",call,currentPresent,&before,&current);
    }
    if(rr20pOptionsQueryFinished.load(std::memory_order_relaxed)&&rr20pOptionsSignatureValid&&
       RR20POptionsSignatureEqual(rr20pOptionsSignature,current)){
        ReleaseSRWLockExclusive(&rr20pOptionsLock);return;
    }

    const unsigned int attempt=rr20pOptionsAttempts.fetch_add(1,std::memory_order_acq_rel)+1;
    if(attempt>16){ReleaseSRWLockExclusive(&rr20pOptionsLock);return;}
    const sl::DLSSMode modes[]={sl::DLSSMode::eDLAA,sl::DLSSMode::eMaxQuality,sl::DLSSMode::eBalanced,
        sl::DLSSMode::eMaxPerformance,sl::DLSSMode::eUltraPerformance,sl::DLSSMode::eUltraQuality};
    sl::DLSSMode bestMode=sl::DLSSMode::eOff;unsigned long long bestError=~0ull;
    sl::DLSSDOptimalSettings bestSettings{};sl::Result bestResult=sl::Result::eErrorMissingInputParameter;
    for(sl::DLSSMode mode:modes){
        sl::DLSSDOptions options{};FillRRPhase7BaseOptions(in,camera,mode,&options);RR20PApplySelectedPreset(options);options.normalRoughnessMode=sl::DLSSDNormalRoughnessMode::ePacked;
        // Streamline RR ignores normal DLSS sharpness. Keep it neutral and do
        // native SR/DLAA sharpening only in Control's original AA wrapper.
        options.sharpness=0.0f;
        sl::DLSSDOptimalSettings settings{};const sl::Result result=slDLSSDGetOptimalSettingsApi(options,settings);
        unsigned long long error=~0ull;
        if(result==sl::Result::eOk&&settings.optimalRenderWidth&&settings.optimalRenderHeight){
            const long long dx=static_cast<long long>(settings.optimalRenderWidth)-static_cast<long long>(in.renderWidth);
            const long long dy=static_cast<long long>(settings.optimalRenderHeight)-static_cast<long long>(in.renderHeight);
            error=static_cast<unsigned long long>(dx<0?-dx:dx)+static_cast<unsigned long long>(dy<0?-dy:dy);
            if(error<bestError){bestError=error;bestMode=mode;bestSettings=settings;bestResult=result;}
        }
        Log("SL_RR_OPTIMAL_CANDIDATE call=%llu present=%llu mode=%s mode_value=%u result=%lld output=%ux%u current_render=%ux%u optimal=%ux%u min=%ux%u max=%ux%u sharpness=%.9g error_pixels=%llu r20r=1",
            call,currentPresent,RRDLSSModeName(mode),unsigned(mode),SLResultCode(result),in.outputWidth,in.outputHeight,in.renderWidth,in.renderHeight,
            settings.optimalRenderWidth,settings.optimalRenderHeight,settings.renderWidthMin,settings.renderHeightMin,settings.renderWidthMax,settings.renderHeightMax,
            double(settings.optimalSharpness),error==~0ull?0xffffffffffffffffull:error);
    }
    rr20pOptionsQueryFinished.store(1,std::memory_order_release);
    const unsigned long long tolerance=8;
    if(bestResult!=sl::Result::eOk||bestMode==sl::DLSSMode::eOff||bestError>tolerance){
        Log("SL_RR_OPTIONS_SKIP call=%llu present=%llu attempt=%u reason=no_matching_dlss_mode current_render=%ux%u output=%ux%u best_mode=%s best_error_pixels=%llu tolerance=%llu r20r=1",
            call,currentPresent,attempt,in.renderWidth,in.renderHeight,in.outputWidth,in.outputHeight,RRDLSSModeName(bestMode),bestError,tolerance);
        ReleaseSRWLockExclusive(&rr20pOptionsLock);return;
    }
    sl::DLSSDOptions selected{};FillRRPhase7BaseOptions(in,camera,bestMode,&selected);RR20PApplySelectedPreset(selected);selected.sharpness=0.0f;selected.normalRoughnessMode=sl::DLSSDNormalRoughnessMode::ePacked;
    const sl::Result setResult=slDLSSDSetOptionsApi(slFgViewport,selected);
    sl::DLSSDState state{};const sl::Result stateResult=setResult==sl::Result::eOk?slDLSSDGetStateApi(slFgViewport,state):sl::Result::eErrorFeatureFailedToLoad;
    const bool success=setResult==sl::Result::eOk;
    if(success){rr20pOptionsApplied.store(1,std::memory_order_release);rr20pOptionsSignature=current;rr20pOptionsSignatureValid=true;rr20pOptionsResolvedMode=bestMode;}
    Log("SL_RR_OPTIONS_HANDSHAKE call=%llu present=%llu attempt=%u success=%u mode=%s mode_value=%u render=%ux%u output=%ux%u optimal=%ux%u pre_exposure=%.9g exposure_scale=%.9g sharpness=0 rr_sharpness_ignored_by_api=1 hdr_input=1 normal_roughness=packed preset=%s preset_value=%u set_result=%lld state_result=%lld estimated_vram=%llu rr_evaluate=0 native_denoiser_bypass=0 r20r=1",
        call,currentPresent,attempt,unsigned(success),RRDLSSModeName(bestMode),unsigned(bestMode),in.renderWidth,in.renderHeight,in.outputWidth,in.outputHeight,
        bestSettings.optimalRenderWidth,bestSettings.optimalRenderHeight,double(in.preExposure),double(in.exposureScale),control_rr::RRUserPresetLabel(),control_rr::RRUserPresetValue(),SLResultCode(setResult),SLResultCode(stateResult),
        static_cast<unsigned long long>(state.estimatedVRAMUsageInBytes));
    ReleaseSRWLockExclusive(&rr20pOptionsLock);
}
