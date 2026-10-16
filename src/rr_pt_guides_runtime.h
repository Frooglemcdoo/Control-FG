#pragma once
static void RRPTAcquireGuides(unsigned long long engineFrame,UINT width,UINT height,
    ID3D12CommandQueue* queue,RRPTGuideSource& out) noexcept {
    out.owner.reset();out.raw=out.surface=0;out.reason="pt_off";
#if defined(CONTROL_PT_SURFACE_REPLACEMENT)
    using namespace PTGlassRuntime::Primary;
    if(!active.load()||!control_rr::rrPTInputRoute.Fresh(engineFrame))return;
    // Called with native guide locks held. Never wait for the PT recording lock.
    std::unique_lock<std::recursive_mutex> lock(mutex,std::try_to_lock);
    if(!lock.owns_lock()){out.reason="pt_busy";return;}
    out.reason="matching_submitted_producer_unavailable";
    for(const auto& weak:frames){
        const auto f=weak.lock();
        if(!f||f->view.engineFrame!=engineFrame||f->view.width!=width||f->view.height!=height||
           f->queue.Get()!=queue||!f->producerSubmitted||!f->armed||f->cancelled||!f->raw||!f->assets||
           !f->assets->reconstruction||!f->assets->reconstruction->freshInput)continue;
        if(f->raw->Texture()->GetDesc().Width!=UINT64(width)*height*64||
           f->output->Texture()->GetDesc().Width!=UINT64(width)*height*32)continue;
        out.owner=f;out.raw=f->raw->Address();out.surface=f->output->Address();out.reason="current_pt_surface";return;
    }
#else
    (void)engineFrame;(void)width;(void)height;(void)queue;
#endif
}
