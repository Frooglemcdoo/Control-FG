#pragma once
#include "rr_performance_policy.h"
// Primary recording thread only. Queries surround owned command emissions or
// the existing NGX call. Worker replay is measured separately as CPU work.
// Lifetime: exact native Present -> queue Signal -> completed fence -> read/reuse.
// Instrumentation faults disable profiling; they never change RR selection.
enum class RRPerfStage : unsigned { ReflectionCopy, LiveGuides, Distance, NativeEvaluation };
static const char* RRPerfName(RRPerfStage stage) noexcept {
    switch(stage){
    case RRPerfStage::ReflectionCopy:return "reflection_copy";
    case RRPerfStage::LiveGuides:return "live_guides";
    case RRPerfStage::Distance:return "hit_distance";
    default:return "native_evaluation";
    }
}
struct RRPerfTicket {
    control_rr_perf::Ticket lease{};ID3D12GraphicsCommandList* list=nullptr;
    void* context=nullptr;unsigned long long frame=0,present=0;
};
struct RRPerfOwner {
    ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;
    ID3D12QueryHeap* queries=nullptr;ID3D12Resource* readback=nullptr;ID3D12Fence* fence=nullptr;
    UINT64* mapped=nullptr;UINT64 frequency=0,signal=0;DWORD thread=0;
    control_rr_perf::Pool<256> pool{};
    RRPerfStage stages[256]{};control_rr_perf::Key keys[256]{};
};
static RRPerfOwner* rrPerf=nullptr;
static bool rrPerfDisabled=false;
static control_rr_perf::Key rrPerfKey{};
static control_rr_perf::Stability rrPerfStability{};
static unsigned long long rrPerfFrame=0,rrPerfSkipped=0;
static DWORD rrPerfFrameThread=0;
static double rrPerfReplayPrepareMs=0,rrPerfReplayWorkersMs=0,rrPerfFilterMs=0;
static unsigned rrPerfReplayBatches=0;
static long long RRPerfClock() noexcept {LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
static double RRPerfElapsed(long long start) noexcept {
    return frequency.QuadPart?1000.0*static_cast<double>(RRPerfClock()-start)/static_cast<double>(frequency.QuadPart):0;
}
static void RRPerfDisable(const char* reason) noexcept {
    if(!rrPerfDisabled)Log("RR_PERF_DISABLED reason=%s rendering_unchanged=1 resources_retained=1",reason);
    rrPerfDisabled=true;
}
static bool RRPerfInitialize(const RRGuideInputSnapshot& in) noexcept {
    if(rrPerfDisabled)return false;
    if(rrPerf){
        if(rrPerf->queue!=in.queue||rrPerf->thread!=GetCurrentThreadId()){RRPerfDisable("queue_or_thread_changed");return false;}
        return true;
    }
    auto* p=new(std::nothrow) RRPerfOwner;if(!p){RRPerfDisable("allocation");return false;}
    rrPerf=p;p->queue=in.queue;p->queue->AddRef();p->thread=GetCurrentThreadId();
    HRESULT hr=p->queue->GetDevice(IID_PPV_ARGS(&p->device));
    if(SUCCEEDED(hr))hr=p->queue->GetTimestampFrequency(&p->frequency);
    D3D12_QUERY_HEAP_DESC query{};query.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;query.Count=512;
    if(SUCCEEDED(hr)&&p->frequency)hr=p->device->CreateQueryHeap(&query,IID_PPV_ARGS(&p->queries));
    if(SUCCEEDED(hr)&&p->queries){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=1;heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=4096;
        desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        hr=p->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&p->readback));
    }
    if(SUCCEEDED(hr)&&p->readback)hr=p->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&p->fence));
    D3D12_RANGE range{0,4096};
    if(SUCCEEDED(hr)&&p->fence)hr=p->readback->Map(0,&range,reinterpret_cast<void**>(&p->mapped));
    if(FAILED(hr)||!p->queries||!p->mapped||!p->fence||!p->frequency){RRPerfDisable("query_setup_failed");return false;}
    Log("RR_PERF_READY gpu_frequency=%llu sample_every=240 queries=512 readback_bytes=4096 wait_for_gpu=0 warmup_seconds=3 replay_gpu_timing=unavailable",p->frequency);
    return true;
}
static RRPerfTicket RRPerfBegin(const RRGuideInputSnapshot& in,RRPerfStage stage) noexcept {
    RRPerfTicket t{};const DWORD saved=GetLastError();
    __try {
        if(rrPerfDisabled||in.engineFrame!=rrPerfFrame||GetCurrentThreadId()!=rrPerfFrameThread||(in.engineFrame%240)!=0)__leave;
        if(!RRPerfInitialize(in))__leave;
        if(!rrPerf->pool.Begin(in.engineFrame,in.presentToken,t.lease)){++rrPerfSkipped;__leave;}
        t.list=in.commandList;t.context=in.commandContext;t.frame=in.engineFrame;t.present=in.presentToken;
        rrPerf->stages[t.lease.index]=stage;rrPerf->keys[t.lease.index]=rrPerfKey;
        t.list->EndQuery(rrPerf->queries,D3D12_QUERY_TYPE_TIMESTAMP,static_cast<UINT>(t.lease.index*2));
        ++*reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(t.context)+8);
    } __except(EXCEPTION_EXECUTE_HANDLER){RRPerfDisable("begin_exception");t={};}
    SetLastError(saved);return t;
}
static void RRPerfEnd(RRPerfTicket t,bool commandsOkay=true) noexcept {
    if(!t.list||!rrPerf||rrPerfDisabled)return;
    const DWORD saved=GetLastError();
    __try {
        if(t.lease.index>=256||rrPerf->pool.slots[t.lease.index].state!=control_rr_perf::State::Recording||
            rrPerf->pool.slots[t.lease.index].serial!=t.lease.serial){RRPerfDisable("stale_ticket");__leave;}
        RRGuideInputSnapshot in{};const char* reason=nullptr;
        const bool same=commandsOkay&&RRGuideReadRendererContext(&in,&reason)&&in.commandList==t.list&&in.commandContext==t.context&&
            in.engineFrame==t.frame&&in.presentToken==t.present&&in.queue==rrPerf->queue&&GetCurrentThreadId()==rrPerf->thread;
        if(same){
            const UINT index=static_cast<UINT>(t.lease.index*2);
            t.list->EndQuery(rrPerf->queries,D3D12_QUERY_TYPE_TIMESTAMP,index+1);
            t.list->ResolveQueryData(rrPerf->queries,D3D12_QUERY_TYPE_TIMESTAMP,index,2,rrPerf->readback,UINT64(index)*8);
            *reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(t.context)+8)+=2;
        }else ++rrPerfSkipped;
        // An abandoned pair is never resolved/read; its query indices still
        // remain reserved until that exact frame's native submission retires.
        if(!rrPerf->pool.End(t.lease,same))RRPerfDisable("stale_ticket");
    } __except(EXCEPTION_EXECUTE_HANDLER){RRPerfDisable("end_exception");}
    SetLastError(saved);
}
static void RRPerfAfterPresent(unsigned long long returned) noexcept {
    if(!rrPerf||rrPerfDisabled)return;
    const DWORD saved=GetLastError();
    __try {
        auto* p=rrPerf;if(GetCurrentThreadId()!=p->thread){RRPerfDisable("present_thread_changed");__leave;}
        const UINT64 completed=p->fence->GetCompletedValue();
        if(completed==UINT64_MAX){RRPerfDisable("device_removed");__leave;}
        for(std::size_t i=0;i<256;++i)if(p->pool.Completed(i,completed)){
            const auto s=p->pool.slots[i];double ms=0;
            if(s.resolved&&control_rr_perf::Milliseconds(p->mapped[i*2],p->mapped[i*2+1],p->frequency,ms)){
                const auto& k=p->keys[i];
                Log("RR_PERF_GPU frame=%llu stage=%s planned=%s width=%u height=%u ms=%.6f completed=1",s.frame,RRPerfName(p->stages[i]),k.rr?"RR":"SR",k.width,k.height,ms);
            }else if(s.resolved)Log("RR_PERF_GPU_INVALID frame=%llu stage=%s",s.frame,RRPerfName(p->stages[i]));
            p->pool.Retire(i,completed);
        }
        bool pending=false;
        for(const auto& s:p->pool.slots)if(s.state==control_rr_perf::State::Recording&&s.present<returned){
            RRPerfDisable("incomplete_scope");__leave;
        }
        for(const auto& s:p->pool.slots)if(s.state==control_rr_perf::State::Recorded){
            if(s.present+1!=returned){RRPerfDisable("missed_exact_present");__leave;}
            pending=true;
        }
        if(pending){
            const UINT64 value=++p->signal;
            if(FAILED(p->queue->Signal(p->fence,value))){RRPerfDisable("signal_failed");__leave;}
            for(std::size_t i=0;i<256;++i)if(p->pool.slots[i].state==control_rr_perf::State::Recorded)
                if(!p->pool.Submit(i,returned,value)){RRPerfDisable("submission_contract");__leave;}
        }
    } __except(EXCEPTION_EXECUTE_HANDLER){RRPerfDisable("retirement_exception");}
    SetLastError(saved);
}
static void RRPerfFrameMode(unsigned long long frame,unsigned ow,unsigned oh,unsigned width,unsigned height,
    bool requested,bool selected,bool ready,unsigned effects) noexcept {
    rrPerfFrame=frame;rrPerfFrameThread=GetCurrentThreadId();
    rrPerfKey={width,height,ow,oh,0,0,effects,selected,requested,ready};
    rrPerfReplayPrepareMs=0;rrPerfReplayWorkersMs=0;rrPerfFilterMs=0;rrPerfReplayBatches=0;
}
static void RRPerfReplayPrepared(unsigned long long frame,double ms) noexcept {
    if(frame==rrPerfFrame&&GetCurrentThreadId()==rrPerfFrameThread)rrPerfReplayPrepareMs=ms;
}
static void RRPerfReplayJoined(unsigned long long frame,double ms,unsigned batches) noexcept {
    if(frame==rrPerfFrame&&GetCurrentThreadId()==rrPerfFrameThread){rrPerfReplayWorkersMs=ms;rrPerfReplayBatches=batches;}
}
static void RRPerfFilter(unsigned long long frame,double ms) noexcept {
    if(frame==rrPerfFrame&&GetCurrentThreadId()==rrPerfFrameThread)rrPerfFilterMs+=ms;
}
static void RRPerfEvaluation(unsigned long long frame,bool rr,bool success,bool reset,long long entryTicks,
    double guidesCpuMs,double evaluationCpuMs,unsigned fg,unsigned hdr) noexcept {
    if(frame!=rrPerfFrame||GetCurrentThreadId()!=rrPerfFrameThread)return;
    const DWORD saved=GetLastError();auto key=rrPerfKey;key.rr=rr;key.fg=fg;key.hdr=hdr;
    double cadence=0;
    const bool eligible=rrPerfStability.Observe(key,frame,static_cast<unsigned long long>(entryTicks),
        static_cast<unsigned long long>(frequency.QuadPart),reset,success,cadence);
    if(frame%240==0||reset||!success)
        Log("RR_PERF_FRAME frame=%llu segment=%llu mode=%s requested=%u ready=%u eligible=%u width=%u height=%u output_width=%u output_height=%u fg=%u hdr=%u rt_effects=%u reset=%u cadence_ms=%.6f guides_cpu_ms=%.6f eval_cpu_ms=%.6f replay_prepare_cpu_ms=%.6f replay_workers_cpu_ms=%.6f replay_batches=%u filter_cpu_ms=%.6f gpu_skipped=%llu",
            frame,rrPerfStability.segment,success?(rr?"RR":"SR"):"FAILED",unsigned(key.requested),unsigned(key.ready),unsigned(eligible),
            key.width,key.height,key.outputWidth,key.outputHeight,fg,hdr,key.effects,unsigned(reset),cadence,guidesCpuMs,evaluationCpuMs,
            rrPerfReplayPrepareMs,rrPerfReplayWorkersMs,rrPerfReplayBatches,rrPerfFilterMs,rrPerfSkipped);
    SetLastError(saved);
}
