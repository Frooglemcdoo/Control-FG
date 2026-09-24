#pragma once
static void RRDiffuseJoinComplete(void* callback) noexcept {
    if(rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Recording) return;
    RRAlbedoLastError error;auto* c=rrDiffuse;
    std::uintptr_t renderer=0,manager=0;void* view=nullptr;DWORD fault=0;unsigned long long frame=0;
    if(!c || !RRAlbedoReadCallback(callback,&renderer,&manager) || renderer!=c->renderer || manager!=c->manager ||
        !control_rr_albedo::ReadJoinView(callback,&view,&fault) || view!=c->primaryView) return;
    const bool frameReadable=ReadEngineFrameSafe(&frame,&fault);
    const bool drawFault=c->fault.load();const UINT replayed=c->batches.load();
    const UINT characterExpected=c->characterBatches.load(std::memory_order_relaxed);
    const UINT characterReplayed=c->characterMaskBatches.load(std::memory_order_relaxed);
    const char* failure=!frameReadable?"join_frame_read_failed":frame!=c->frame?"join_frame_mismatch":drawFault?"join_draw_fault":replayed!=c->accepted?"join_batch_count_mismatch":nullptr;
    if(failure) {
        Log("RR_G12_DIFFUSE_JOIN_FAILURE reason=%s expected_frame=%llu actual_frame=%llu frame_readable=%u exception=0x%08lX draw_fault=%u accepted=%u replayed=%u rejected=%u draw_calls=%u frame_skips=%u range_skips=%u view_skips=%u rr_eval=disabled",failure,c->frame,frame,unsigned(frameReadable),fault,unsigned(drawFault),c->accepted,replayed,c->rejected,c->drawCalls.load(),c->frameSkips.load(),c->rangeSkips.load(),c->viewSkips.load());
        RRDiffuseStop(failure);return;
    }
    rrDiffuseStage.store(RRDiffuseStage::Joined,std::memory_order_release);const auto n=++c->joined;
    RRPerfReplayJoined(frame,frequency.QuadPart?1000.0*double(c->drawTicks.load())/double(frequency.QuadPart):0,c->batches.load());
    if(control_rr::RRUserSkinResponsivity() && characterExpected && characterReplayed!=characterExpected) {
        Log("RR_SKIN_MASK_INCOMPLETE frame=%llu expected_batches=%u replayed_batches=%u action=disable_mask_for_frame rr_continues=1",frame,characterExpected,characterReplayed);
    }
    if(n<=4 || (n&(n-1))==0 || (control_rr::RRUserSkinResponsivity()&&frame%240ull==0)) Log("RR_G12_DIFFUSE_JOINED count=%llu frame=%llu replayed=%u rejected=%u replay_cpu_ms_sum=%.3f skin_batches=%u skin_replayed=%u skin_replay_cpu_ms_sum=%.3f coverage_proven=0 rr_eval=disabled",n,frame,c->batches.load(),c->rejected,frequency.QuadPart?1000.0*double(c->drawTicks.load())/double(frequency.QuadPart):-1.0,characterExpected,characterReplayed,frequency.QuadPart?1000.0*double(c->characterDrawTicks.load())/double(frequency.QuadPart):-1.0);
}
static ID3D12Resource* RRDiffuseCurrent(const RRGuideInputSnapshot* in) noexcept {
    if(rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Joined) return nullptr;
    auto* c=rrDiffuse;
    if(!c || c->frame!=in->engineFrame || c->queue!=in->queue || c->renderer!=reinterpret_cast<std::uintptr_t>(in->camera.renderer) ||
       c->width!=in->gbuffer1.desc.Width || c->height!=in->gbuffer1.desc.Height) return nullptr;
    return c->targets[0].resource;
}

static ID3D12Resource* RRDiffuseCharacterCurrent(unsigned long long frame,UINT width,UINT height) noexcept {
    if(!control_rr::RRUserSkinResponsivity() || rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Joined) return nullptr;
    auto* c=rrDiffuse;
    if(!c || c->frame!=frame || c->width!=width || c->height!=height || !c->characterTarget.nativeTexture || !c->characterTarget.resource) return nullptr;
    const UINT expected=c->characterBatches.load(std::memory_order_relaxed);
    const UINT replayed=c->characterMaskBatches.load(std::memory_order_relaxed);
    if(!expected || replayed!=expected) return nullptr;
    if(!RRAlbedoNativeTextureState(c->characterTarget.nativeTexture,D3D12_RESOURCE_STATE_RENDER_TARGET)) return nullptr;
    return c->characterTarget.resource;
}

static void* RRDiffuseBeginJoin(void* callback) noexcept {
    void* previous=rrDiffuseJoinView;DWORD fault=0;
    rrDiffuseJoinView=nullptr;control_rr_albedo::ReadJoinView(callback,&rrDiffuseJoinView,&fault);return previous;
}
static void RRDiffuseEndJoin(void* callback,void* previous) noexcept {
    RRDiffuseJoinComplete(callback);rrDiffuseJoinView=previous;
}

static void RRDiffuseDisableForLiveFailure() noexcept {
    if(rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Stopped) RRDiffuseStop("live_generation_stopped");
}
