#pragma once
// Recurring native albedo replay. CPU draw data lives only through its join.
// Targets resize only after native CPU join and post-Present GPU retirement.
enum class RRDiffuseStage {Idle,Recording,Joined,Stopped};
struct RRDiffuseReplay {
    control_rr_albedo::PreparedReplay replay;
    control_rr_albedo::PreparedReplay characterReplay;
    RRAlbedoNativeTarget targets[2]{};
    RRAlbedoNativeTarget characterTarget{};
    std::uintptr_t renderer=0,manager=0;
    void* primaryView=nullptr;
    ID3D12CommandQueue* queue=nullptr; // owned reference
    ID3D12Fence* fence=nullptr;
    DWORD recordingThread=0;
    unsigned long long lastUsePresent=0,signal=0,retireFence=0;
    bool resizePending=false;
    unsigned long long frame=0,prepared=0,joined=0;
    UINT width=0,height=0,targetCount=0,accepted=0,rejected=0;
    std::atomic<long long> drawTicks{0};
    std::atomic<long long> characterDrawTicks{0};
    std::atomic<UINT> frameSkips{0},rangeSkips{0},viewSkips{0},drawCalls{0};
    std::atomic<UINT> batches{0};
    std::atomic<UINT> characterMaskBatches{0};
    std::atomic<bool> fault{false};
    // Published after the per-frame shader-contract filter. Evaluation/compact
    // diagnostics can identify when audited character materials are actually
    // present without touching the replay vectors from another callback.
    std::atomic<unsigned long long> familyFrame{0};
    std::atomic<UINT> familyCounts[7]{};
    std::atomic<UINT> characterBatches{0};
    std::atomic<unsigned long long> characterInstances{0};
    std::atomic<unsigned long long> characterFingerprint{0};
};
static RRDiffuseReplay* rrDiffuse=nullptr;
static std::atomic<RRDiffuseStage> rrDiffuseStage{RRDiffuseStage::Idle};
static thread_local void* rrDiffuseJoinView=nullptr;
static std::atomic<unsigned long long> rrSkinCharacterLogSignature{~0ull};
static std::atomic<unsigned long long> rrSkinCharacterLogCount{0};
[[maybe_unused]] static UINT RRDiffuseFamilyCount(unsigned long long frame,UINT family) noexcept {
    auto* c=rrDiffuse;if(!c||family>=7||c->familyFrame.load(std::memory_order_acquire)!=frame)return 0;
    return c->familyCounts[family].load(std::memory_order_relaxed);
}
struct RRDiffuseCharacterSummary {
    UINT batches=0;
    unsigned long long instances=0;
    unsigned long long fingerprint=0;
};
[[maybe_unused]] static RRDiffuseCharacterSummary RRDiffuseCharacterInfo(unsigned long long frame) noexcept {
    RRDiffuseCharacterSummary out{};
    auto* c=rrDiffuse;if(!c||c->familyFrame.load(std::memory_order_acquire)!=frame)return out;
    out.batches=c->characterBatches.load(std::memory_order_relaxed);
    out.instances=c->characterInstances.load(std::memory_order_relaxed);
    out.fingerprint=c->characterFingerprint.load(std::memory_order_relaxed);
    return out;
}
static void RRDiffuseStop(const char* reason) noexcept {
    rrDiffuseStage.store(RRDiffuseStage::Stopped,std::memory_order_release);
    Log("RR_G12_DIFFUSE_STOP reason=%s native_targets_retained=1 rr_eval=disabled",reason);
}
static void RRDiffuseDrawFault(RRDiffuseReplay* c,const char* reason,int first,int end,DWORD fault,DWORD restoreFault=0) noexcept {
    if(!c->fault.exchange(true)) Log("RR_G12_DIFFUSE_DRAW_FAULT frame=%llu reason=%s first=%d end=%d exception=0x%08lX restore_exception=0x%08lX rr_eval=disabled",c->frame,reason,first,end,fault,restoreFault);
}
#include "rr_diffuse_resize.h"
static void RRDiffusePrepare(void* callback,void* table) noexcept {
    if(control_rr::RRGBufferGuideMode) return;
    // No replay preparation, worker recording, target clears or material scans while FULL RR is off.
    if(!control_rr::RRUserRuntimeWorkRequested()) return;
    RRAlbedoLastError error;
    // Above the diagnostic pixel bound, use the validated native extent without
    // requiring multi-GiB diagnostic exports first. Never bypass a failed or
    // partially recorded diagnostic; shader/frame/view checks below still run.
    const auto highResolution=rrGuideHighResolutionExtent.load(std::memory_order_acquire);
    const auto diagnosticStage=rrAlbedoStage.load(std::memory_order_acquire);
    // r20t is a lean performance build: the recurring replay contract is already
    // validated, so it no longer waits for or creates the one-shot G3 diagnostic capture.
    (void)highResolution;(void)diagnosticStage;
    if(rrDiffuseStage.load(std::memory_order_acquire)==RRDiffuseStage::Stopped) return;
    unsigned long long frame=0;DWORD fault=0;const char* reason="prepare_unavailable";
    control_rr_albedo::PreparationState preparation{};CameraSnapshot camera{};RRGuideInputSnapshot context{};
    if(!ReadEngineFrameSafe(&frame,&fault) || !frame ||
       !control_rr_albedo::ReadPreparationState(reinterpret_cast<std::uintptr_t>(verifiedRenderer),callback,table,&preparation,&reason,&fault) ||
       !ReadCamera(&camera,&fault,&reason) || camera.engineFrame!=frame ||
       reinterpret_cast<std::uintptr_t>(camera.renderer)!=preparation.renderer ||
       !RRGuideReadRendererContext(&context,&reason) || context.engineFrame!=frame) return;
    // Native feature reset publishes this frame's render extent before jobs
    // are prepared. A diagnostic snapshot may belong to an older resolution.
    if(rrDiffuseExtent.frame!=frame||rrDiffuseExtent.thread!=GetCurrentThreadId()||
       rrDiffuseExtent.width<64||rrDiffuseExtent.height<64||
       !control_rr::RenderExtent(rrDiffuseExtent.width,rrDiffuseExtent.height))return;
    if(rrDiffuse && frame<=rrDiffuse->frame) return;
    if(rrDiffuseStage.load(std::memory_order_acquire)==RRDiffuseStage::Recording) {RRDiffuseStop("previous_frame_not_joined");return;}

    const long long start=RRGuideQpc();
    try {
        if(!rrDiffuse) {
            rrDiffuse=new(std::nothrow) RRDiffuseReplay;if(!rrDiffuse) return;
            rrDiffuse->width=rrDiffuseExtent.width;rrDiffuse->height=rrDiffuseExtent.height;
            rrDiffuse->queue=context.queue;context.queue->AddRef();
            rrDiffuse->recordingThread=GetCurrentThreadId();
            ID3D12Device* device=nullptr;
            HRESULT hr=context.queue->GetDevice(IID_PPV_ARGS(&device));
            if(SUCCEEDED(hr)&&device)hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&rrDiffuse->fence));
            RRGuideRelease(device);
            if(FAILED(hr)||!rrDiffuse->fence){RRDiffuseStop("resize_fence_creation_failed");return;}
        }
        auto* c=rrDiffuse;
        if(c->queue!=context.queue) {RRDiffuseStop("native_queue_changed");return;}
        if(!RRDiffuseResize(c,rrDiffuseExtent.width,rrDiffuseExtent.height,&fault))return;
        rrDiffuseStage.store(RRDiffuseStage::Idle,std::memory_order_release);
        c->frame=frame;c->renderer=preparation.renderer;c->manager=preparation.manager;c->primaryView=preparation.primaryView;
        c->replay={};c->characterReplay={};c->drawTicks.store(0);c->characterDrawTicks.store(0);c->batches.store(0);c->characterMaskBatches.store(0);c->fault.store(false);
        c->familyFrame.store(0,std::memory_order_release);
        for(auto& count:c->familyCounts)count.store(0,std::memory_order_relaxed);
        c->characterBatches.store(0,std::memory_order_relaxed);
        c->characterInstances.store(0,std::memory_order_relaxed);
        c->characterFingerprint.store(0,std::memory_order_relaxed);
        c->frameSkips=0;c->rangeSkips=0;c->viewSkips=0;c->drawCalls=0;
        RRAlbedoShader::Session shaders; // fresh native shader validation each frame
        const control_rr_albedo::Access access{&RRAlbedoRead,&RRAlbedoGetShader,nullptr,&shaders,&RRAlbedoValidateEyeCandidate};
        if(!control_rr_albedo::prepare(access,c->manager,c->replay,true)) {rrDiffuseStage.store(RRDiffuseStage::Idle);return;}
        std::uintptr_t source=0;memcpy(&source,c->replay.manager.data()+0x30,sizeof(source));
        size_t kept=0;UINT targets=0;
        for(size_t i=0;i<c->replay.batches.size();++i) {
            control_rr_albedo::OpaqueBatch original{};RRAlbedoShader::Result result{};
            if(!RRAlbedoRead(source+size_t(c->replay.sourceIndices[i])*sizeof(original),&original,sizeof(original)) ||
               !shaders.ValidatePair(original.shader,c->replay.batches[i].shader,result,&reason) ||
               result.targetCount<1 || result.targetCount>2 || (targets && targets!=result.targetCount)) continue;
            targets=result.targetCount;c->replay.batches[kept]=c->replay.batches[i];
            c->replay.sourceIndices[kept]=c->replay.sourceIndices[i];c->replay.familyKinds[kept]=c->replay.familyKinds[i];++kept;
        }
        c->replay.batches.resize(kept);c->replay.sourceIndices.resize(kept);c->replay.familyKinds.resize(kept);
        c->accepted=static_cast<UINT>(kept);c->rejected=c->replay.coverage.sourceBatches-c->accepted;
        c->replay.coverage.replayBatches=c->accepted;c->replay.coverage.rejectedBatches=c->rejected;c->replay.coverage.complete=c->rejected==0;
        UINT familyCounts[7]{};
        UINT characterBatches=0;
        unsigned long long characterInstances=0;
        unsigned long long characterFingerprint=1469598103934665603ull;
        auto characterMix=[&](unsigned long long value) noexcept { characterFingerprint^=value; characterFingerprint*=1099511628211ull; };
        for(size_t i=0;i<c->replay.familyKinds.size();++i){
            const auto kind=c->replay.familyKinds[i];
            if(kind<7)++familyCounts[kind];
            if(kind==2){
                ++characterBatches;
                const auto& batch=c->replay.batches[i];
                if(batch.instanceCount>0)characterInstances+=static_cast<unsigned long long>(batch.instanceCount);
                characterMix(static_cast<unsigned long long>(c->replay.sourceIndices[i]));
                characterMix(static_cast<unsigned long long>(batch.material));
                characterMix(static_cast<unsigned long long>(static_cast<unsigned int>(batch.instanceCount)));
                characterMix(static_cast<unsigned long long>(batch.instanceBufferIndex));
            }
        }
        if(!characterBatches)characterFingerprint=0;
        for(UINT i=0;i<7;++i)c->familyCounts[i].store(familyCounts[i],std::memory_order_relaxed);
        c->characterBatches.store(characterBatches,std::memory_order_relaxed);
        c->characterInstances.store(characterInstances,std::memory_order_relaxed);
        c->characterFingerprint.store(characterFingerprint,std::memory_order_relaxed);
        c->familyFrame.store(frame,std::memory_order_release);
        if(characterBatches){
            const unsigned long long signature=(static_cast<unsigned long long>(characterBatches)<<48) ^
                (characterInstances<<16) ^ characterFingerprint;
            rrSkinCharacterLogSignature.store(signature,std::memory_order_relaxed);
            const auto count=rrSkinCharacterLogCount.fetch_add(1,std::memory_order_relaxed)+1;
            if(count<=8||(count&(count-1))==0||frame%240ull==0)
                Log("RR_SKIN_CHARACTER_BATCHES frame=%llu batches=%u instances=%llu fingerprint=0x%016llX source=native_opaque_character_replay skin_mask_candidate=1 log_policy=first8_powers_periodic240",
                    frame,characterBatches,characterInstances,characterFingerprint);
        }
        if(!kept) {rrDiffuseStage.store(RRDiffuseStage::Idle);return;}
        // Build a second immutable replay containing only audited Character-family
        // draws. Its target is cleared to zero each frame; the selected albedo
        // shaders write alpha for covered pixels, giving us an exact-build
        // screen-space character coverage candidate without touching Control's
        // original render targets. This is used only by the optional skin test.
        if(control_rr::RRUserSkinResponsivity()){
            c->characterReplay.manager=c->replay.manager;
            c->characterReplay.coverage={};
            c->characterReplay.rejectionAudit={};
            c->characterReplay.sourceFirst=c->replay.sourceFirst;
            c->characterReplay.sourceEnd=c->replay.sourceEnd;
            c->characterReplay.batches.reserve(characterBatches);
            c->characterReplay.sourceIndices.reserve(characterBatches);
            c->characterReplay.familyKinds.reserve(characterBatches);
            for(size_t i=0;i<c->replay.familyKinds.size();++i){
                if(c->replay.familyKinds[i]!=static_cast<std::uint8_t>(control_rr_albedo::MaterialAdmission::Character))continue;
                c->characterReplay.batches.push_back(c->replay.batches[i]);
                c->characterReplay.sourceIndices.push_back(c->replay.sourceIndices[i]);
                c->characterReplay.familyKinds.push_back(c->replay.familyKinds[i]);
            }
            c->characterReplay.coverage.sourceBatches=c->replay.coverage.sourceBatches;
            c->characterReplay.coverage.replayBatches=static_cast<std::uint32_t>(c->characterReplay.batches.size());
            c->characterReplay.coverage.rejectedBatches=c->characterReplay.coverage.sourceBatches-c->characterReplay.coverage.replayBatches;
            c->characterReplay.coverage.complete=false;
        }
        c->targetCount=targets;
        c->lastUsePresent=context.presentToken; // mark before the first native command
        for(UINT i=0;i<targets;++i) {
            if(!c->targets[i].nativeTexture && !RRAlbedoNativeCreate(&rrAlbedoAPI,c->width,c->height,&c->targets[i],&reason,&fault)) {RRDiffuseStop("target_creation_failed");return;}
            RRGuideInputSnapshot deviceCheck=context;deviceCheck.gbuffer1.resource=c->targets[i].resource;deviceCheck.gbuffer2.resource=c->targets[i].resource;
            if(!RRGuideResourcesShareDevice(&deviceCheck,nullptr,nullptr)) {RRDiffuseStop("target_device_mismatch");return;}
            if(!RRAlbedoNativePrepareClear(&rrAlbedoAPI,&c->targets[i],&fault)) {RRDiffuseStop("target_clear_failed");return;}
        }
        if(characterBatches && control_rr::RRUserSkinResponsivity()){
            if(!c->characterTarget.nativeTexture && !RRAlbedoNativeCreate(&rrAlbedoAPI,c->width,c->height,&c->characterTarget,&reason,&fault)){
                Log("RR_SKIN_MASK_DISABLED frame=%llu reason=target_creation_failed fault=0x%08lX",frame,fault);
            } else if(c->characterTarget.nativeTexture && !RRAlbedoNativePrepareClear(&rrAlbedoAPI,&c->characterTarget,&fault)){
                Log("RR_SKIN_MASK_DISABLED frame=%llu reason=target_clear_failed fault=0x%08lX",frame,fault);
            }
        }
        rrDiffuseStage.store(RRDiffuseStage::Recording,std::memory_order_release);
        RRPerfReplayPrepared(frame,RRGuideElapsedMs(start));
        const auto n=++c->prepared;
        if(n<=4 || (n&(n-1))==0) Log("RR_G12_DIFFUSE_PREPARED count=%llu frame=%llu accepted=%u rejected=%u targets=%u width=%u height=%u prepare_ms=%.3f coverage_proven=0 rr_eval=disabled standard=%u character=%u cloth=%u foliage=%u hair=%u eye=%u",n,frame,c->accepted,c->rejected,targets,c->width,c->height,RRGuideElapsedMs(start),familyCounts[1],familyCounts[2],familyCounts[3],familyCounts[4],familyCounts[5],familyCounts[6]);
    } catch(...) {RRDiffuseStop("preparation_exception");}
}
#include "rr_diffuse_view.h"
#include "rr_diffuse_draw.h"
#include "rr_diffuse_join.h"
