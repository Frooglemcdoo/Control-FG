#pragma once
static void RRLiveAfterPresent(unsigned long long present) noexcept {
    const DWORD saved=GetLastError();
    if(!TryAcquireSRWLockExclusive(&rrLiveLock)) {SetLastError(saved);return;}
    __try {
        auto* c=rrLive;
        if(c && c->prepared && !c->stopped) {
            const auto completed=c->fence->GetCompletedValue();
            if(!c->policy.Collect(c->Key(),completed)) {c->stopped=true;Log("RR_G12_LIVE_STOP reason=fence_completion_invalid resources_retained=1 stage=guide_retirement");}
            if(rrLiveGuideStatsEnabled && !c->stopped && c->capture && c->capture->policy.Retire(completed)) {
                if(!QueueUserWorkItem(&RRLiveCaptureExport,c->capture,WT_EXECUTEDEFAULT)) Log("RR_G12_LIVE_CAPTURE_FAILED reason=export_queue stage=guide_retirement");
            }
            if(!c->stopped) for(UINT i=0;i<3;++i) {
                const auto state=c->policy.Inspect()[i];auto& s=c->slots[i];
                if(state.state==control_rr::SlotState::Free && s.sources[0]) {RRLiveReleaseSources(s);++c->retired;}
                if(state.state==control_rr::SlotState::Ready && s.present<present) {
                    const auto value=++c->signal;
                    const HRESULT hr=c->queue->Signal(c->fence,value);
                    if(FAILED(hr) || !c->policy.Submit({i,state.serial},value)) {c->stopped=true;Log("RR_G12_LIVE_STOP reason=fence_signal_failed resources_retained=1 stage=guide_retirement");break;}
                    if(rrLiveGuideStatsEnabled&&c->capture)c->capture->policy.Submit(i,state.serial,value);
                }
            }
            if(present%120==0) Log("RR_G12_LIVE_STATUS present=%llu recorded=%llu retired=%llu skipped=%llu fence=%llu completed=%llu guide_frames=%llu stopped=%u stage=guide_retirement architecture=gbuffer_material_envbrdf",present,c->recorded,c->retired,c->skipped,c->signal,completed,c->guideFrames,unsigned(c->stopped));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {if(rrLive)rrLive->stopped=true;Log("RR_G12_LIVE_STOP reason=retirement_exception resources_retained=1 stage=guide_retirement");}
    ReleaseSRWLockExclusive(&rrLiveLock);SetLastError(saved);
}
