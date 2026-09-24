#pragma once
// Select the primary view before validating its borrowed render targets.
static void RRDiffuseDraw(void* manager,int first,int end) noexcept {
    if(rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Recording) return;
    RRAlbedoLastError error;auto* c=rrDiffuse;
    if(!c || c->manager!=reinterpret_cast<std::uintptr_t>(manager) || c->fault.load()) return;
    unsigned long long frame=0;DWORD fault=0;
    ++c->drawCalls;
    if(!ReadEngineFrameSafe(&frame,&fault) || frame!=c->frame) {++c->frameSkips;return;}
    control_rr_albedo::DrawRange range{};
    if(!control_rr_albedo::prepareDrawRange(c->replay,first,end,range)) {++c->rangeSkips;return;}
    RRDiffuseViewSnapshot selected{};
    if(!RRDiffuseReadView(&selected,&fault)) {RRDiffuseDrawFault(c,"draw_view_read_failed",first,end,fault);return;}
    if(!selected.view || selected.view!=c->primaryView) {++c->viewSkips;return;}
    RRAlbedoNativeBindings saved{};const char* reason="bindings";
    if(!RRAlbedoNativeReadBindings(&rrAlbedoAPI,&saved,&reason,&fault)) {
        RRDiffuseDepthEvidence(c->frame,first,end,&saved);
        RRDiffuseDrawFault(c,reason,first,end,fault);return;
    }
    if(saved.staticTlsBlock!=selected.tls || saved.commandContext!=selected.context ||
       saved.deviceState!=selected.state || saved.workerContext!=selected.worker) {
        RRDiffuseDrawFault(c,"view_binding_context_changed",first,end,0);return;
    }
    if(saved.width!=c->width || saved.height!=c->height) {
        Log("RR_G12_DIFFUSE_EXTENT_MISMATCH frame=%llu expected_width=%u expected_height=%u actual_width=%u actual_height=%u",c->frame,c->width,c->height,saved.width,saved.height);
        RRDiffuseDrawFault(c,"draw_extent_mismatch",first,end,fault);return;
    }
    const auto start=RRGuideQpc();
    RRAlbedoNativeTarget* targets[2]{&c->targets[0],&c->targets[1]};bool changed=false;
    const bool bound=RRAlbedoNativeBind(&rrAlbedoAPI,&saved,targets,c->targetCount,&changed,&fault);
    const bool drawn=bound && RRAlbedoDrawSafe(range.managerView(),&fault);
    DWORD restoreFault=0;const bool restored=!changed || RRAlbedoNativeRestore(&rrAlbedoAPI,&saved,&restoreFault);
    c->drawTicks.fetch_add(RRGuideQpc()-start);
    if(!drawn || !restored) {RRDiffuseDrawFault(c,!restored?"restore_failed":!bound?"bind_failed":"draw_failed",first,end,fault,restoreFault);return;}
    c->batches.fetch_add(range.coverage.replayBatches);

    // Optional skin experiment: replay only audited Character-family batches
    // into a separate zero-cleared RGBA16F target. This leaves Control's native
    // targets and the production diffuse replay unchanged. The alpha channel is
    // converted to a one-channel mask immediately before RR evaluation.
    if(control_rr::RRUserSkinResponsivity() && c->characterTarget.nativeTexture && !c->characterReplay.batches.empty()) {
        control_rr_albedo::DrawRange characterRange{};
        if(control_rr_albedo::prepareDrawRange(c->characterReplay,first,end,characterRange)) {
            RRAlbedoNativeTarget* skinTargets[1]{&c->characterTarget};bool skinChanged=false;DWORD skinFault=0;
            const auto skinStart=RRGuideQpc();
            const bool skinBound=RRAlbedoNativeBind(&rrAlbedoAPI,&saved,skinTargets,1,&skinChanged,&skinFault);
            const bool skinDrawn=skinBound && RRAlbedoDrawSafe(characterRange.managerView(),&skinFault);
            DWORD skinRestoreFault=0;const bool skinRestored=!skinChanged || RRAlbedoNativeRestore(&rrAlbedoAPI,&saved,&skinRestoreFault);
            c->characterDrawTicks.fetch_add(RRGuideQpc()-skinStart);
            if(!skinDrawn || !skinRestored) {
                Log("RR_SKIN_MASK_DRAW_FAILED frame=%llu first=%d end=%d bound=%u drawn=%u restored=%u fault=0x%08lX restore_fault=0x%08lX skin_mode=responsivity",
                    c->frame,first,end,unsigned(skinBound),unsigned(skinDrawn),unsigned(skinRestored),skinFault,skinRestoreFault);
                // Skin protection is optional. Disable this frame's mask without
                // stopping the proven RR diffuse/guide path.
                c->characterMaskBatches.store(0,std::memory_order_relaxed);
            } else {
                c->characterMaskBatches.fetch_add(characterRange.coverage.replayBatches,std::memory_order_relaxed);
            }
        }
    }
}
