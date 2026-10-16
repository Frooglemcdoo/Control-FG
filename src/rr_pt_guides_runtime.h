#pragma once
static void RRPTAcquireGuides(unsigned long long engineFrame,UINT width,UINT height,
    ID3D12CommandQueue* queue,ID3D12GraphicsCommandList* list,RRPTGuideSource& out) noexcept {
    out.owner.reset();out.raw=out.surface=out.guard=0;out.reason="pt_off";
#if defined(CONTROL_PT_SURFACE_REPLACEMENT)
    try {
    using namespace PTGlassRuntime::Primary;
    if(!active.load()||!control_rr::rrPTInputRoute.Fresh(engineFrame))return;
    // Try both locks in the established queue -> PT order; never block native guide locks.
    std::unique_lock<std::recursive_mutex> ordered(PTSceneQueues::queueMutex,std::try_to_lock);
    if(!ordered.owns_lock()){out.reason="queue_busy";return;}
    PTSceneQueues::Scope privateQueues;
    std::unique_lock<std::recursive_mutex> lock(mutex,std::try_to_lock);
    if(!lock.owns_lock()){out.reason="pt_busy";return;}
    out.reason="matching_recorded_producer_unavailable";
    for(const auto& weak:frames){
        const auto f=weak.lock();
        if(!f||f->view.engineFrame!=engineFrame||f->view.width!=width||f->view.height!=height||
           f->queue.Get()!=queue||!f->producerRecorded||f->cancelled||!f->raw||!f->assets||
           !f->assets->reconstruction||!f->assets->reconstruction->freshInput)continue;
        if(f->raw->Texture()->GetDesc().Width!=UINT64(width)*height*64||
           f->output->Texture()->GetDesc().Width!=UINT64(width)*height*32)continue;
        if(!PTSceneQueues::WatchList(list)){out.reason="guide_list_watch";return;}
        auto* recording=RecordingFor(list);
        if(!recording||recording->submitted||recording->guides.size()>=4){out.reason="guide_recording_unavailable";return;}
        auto guide=std::make_shared<Guide>();guide->frame=f;
        // Initially zero. Only ordered submission can enable this consumer.
        if(FAILED(Output::Create(queue,1,1,guide->guard,true,true))){out.reason="guide_guard_allocation";return;}
        recording->guides.push_back(guide);
        out.owner=guide;out.raw=f->raw->Address();out.surface=f->output->Address();out.guard=guide->guard->Address();out.reason="recorded_pending_submission";return;
    }
    }catch(...){out.reason="guide_source_exception";}
#else
    (void)engineFrame;(void)width;(void)height;(void)queue;(void)list;
#endif
}
