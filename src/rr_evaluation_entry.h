#include "rr_clamp_strength_policy.h"
#pragma once
#include "rr_evaluation_tail.h"
#include "rr_user_control.h"
#include "rr_live_frame_input.h"
#include "rr_distance_binding_policy.h"
#include "rr_skin_diagnostic.h"
#include "rr_responsivity_runtime.h"
// Exact-build evaluation gateway installed during initialization. The live
// candidate pass owns its resources; original NGX arguments pass through.
using RREvaluationC = unsigned int (*)(ID3D12GraphicsCommandList*, void*, void*, void*);
static RREvaluationC rrEvaluationOriginal = nullptr;
static RRAlbedoCallPatch rrEvaluationPatches[2]{};
static std::atomic<unsigned long long> rrEvaluationCalls[2]{};
static LONGLONG rrGI25LastEvaluationQpc=0;
static LARGE_INTEGER rrGI25QpcFrequency{};
static float RRGI25FrameTimeMs() noexcept {
 LARGE_INTEGER now{};
 if(rrGI25QpcFrequency.QuadPart<=0)QueryPerformanceFrequency(&rrGI25QpcFrequency);
 QueryPerformanceCounter(&now);
 float ms=16.666667f;
 if(rrGI25LastEvaluationQpc>0&&rrGI25QpcFrequency.QuadPart>0)
  ms=static_cast<float>((double(now.QuadPart-rrGI25LastEvaluationQpc)*1000.0)/double(rrGI25QpcFrequency.QuadPart));
 rrGI25LastEvaluationQpc=now.QuadPart;
 return control_rr_responsivity::ClampFrameTimeMs(ms);
}
#include "rr_evaluation_inputs.h"
static RREvaluationInputs RRReadEvaluationInputs(void* parameters) noexcept {
    RREvaluationInputs in{};
    // POD-only SEH boundary. Never catch or retry a fault in native evaluation.
    __try {
        if (!parameters || !ngxGetResource || !ngxGetFloat || !ngxGetInt) return in;
        if (ngxGetResource(parameters,"Color",&in.color)==1 && in.color) {
            in.valid|=1;const auto desc=in.color->GetDesc();
            if(desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D && desc.Width<=8192 && desc.Width>=64 &&
               desc.Height>=64 && desc.Height<=8192 && desc.DepthOrArraySize==1 && desc.MipLevels==1 && desc.SampleDesc.Count==1) {
                in.width=static_cast<unsigned int>(desc.Width);in.height=desc.Height;
            }
        }
        if (ngxGetResource(parameters,"Output",&in.output)==1 && in.output) in.valid|=2;
        if (ngxGetResource(parameters,"Depth",&in.depth)==1 && in.depth) in.valid|=4;
        if (ngxGetResource(parameters,"MotionVectors",&in.motion)==1 && in.motion) in.valid|=8;
        if (ngxGetFloat(parameters,"Jitter.Offset.X",&in.jitterX)==1) in.valid|=16;
        if (ngxGetFloat(parameters,"Jitter.Offset.Y",&in.jitterY)==1) in.valid|=32;
        if (ngxGetInt(parameters,"Reset",&in.reset)==1) in.valid|=64;
        ReadEngineFrameSafe(&in.frame,&in.fault);
    } __except(EXCEPTION_EXECUTE_HANDLER) { in.fault=GetExceptionCode(); }
    return in;
}
static unsigned int RREvaluationEntry(unsigned int branch, ID3D12GraphicsCommandList* list,
    void* feature, void* parameters, void* callback) {
    const DWORD incomingError=GetLastError();
    const auto perfEntry=RRPerfClock();
    const auto call=rrEvaluationCalls[branch].fetch_add(1,std::memory_order_relaxed)+1;
    const auto in=RRReadEvaluationInputs(parameters);
    // The gateway does not own these borrowed resources. Nothing escapes this
    // invocation; the separate live pass acquires its own source references.
    const control_rr::LiveFrameInput frameInput{in.frame,in.jitterX,in.jitterY,in.reset,in.valid,in.fault,in.width,in.height};
    const bool fullWork=rrNativeFrameEnabled&&control_rr::RRUserRuntimeWorkRequested();
    if(fullWork) RRLiveBeforeEvaluation(list,frameInput);
    const bool partial=rrNativeFrameEnabled&&RRNativePartialActive();
    // Compile-time retained references keep the old skin experiment available for source comparison,
    // but r20t never executes it.
    if(false&&rrNativeFrameEnabled)RRNativeObserveSkinInputs(parameters,in.frame,"r20t_disabled",in.width,in.height);
    // D1: optional, owned, same-frame distance; never reuses a prior frame.
    RRNativeGuideBindings bindings{};bool rrAttempt=false;
    ID3D12Resource* hitDistance=nullptr;bool distanceReset=false;bool clampReset=false;bool responsivityReset=false;
    float rrFrameTimeMs=16.666667f;int rrResponsivityBias=0;
    static control_rr_clamp::History clampHistory;
    static control_rr::DistanceBindingHistory distanceHistory;
    static std::atomic<int> responsivityHistory{1001};
    void* evaluationFeature=feature;bool presetReset=false;unsigned activePreset=0;
    if(rrNativeFrameEnabled&&branch==1&&!partial){
        if(!RRNativeResolveEvaluationPreset(list,feature,parameters,&evaluationFeature,&presetReset,&activePreset)){
            rrNativeFrame.Fail();control_rr::RRUserPublish(control_rr::RRUserStatus::Stopped);
            Log("RR_FRAME_STOP frame=%llu reason=preset_feature_resolution requested=%s requested_value=%u control_feature=%p native_evaluation_called=0",in.frame,control_rr::RRUserPresetLabel(),control_rr::RRUserPresetValue(),feature);
            SetLastError(incomingError);return 0xBAD00001u;
        }
        const bool guides=in.valid==127&&!in.fault&&RRNativeTakeGuides(in,bindings);
        rrFrameTimeMs=RRGI25FrameTimeMs();
        rrResponsivityBias=activePreset==control_rr::RRPresetF?control_rr::RRUserResponsivityBias():0;
        if(guides&&control_rr_responsivity::Enabled(activePreset==control_rr::RRPresetF,rrResponsivityBias))
            bindings.responsivity=RRResponsivityBeforeEvaluation(list,in.width,in.height,in.frame,rrResponsivityBias);
        const int previousResponsivity=responsivityHistory.exchange(rrResponsivityBias,std::memory_order_acq_rel);
        responsivityReset=previousResponsivity!=rrResponsivityBias;
        if(responsivityReset)Log("RR_GI25_RESPONSIVITY_RESET frame=%llu previous=%d current=%d bound=%u",in.frame,previousResponsivity,rrResponsivityBias,unsigned(bindings.responsivity!=nullptr));
        const bool lighting=RRNativeLightingReady(in.frame);
        const bool beginEvaluation=rrNativeFrame.BeginEvaluation(in.frame,guides,lighting);
        if(control_rr::DistanceEligible(beginEvaluation,bindings.projectionValid,activePreset,control_rr::RRPresetF))
            hitDistance=RRDistanceBeforeEvaluation(list,in.depth,in.frame,lighting);
        distanceReset=distanceHistory.Update(hitDistance!=nullptr);
        clampReset=clampHistory.Update(control_rr_clamp::effective.load(std::memory_order_acquire));
        if(clampReset)Log("RR_CLAMP_CS3_HISTORY_RESET frame=%llu effective=%u",in.frame,control_rr_clamp::effective.load());
        const bool frameTimeReady=RRNativeSetFrameTime(parameters,rrFrameTimeMs);
        if(!beginEvaluation||!frameTimeReady||!RRNativeSetGuides(parameters,&bindings,hitDistance,rrNativeFrame.Reset()||presetReset||distanceReset||clampReset||responsivityReset)){
            rrNativeFrame.Fail();control_rr::RRUserPublish(control_rr::RRUserStatus::Stopped);
            Log("RR_FRAME_STOP frame=%llu reason=pre_evaluation_contract hit_distance_required=0 guides=%u lighting=%u frame_time_ready=%u native_evaluation_called=0",in.frame,unsigned(guides),unsigned(lighting),unsigned(frameTimeReady));
            SetLastError(incomingError);return 0xBAD00001u;
        }
        if(call<=4||(call%240)==0||responsivityReset)
            Log("RR_GI25_TEMPORAL_INPUTS frame=%llu preset=%u frame_time_ms=%.3f responsivity_bias=%d responsivity_value=%.3f responsivity_bound=%u reset=%u",
             in.frame,activePreset,double(rrFrameTimeMs),rrResponsivityBias,double(control_rr_responsivity::MaskValue(rrResponsivityBias)),unsigned(bindings.responsivity!=nullptr),unsigned(responsivityReset));
        RRInputCaptureBeforeEvaluation(list,in.frame,activePreset,bindings.normal,bindings.specular,bindings.diffuse,
            hitDistance,hitDistance?rrDistanceLastStatus:nullptr,bindings.viewToClip,bindings.projectionValid);
        rrAttempt=true;
        if(call<=4||(call%240)==0||distanceReset)
            Log("RR_F_DISTANCE_D1 frame=%llu preset=%u bound=%u resource=%p history_reset=%u projection_valid=%u",in.frame,activePreset,unsigned(hitDistance!=nullptr),hitDistance,unsigned(distanceReset),unsigned(bindings.projectionValid));
    }
    if(rrNativeFrameEnabled&&branch==0&&rrNativeFrame.Selected()){
        rrNativeFrame.Fail();Log("RR_FRAME_STOP frame=%llu reason=unexpected_sr_branch native_evaluation_called=0",in.frame);
        SetLastError(incomingError);return 0xBAD00001u;
    }
    if(partial&&branch==1){
        Log("RR_PARTIAL_EVALUATION_FORWARD frame=%llu branch=1 feature=%p guides_overridden=0 distance_overridden=0 feature13_request=0 partial=1",in.frame,feature);
    }
    const double perfGuidesCpu=RRPerfElapsed(perfEntry);
    RRGuideInputSnapshot perfContext{};const char* perfReason=nullptr;RRPerfTicket perf{};
    if(RRGuideReadRendererContext(&perfContext,&perfReason)&&perfContext.commandList==list&&perfContext.engineFrame==in.frame)
        perf=RRPerfBegin(perfContext,RRPerfStage::NativeEvaluation);
    const auto perfNativeStart=RRPerfClock();
    SetLastError(incomingError);
    const unsigned int result=rrEvaluationOriginal(list,evaluationFeature,parameters,callback);
    const DWORD nativeError=GetLastError();
    const double perfNativeCpu=RRPerfElapsed(perfNativeStart);
    RRPerfEnd(perf);
    RRPerfEvaluation(in.frame,rrAttempt,(result&0xFFF00000u)!=0xBAD00000u,
        in.reset!=0||(rrAttempt&&(rrNativeFrame.Reset()||presetReset||distanceReset||clampReset||responsivityReset)),perfEntry,perfGuidesCpu,perfNativeCpu,
        GetFGUserMultiplier(),IsHdr10BridgeActive()?1u:0u);
    if(partial&&branch==1){
        const bool success=(result&0xFFF00000u)!=0xBAD00000u;
        if(call<=4||(call%240)==0||!success)
            Log("RR_PARTIAL_EVALUATED frame=%llu result=0x%08X success=%u feature=%p untouched_native_parameters=1 partial=1",in.frame,result,unsigned(success),feature);
    }
    if(rrAttempt){
        const bool success=(result&0xFFF00000u)!=0xBAD00000u;
        rrNativeFrame.EvaluationResult(success);
        control_rr::RRUserPublish(success?control_rr::RRUserStatus::Active:control_rr::RRUserStatus::Stopped);
        if(call<=4||(call%240)==0||!success||rrNativeFrame.Reset()||presetReset||distanceReset||clampReset||responsivityReset)Log("RR_NATIVE_EVALUATED frame=%llu result=0x%08X success=%u diffuse=%p specular=%p normal=%p hit_distance=D1_optional specular_mvec=cleared reflection_mvec=cleared matrix_mode=preset_dependent_projection_p1 responsivity=%p responsivity_bias=%d frame_time_ms=%.3f replaces_sr=1 reset=%u preset_reset=%u preset_requested=%s preset_value=%u active_preset=%u control_feature=%p evaluation_feature=%p native_create_confirmed=%u native_create_generation=%llu",in.frame,result,unsigned(success),bindings.diffuse,bindings.specular,bindings.normal,bindings.responsivity,rrResponsivityBias,double(rrFrameTimeMs),unsigned(rrNativeFrame.Reset()||presetReset||distanceReset||clampReset||responsivityReset),unsigned(presetReset),control_rr::RRUserPresetLabel(),control_rr::RRUserPresetValue(),activePreset,feature,evaluationFeature,control_rr::RRUserConfirmedPresetValue(),control_rr::RRUserPresetCreateGeneration());
    } else if(rrNativeFrameEnabled&&branch==0){
        if(rrNativeFrame.Selected()){rrNativeFrame.Fail();Log("RR_FRAME_STOP frame=%llu reason=unexpected_sr_branch",in.frame);}
        else {
            const bool recovering=rrNativeFrame.Recovering();
            const bool complete=rrNativeFrame.CompleteSR((result&0xFFF00000u)!=0xBAD00000u,
                in.valid==127&&!in.fault&&in.frame==rrNativeFrame.Key().frame);
            if(recovering){
                Log("RR_FRAME_RECOVERY_SR frame=%llu success=%u stopped=%u next_rr_reset=%u reason=%s",in.frame,unsigned(complete),unsigned(rrNativeFrame.Stopped()),unsigned(complete&&!rrNativeFrame.Stopped()),rrNativeFrame.StopReason());
                control_rr::RRUserFrameStatus(control_rr::RRUserRequested(),false,rrNativeFrame.Stopped(),rrNativeFrame.Recovering());
            }
        }
    }
    if(call<=4 || (call&(call-1))==0 || (result&0xFFF00000u)==0xBAD00000u)
        Log("RR_G12_EVALUATION_ENTRY branch=%u call=%llu frame=%llu list=%p control_feature=%p evaluation_feature=%p parameters=%p callback=%p valid=0x%X color=%p output=%p depth=%p motion=%p jitter_x=%.9g jitter_y=%.9g reset=%d preset_reset=%u read_fault=0x%08lX native_result=0x%08X native_forwarded=1 rr_eval=%s",
            branch,call,in.frame,list,feature,evaluationFeature,parameters,callback,in.valid,in.color,in.output,
            in.depth,in.motion,double(in.jitterX),double(in.jitterY),in.reset,unsigned(presetReset),in.fault,result,rrAttempt?"native_full":(partial?"partial":"sr"));
    SetLastError(nativeError);
    return result;
}
static unsigned int RREvaluationEntry0(ID3D12GraphicsCommandList* list,void* feature,void* params,void* callback) {
    return RREvaluationEntry(0,list,feature,params,callback);
}
static unsigned int RREvaluationEntry1(ID3D12GraphicsCommandList* list,void* feature,void* params,void* callback) {
    return RREvaluationEntry(1,list,feature,params,callback);
}
static bool RRInstallEvaluationEntry(HMODULE d3d) noexcept {
    if(!d3d || d3d!=verifiedD3d) return false;
    auto* base=reinterpret_cast<unsigned char*>(d3d);
    auto* target=base+control_rr::EvaluationCExport;
    if(reinterpret_cast<void*>(GetProcAddress(d3d,"NVSDK_NGX_D3D12_EvaluateFeature_C"))!=target) return false;
    // Validate BOTH tails before allocating or writing either site.
    for(unsigned int i=0;i<2;++i) {
        control_rr::Jump5 expected{};
        auto* site=base+control_rr::EvaluationTailSites[i];
        if(!control_rr::RelativeJump(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(target),expected) ||
            memcmp(site,expected.data(),5)) return false;
    }
    rrEvaluationOriginal=reinterpret_cast<RREvaluationC>(target);
    RREvaluationC hooks[2]{&RREvaluationEntry0,&RREvaluationEntry1};
    for(unsigned int i=0;i<2;++i) {
        auto& p=rrEvaluationPatches[i];p.site=base+control_rr::EvaluationTailSites[i];
        memcpy(p.original,p.site,5);p.relay=AllocateExecutableRelayNear(p.site);
        if(!p.relay) return false;
        control_rr::Tail14 stub{};control_rr::Jump5 old{},next{};
        memcpy(old.data(),p.original,5);
        if(!control_rr::AbsoluteTail(reinterpret_cast<std::uintptr_t>(hooks[i]),stub) ||
            !control_rr::CheckedTailPatch(old,reinterpret_cast<std::uintptr_t>(p.site),
                reinterpret_cast<std::uintptr_t>(target),reinterpret_cast<std::uintptr_t>(p.relay),next)) return false;
        memcpy(p.relay,stub.data(),stub.size());memcpy(p.replacement,next.data(),5);
        DWORD prior=0;
        if(!VirtualProtect(p.relay,4096,PAGE_EXECUTE_READ,&prior) ||
            !FlushInstructionCache(GetCurrentProcess(),p.relay,stub.size())) return false;
    }
    unsigned int applied=0;bool healthy=true;
    while(applied<2) {
        if(!RRAlbedoExchangeCall(&rrEvaluationPatches[applied],true)) { healthy=false;break; }
        const bool okay=rrEvaluationPatches[applied++].writeHealthy;
        if(!okay) { healthy=false;break; }
    }
    if(!healthy || applied!=2) {
        bool rollback=true;
        while(applied) {
            auto& p=rrEvaluationPatches[--applied];
            if(!RRAlbedoExchangeCall(&p,false) || !p.writeHealthy) rollback=false;
        }
        // Relays and target remain valid even on uncertain rollback. Never free
        // executable memory which a surviving jump could still reference.
        Log("RR_G12_EVALUATION_HOOKS_FAILED rollback=%u relays_retained=1 rr_eval=disabled",unsigned(rollback));
        return false;
    }
    Log("RR_G12_EVALUATION_HOOKS_READY tails=2 target=0x52A20 mode=coordinated_native rr_eval=gated");
    return true;
}
