#include "pt_surface_history.h"
#include "pt_guide_admission.h"
// Included inside PTGlassRuntime after the native context/allocator adapters.
// One session owner. Every native call is still forwarded by its outer hook.
namespace Primary {
// Recovery packages must not accidentally enable the unfiltered replacement.
#if defined(CONTROL_PT_RECOVERY_BUILD) && defined(CONTROL_PT_SURFACE_REPLACEMENT)
#error Recovery builds must retain the native lighting reconstruction path
#endif
// Compile-time surface integration route. The packaged host and distinct
// shader sidecar must agree; unsupported materials retain native fallback.
#if defined(CONTROL_PT_SURFACE_REPLACEMENT)
static constexpr bool surfaceReplacement=true;
#else
static constexpr bool surfaceReplacement=false;
#endif
using View=PTNativeView::View;
using Bindings=PTCommandBindings::Snapshot;
using Table=PTTableMirror::Bank::Snapshot;
using Output=PTSurfaceComposition::Output;
static PTSceneHandoff::Runtime handoff{PTSceneLive::owner};
static constexpr const char* outputName=surfaceReplacement?"private_surface":"native_light_buffers";
static std::atomic<bool> active{false};
static bool transportValidated=false;
static unsigned transportGoodFrames=0;
static std::recursive_mutex mutex;
static std::vector<unsigned char> library,emitterLibrary;
static std::map<ID3D12RootSignature*,std::shared_ptr<PTEmitters::Builder>> emitterBuilders;
static std::shared_ptr<PTSurfaceComposition::ReconstructionPipeline> reconstructionPipeline;
static std::shared_ptr<Output> historyOutput;
static View historyView;
static ComPtr<ID3D12Resource> historyMaterials;
static std::shared_ptr<PTSurfaceComposition::SignalStorage> historySignals;
static PTNativeView::Providers viewProviders;
static std::uint64_t nextTicket=1ull<<63;
static std::atomic<unsigned> scheduleTrace{0};
static bool TraceSchedule(){auto remaining=scheduleTrace.load();while(remaining&& !scheduleTrace.compare_exchange_weak(remaining,remaining-1)){}return remaining!=0;}
static void FenceObserved(void*,ID3D12CommandQueue* queue,ID3D12Fence* fence,UINT64 value,bool signal,HRESULT hr) noexcept {
    if(!active||!TraceSchedule())return;
    Log("PT_PRIMARY_SCHEDULE operation=%s queue=%p type=%u fence=%p value=%llu completed=%llu hr=0x%08lX",signal?"signal":"wait",queue,unsigned(queue->GetDesc().Type),fence,value,fence->GetCompletedValue(),static_cast<unsigned long>(hr));
}
static ComPtr<IUnknown> pinnedQueue;
// Extend native AS/attribute read lifetime through the private traversal. Every
// later submission on another native queue waits for the latest PT use. The
// scene cut rejects ALREADY submitted unordered AS writes before arming rays.
static ComPtr<ID3D12Fence> sceneUseFence;
static UINT64 sceneUseValue=0,sceneUseNext=0;
static std::map<IUnknown*,std::pair<ComPtr<IUnknown>,UINT64>> sceneWaits;
static std::map<ID3D12RootSignature*,std::shared_ptr<PTScenePipeline::Pipeline>> privatePipelines;
static std::atomic<UINT64> jobCallbacks{0},jobOff{0},jobUnknownPipeline{0},opaqueAccepted{0};
static std::atomic<UINT64> computeCallbacks{0},computeOff{0},computeNoPipeline{0},computeOtherPipeline{0},primarySeen{0},environmentSeen{0};
static void Reject(const char* reason){
    // Throttle independently: common surface-binding misses must not hide
    // the first producer preparation/admission failure for hundreds of calls.
    static std::mutex rejectMutex;static std::map<std::string,UINT64> counts;
    std::lock_guard<std::mutex> lock(rejectMutex);const auto n=++counts[reason];
    if(n<=8||n%600==0)Log("PT_PRIMARY_SKIP reason=%s reason_count=%llu native_surface=retained",reason,n);
}
static ComPtr<IUnknown> Identity(IUnknown* object){ComPtr<IUnknown> result;if(object)object->QueryInterface(IID_PPV_ARGS(&result));return result;}
static bool ReadView(UINT64 address,View& view,PTNativeView::Pass pass=PTNativeView::Pass::opaque){
    if(!PTNativeView::uploads.Read(address,view.bytes.data(),view.bytes.size())&&
       !PTSceneLive::owner.Resources().ReadUpload(address,view.bytes.data(),view.bytes.size())){Reject("view_upload_untracked");return false;}
    if(!viewProviders.Read(view,pass)){
        static std::atomic<UINT64> failures{0};const auto n=++failures;
        if(n<=8||n%24000==0){float inverse[2]{};std::memcpy(inverse,view.bytes.data()+8,8);
            UINT boundValue=0,providerValue=0;
            if(view.failureOffset<=view.bytes.size()-4){std::memcpy(&boundValue,view.bound.data()+view.failureOffset,4);std::memcpy(&providerValue,view.bytes.data()+view.failureOffset,4);}
            Log("PT_PRIMARY_VIEW_INVALID reason=%s pass=%u offset=%u bound=0x%08X provider=0x%08X inverse=%.9g,%.9g dimensions=%ux%u frame=%u count=%llu",view.failure,unsigned(pass),view.failureOffset,boundValue,providerValue,inverse[0],inverse[1],view.width,view.height,view.frame,n);}
        Reject("view_constants_invalid");return false;
    }return true;
}
static double CpuMilliseconds(LARGE_INTEGER before,LARGE_INTEGER after){
    LARGE_INTEGER frequency{};if(!QueryPerformanceFrequency(&frequency)||!frequency.QuadPart)return 0;
    return double(after.QuadPart-before.QuadPart)*1000.0/double(frequency.QuadPart);
}
static bool SameView(UINT64 address,const View& expected){std::array<unsigned char,1136> current{};
    return (PTNativeView::uploads.Read(address,current.data(),current.size())||
        PTSceneLive::owner.Resources().ReadUpload(address,current.data(),current.size()))&&expected.MatchesBound(current);
}
static bool ContextFor(ID3D12GraphicsCommandList* list,const Bindings& bindings,UINT tableParameter,Context& context){
    if(!ReadContext(&context)||Identity(context.list).Get()!=Identity(list).Get()||
       Identity(context.table.heap).Get()!=Identity(bindings.resources.Get()).Get())return false;
    const auto q=Identity(context.queue);if(!q)return false;
    if(pinnedQueue&&pinnedQueue.Get()!=q.Get())return false;
    const auto base=bindings.resources->GetGPUDescriptorHandleForHeapStart().ptr;
    const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto address=bindings.arguments[tableParameter].value;
    if(!step||address<base||(address-base)%step)return false;
    const auto first=(address-base)/step;
    if(first>UINT32_MAX||first+49>bindings.resources->GetDesc().NumDescriptors)return false;
    context.table.first=static_cast<UINT>(first);context.gpu.ptr=address;
    if(!PTSceneQueues::WatchQueue(context.queue)||!PTSceneQueues::WatchList(list))return false;
    if(!pinnedQueue)pinnedQueue=q;return true;
}
struct Frame;
struct Candidate {PTNativeJobRoute::Ticket ticket;View view;std::shared_ptr<Frame> frame;};
static std::map<UINT64,Candidate> candidates;
struct Environment {
    View view;Table table;ComPtr<IUnknown> list,queue;bool submitted=false;
    UINT64 scaleAddress=0;float scale=0;
    UINT64 giAddress=0,giScaleAddress=0;bool gi=false;
    std::array<unsigned char,64> giConstants{};
};
static std::vector<std::shared_ptr<Environment>> environments;
struct Assets {
    PTPrimaryMetrics::Counters metrics;
    std::array<double,12> cpuCosts{}; // prepare, lease, SBT, camera, metrics, record
    std::shared_ptr<PTUploadPool::Buffer> cameraStorage;
    ComPtr<ID3D12Resource> camera;View boundView;
    std::array<unsigned char,416> boundLights{},lightConstants{};
    std::shared_ptr<PTSurfaceComposition::ReconstructionPass> reconstruction;
    std::shared_ptr<Output> history;
    Bindings bindings,graphicsRestore;Table primary;std::shared_ptr<Environment> environment;
    PTOwnedScene::LeaseBatch scene;
    std::shared_ptr<const PTSceneHandoff::Prepared> proof;
    std::shared_ptr<PTScenePipeline::Dispatch> dispatch;
    PTGlass::DescriptorRange privateTable;
    std::shared_ptr<PTEmitters::Storage> emitters;
    std::shared_ptr<PTEmitters::Builder> emitterBuilder;
    std::vector<PTEmitters::Geometry> emitterGeometry;
    UINT64 ticket=0,engineFrame=0;bool probe=true;
};
struct Frame {
    View view;std::shared_ptr<Output> output,raw;ComPtr<ID3D12CommandQueue> queue;
    std::shared_ptr<Assets> assets;bool producerRecorded=false,producerSubmitted=false,armed=false,deferred=false,cancelled=false;
    PTSceneLive::Capture::PendingFence wait;UINT64 sceneUse=0;
};
struct Draw {
    std::shared_ptr<Frame> frame;std::shared_ptr<PTSurfaceComposition::Pipeline> pipeline;
    Table original;UINT64 viewAddress=0,vertexViewAddress=0;View boundView;bool pendingCamera=false;std::shared_ptr<Output> consumer;
};
struct Guide {std::shared_ptr<Frame> frame;std::shared_ptr<Output> guard;bool selected=false;};
struct Recording {
    ComPtr<IUnknown> identity;std::vector<std::shared_ptr<Draw>> draws;
    std::vector<std::shared_ptr<Frame>> producers;std::vector<std::shared_ptr<Guide>> guides;bool submitted=false;
};
static std::map<IUnknown*,Recording> recordings;
static std::vector<std::weak_ptr<Frame>> frames;
static std::vector<std::shared_ptr<Frame>> pendingMetrics;
static void PollMetrics(){
    for(auto it=pendingMetrics.begin();it!=pendingMetrics.end();){auto& f=**it;
        if(!f.producerSubmitted||!f.output->Completed()){++it;continue;}
        if(f.deferred&&f.assets){
            static UINT64 expired=0;if(++expired<=8||expired%240==0)
                Log("PT_PRIMARY_LATE_EXPIRED ticket=%llu fence=%llu required=%llu",f.assets->ticket,f.wait.graph.fence,f.wait.graph.value);
            f.deferred=false;f.cancelled=true;PTSceneLive::owner.Scenes().Forget(f.assets->ticket);
        }
        std::array<UINT,8> counts{};const bool good=f.assets&&f.assets->metrics.Read(counts);
        if(f.assets){f.assets->metrics.CompletedForReuse();f.assets->history.reset();}
        std::array<UINT,16> faults{};const bool faultRead=good&&((!surfaceReplacement&&!counts[7])||f.assets->metrics.ReadFaults(faults));
        if(good&&f.armed&&counts[5]){
            const auto health=faultRead?PTPrimaryReadiness::Check(counts[5],counts[6],counts[7],faults[12]):PTPrimaryReadiness::Result::unhealthy;
            if(health==PTPrimaryReadiness::Result::unhealthy){transportGoodFrames=0;transportValidated=false;}
            else if(health==PTPrimaryReadiness::Result::healthy&&f.assets->probe&&!transportValidated&&++transportGoodFrames>=3){
                transportValidated=true;Log("PT_PRIMARY_TRANSPORT_READY frame=%u covered=%u stable_frames=%u",f.view.frame,counts[6],transportGoodFrames);}
        }
        static UINT64 samples=0;const auto n=++samples;
        if(n<=8||n%240==0)Log("PT_PRIMARY_GPU frame=%u readback=%u armed=%u sampled=%u dimensions=%u primary_hit=%u material_match=%u depth_match=%u integrated=%u covered=%u fault=%u probe=%u sparse_step=64 output=%s",f.view.frame,unsigned(good),unsigned(f.armed),counts[0],counts[1],counts[2],counts[3],counts[4],counts[5],counts[6],counts[7],unsigned(f.assets->probe),outputName);
        if(good&&(n<=8||n%240==0)){UINT64 frequency=0;double ms=0;
            const bool timed=SUCCEEDED(f.queue->GetTimestampFrequency(&frequency))&&f.assets->metrics.ReadMilliseconds(frequency,ms);
            Log("PT_PRIMARY_TIMING frame=%u valid=%u dispatch_gpu_ms=%.6f probe=%u roulette_start=3 max_hits=4",f.view.frame,unsigned(timed),ms,unsigned(f.assets->probe));}
        if(good&&f.armed&&!f.cancelled&&!f.assets->probe){
            UINT64 frequency=0;double ms=0;
            if(SUCCEEDED(f.queue->GetTimestampFrequency(&frequency))&&f.assets->metrics.ReadMilliseconds(frequency,ms)){
                static PTScalingMetrics::Window window;
                if(window.Add(f.view.width,f.view.height,ms,f.assets->cpuCosts)){
                    const double n=double(window.samples);const auto& c=window.cpu;
                    Log("PT_PRIMARY_SCALING dimensions=%ux%u samples=%u sequence=%llu gpu_avg_ms=%.6f prepare_avg_ms=%.6f lease_avg_ms=%.6f sbt_avg_ms=%.6f camera_avg_ms=%.6f metrics_avg_ms=%.6f record_avg_ms=%.6f setup_other_avg_ms=%.6f handoff_wait_avg_ms=%.6f scene_preview_avg_ms=%.6f owner_wait_avg_ms=%.6f plan_validate_avg_ms=%.6f resource_retain_avg_ms=%.6f proof_avg_ms=%.6f scope=completed_armed_pt_not_total_frame_time",
                        window.width,window.height,window.samples,static_cast<UINT64>(window.sequence),window.gpu/n,c[0]/n,c[1]/n,c[2]/n,c[3]/n,c[4]/n,c[5]/n,(c[0]-c[1]-c[2]-c[3]-c[4]-c[5])/n,c[6]/n,c[7]/n,c[8]/n,c[9]/n,c[10]/n,c[11]/n);
                    const auto uploads=PTUploadPool::Buffer::Statistics();
                    Log("PT_PRIMARY_UPLOAD_POOL created=%llu reused=%llu idle_bytes=%llu cumulative=1 retirement=recordings_and_gpu_fence",uploads.created,uploads.reused,uploads.idleBytes);
                    const auto metrics=PTPrimaryMetrics::Counters::Statistics();
                    Log("PT_PRIMARY_METRICS_POOL created=%llu reused=%llu idle=%llu cumulative=1 retirement=recordings_and_gpu_fence admission=cleared_after_trace",metrics.created,metrics.reused,metrics.idle);
                    window.ClearSamples();
                }
            }
        }
        if(surfaceReplacement&&good&&!f.assets->probe&&(n<=8||n%240==0)){
            std::array<UINT,3> c{};if(f.assets->metrics.ReadCorrespondence(c))
                Log("PT_PRIMARY_CORRESPONDENCE frame=%u different_material_same_depth=%u different_material_different_depth=%u same_material_different_depth=%u rejection_retained=1",f.view.frame,c[0],c[1],c[2]);
            std::array<UINT,12> h{};if(f.assets->metrics.ReadReconstruction(h))
                Log("PT_PRIMARY_HISTORY_PIXELS frame=%u valid=%u available=%u projected=%u covered=%u compatible=%u accepted=%u sample_sum=%u material_match=%u normal_match=%u rescued=%u stable_material_match=%u reindexed_accepted=%u taps=4 candidates=4",f.view.frame,h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7],h[8],h[9],h[10],h[11]);
        }
        if(surfaceReplacement&&faultRead&&(n<=8||n%240==0))
            Log("PT_PRIMARY_TERMINAL_GI_GPU frame=%u positive_samples=%u sampled_grid=64 terminal_vertex=diffuse_hit_or_limit_or_miss",f.view.frame,faults[15]);
        if(surfaceReplacement&&good&&!f.assets->probe&&(n<=8||n%240==0)){
            std::array<UINT,3> hitGI{};if(f.assets->metrics.ReadHitGI(hitGI))
                Log("PT_PRIMARY_HIT_GI_GPU frame=%u requests=%u cache_positive=%u cache_unavailable=%u sampled_grid=64 diffuse_tail_replaced=1",f.view.frame,hitGI[0],hitGI[1],hitGI[2]);
            std::array<UINT,2> miss{};if(f.assets->metrics.ReadMissGI(miss))
                Log("PT_PRIMARY_MISS_GI_GPU frame=%u requests=%u cache_positive=%u sampled_grid=64 native_extent=1",f.view.frame,miss[0],miss[1]);
        }
        if(surfaceReplacement&&faultRead&&faults[0]&&(n<=8||n%240==0))
            Log("PT_PRIMARY_SECONDARY_TERMINATION frame=%u sampled=%u retained_primary=1",f.view.frame,faults[0]);
        if(good&&counts[7]&&(n<=8||n%240==0)){
            std::array<UINT,256> rejected{};
            if(f.assets->metrics.ReadRejections(rejected))for(UINT tile=0;tile<32;++tile){const auto* r=rejected.data()+tile*8;
                if(r[0])Log("PT_PRIMARY_REJECT_REGION frame=%u tile=%u sampled=%u x=%u y=%u material=%u flags_x=%08x flags_y=%08x fault=%u",f.view.frame,tile,r[0],r[1],r[2],r[3],r[4],r[5],r[6]);
            }
        }
        if(good&&counts[7]){
            if(faultRead&&(n<=8||n%240==0))
                Log("PT_PRIMARY_FAULTS frame=%u probe=%u input=%u hit=%u material=%u normal=%u facing=%u light=%u direct=%u sample=%u spawn=%u radiance=%u material_bounds=%u brdf=%u material_values=%u native_light=%u stage=primary_rejection",f.view.frame,unsigned(f.assets->probe),faults[1],faults[2],faults[3],faults[4],faults[5],faults[6],faults[7],faults[8],faults[9],faults[10],faults[11],faults[12],faults[13],faults[14]);
        }
        it=pendingMetrics.erase(it);
    }
}
static Recording* RecordingFor(ID3D12GraphicsCommandList* list){
    auto key=Identity(list);if(!key)return nullptr;auto it=recordings.find(key.Get());
    if(it!=recordings.end())return &it->second;if(recordings.size()>=4096)return nullptr;
    Recording r;r.identity=key;return &recordings.emplace(key.Get(),std::move(r)).first->second;
}
static std::shared_ptr<Frame> FrameFor(const View& view,ID3D12CommandQueue* queue){
    for(auto it=frames.begin();it!=frames.end();){auto f=it->lock();if(!f){it=frames.erase(it);continue;}
        if(f->view.Same(view))return f;++it;
    }
    if(frames.size()>=8)return {};
    auto f=std::make_shared<Frame>();f->view=view;f->queue=queue;
    if(FAILED(Output::Create(queue,surfaceReplacement?view.width:1,surfaceReplacement?view.height:1,f->output,true,surfaceReplacement,surfaceReplacement?64:16)))return {};
    if(surfaceReplacement&&FAILED(Output::Create(queue,view.width,view.height,f->raw,true,true,64)))return {};
    frames.push_back(f);return f;
}
static D3D12_CPU_DESCRIPTOR_HANDLE Cpu(const PTGlass::DescriptorRange& range,UINT slot=0){
    auto handle=range.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr+=SIZE_T(range.first+slot)*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);return handle;
}
static D3D12_GPU_DESCRIPTOR_HANDLE Gpu(const PTGlass::DescriptorRange& range){
    auto handle=range.heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr+=UINT64(range.first)*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);return handle;
}
static bool Disjoint(const PTGlass::DescriptorRange& a,UINT original){return a.first+49<=original||original+49<=a.first;}
// Defined below with the other submission/admission helpers.
static void CancelFrame(const std::shared_ptr<Frame>& frame,ID3D12CommandQueue* queue);
static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource*,D3D12_CPU_DESCRIPTOR_HANDLE){
    auto& a=*static_cast<Assets*>(context);ComPtr<ID3D12GraphicsCommandList4> rays;if(FAILED(list->QueryInterface(IID_PPV_ARGS(&rays))))return false;
    auto counter=Gpu(a.privateTable);counter.ptr+=47ull*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_RESOURCE_BARRIER ordered{};ordered.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
    list->ResourceBarrier(1,&ordered); // Native u0/u1 writes precede the private overwrite.
    a.metrics.Begin(list,counter);
    if(surfaceReplacement){
        auto emitterUav=Gpu(a.privateTable);emitterUav.ptr+=45ull*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(!a.emitterBuilder->Record(list,a.bindings,Gpu(a.privateTable),emitterUav,a.emitters->Uav(),*a.emitters,a.emitterGeometry))return false;
    }
    list->SetComputeRootConstantBufferView(2,a.camera->GetGPUVirtualAddress());
    list->SetComputeRootConstantBufferView(4,a.camera->GetGPUVirtualAddress()+1280);
    const bool recorded=a.dispatch->Record(rays.Get(),a.bindings.layout->Root(),a.bindings.pipeline.Get(),{a.bindings.arguments[0].value},Gpu(a.privateTable));
    list->SetComputeRootConstantBufferView(2,a.bindings.arguments[2].value);
    list->SetComputeRootConstantBufferView(4,a.bindings.arguments[4].value);
    list->ResourceBarrier(1,&ordered); // Surface consumers observe both committed outputs.
    if(!surfaceReplacement)a.metrics.End(list);return recorded;
}
static bool Reconstruct(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource* output,D3D12_CPU_DESCRIPTOR_HANDLE){
    auto& a=*static_cast<Assets*>(context);if(!a.reconstruction)return false;
    a.reconstruction->Record(list,output,a.metrics.Buffer());
    // The filter uses its own three-descriptor heap. Restore BOTH native heaps
    // and every native compute argument before returning to the engine.
    ID3D12DescriptorHeap* heaps[]={a.bindings.resources.Get(),a.bindings.samplers.Get()};list->SetDescriptorHeaps(2,heaps);
    list->SetComputeRootSignature(a.bindings.layout->Root());
    for(UINT n=0;n<8;++n){const auto& argument=a.bindings.arguments[n];
        if(argument.kind==PTCommandBindings::ArgumentKind::table)list->SetComputeRootDescriptorTable(n,{argument.value});
        else if(argument.kind==PTCommandBindings::ArgumentKind::constant)list->SetComputeRootConstantBufferView(n,argument.value);
    }
    if(a.graphicsRestore.layout){
        list->SetGraphicsRootSignature(a.graphicsRestore.layout->Root());
        for(UINT n=0;n<18;++n){const auto& argument=a.graphicsRestore.arguments[n];
            if(argument.kind==PTCommandBindings::ArgumentKind::table)list->SetGraphicsRootDescriptorTable(n,{argument.value});
            else if(argument.kind==PTCommandBindings::ArgumentKind::constant)list->SetGraphicsRootConstantBufferView(n,argument.value);
        }
    }
    list->SetPipelineState(a.bindings.pipeline.Get());a.metrics.End(list);return true;
}
static void Job(PTNativeJobRuntime::Phase phase,const PTNativeJobRoute::Ticket& ticket) noexcept {
    if(!active)return;
    try {
        // Pre-schedule WriterReady owns its input until this callback returns. It never waits
        // for the native worker or acquires the queue submission lock.
        if(phase==PTNativeJobRuntime::Phase::writerReady){handoff.WriterReady(ticket);return;}
        std::lock_guard<std::recursive_mutex> ordered(PTSceneQueues::queueMutex);
        std::lock_guard<std::recursive_mutex> guard(mutex);
        if(phase==PTNativeJobRuntime::Phase::cancelled){handoff.Cancelled(ticket.serial);candidates.erase(ticket.serial);return;}
        ++jobCallbacks;
        if(ptReflectionUserState.load(std::memory_order_acquire)!=1){++jobOff;
            for(const auto& c:candidates)handoff.Cancelled(c.first);candidates.clear();return;
        }
        if(!PTSceneLive::owner.Scenes().Healthy())return;
        auto pipeline=opaquePipelines.Find(reinterpret_cast<ID3D12StateObject*>(ticket.pipeline));if(!pipeline){++jobUnknownPipeline;return;}
        ++opaqueAccepted;auto* list=reinterpret_cast<ID3D12GraphicsCommandList4*>(ticket.list);Bindings bindings;View view;Context context;
        if(!PTCommandBindings::tracker.Read(list,nativeRoots,false,0xc5,bindings)||!pipeline->root->Matches(bindings.layout->Root())||
           !ReadView(bindings.arguments[2].value,view)||view.engineFrame!=ticket.engineFrame||view.width!=ticket.width||view.height!=ticket.height||
           !ContextFor(list,bindings,0,context)){Reject("opaque_view_or_context");return;}
        for(auto it=candidates.begin();it!=candidates.end();){
            if(it->second.ticket.engineFrame<ticket.engineFrame&&ticket.engineFrame-it->second.ticket.engineFrame>2){handoff.Cancelled(it->first);it=candidates.erase(it);}else ++it;
        }
        while(candidates.size()>=8){handoff.Cancelled(candidates.begin()->first);candidates.erase(candidates.begin());}
        if(handoff.Recorded(ticket,bindings.resources.Get(),context.table.first,5)){
            // Reserve before native surface workers record their draws. CPU
            // producer recording order is NOT the GPU execution dependency.
            std::shared_ptr<Frame> frame;
            if constexpr(surfaceReplacement){
                PTSceneQueues::Scope privateQueues;
                PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
                frame=FrameFor(view,context.queue);
                if(!frame||frame->cancelled){
                    handoff.Cancelled(ticket.serial);Reject("surface_frame_reservation");return;
                }
                static UINT64 reserved=0;if(++reserved<=8||reserved%240==0)
                    Log("PT_PRIMARY_SURFACE_RESERVED frame=%u ticket=%llu binding=root_buffer stage=opaque_job",view.frame,ticket.serial);
            }
            // Legacy PT creates its placeholder at the lighting producer, as in
            // R66. Do not acquire graphics descriptor locks or allocate early
            // output resources for a route that has no surface consumers.
            candidates.emplace(ticket.serial,Candidate{ticket,view,std::move(frame)});
        }else Reject("opaque_handoff");
    }catch(...){Reject("job_allocation");}
}
static void Dispatched(void*,ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) noexcept {
    if(!active)return;++computeCallbacks;
    if(ptReflectionUserState.load(std::memory_order_acquire)!=1){++computeOff;return;}
    if(!PTSceneLive::owner.Scenes().Healthy())return;
    try {
        const auto pso=PTCommandBindings::tracker.PipelineFor(list);
        PTPrimaryPipelineProbe::Used(pso.Get(),x,y,z);
        const bool environmentPass=pso.Get()==ptReflectionNativeCompositePso||pso.Get()==ptReflectionP2CompositePso;
        const auto primary=primaryPipelines.Find(pso.Get());
        if(!pso){++computeNoPipeline;return;}
        if(!environmentPass&&!primary){++computeOtherPipeline;return;}
        if(environmentPass)++environmentSeen;else ++primarySeen;
        PTHotpathTiming::Sample profile(PTHotpathTiming::Kind::primary_queue_wait,!environmentPass);
        std::lock_guard<std::recursive_mutex> ordered(PTSceneQueues::queueMutex);PTSceneQueues::Scope privateQueues;
        profile.Next(PTHotpathTiming::Kind::primary_guard_wait);
        std::lock_guard<std::recursive_mutex> guard(mutex);std::optional<PTTableMirror::Scope> descriptorLock(std::in_place,true);
        std::optional<PTTableMirror::Mute> privateWrites(std::in_place);
        profile.Next(PTHotpathTiming::Kind::primary_setup);
        Bindings bindings;if(!PTCommandBindings::tracker.Read(list,nativeRoots,false,(environmentPass&&surfaceReplacement)?0xdd:0xd5,bindings)){Reject("lighting_bindings");return;}
        View view;Context context;
        if(!ReadView(bindings.arguments[2].value,view,environmentPass?PTNativeView::Pass::environment:PTNativeView::Pass::primary)||!ContextFor(list,bindings,0,context)){Reject("lighting_view_or_context");return;}
        if(environmentPass){
            // Prefer the first observed native/P2 dispatch for this immutable
            // table/view, not a guessed globally most recent environment.
            for(const auto& e:environments)if(e->view.Same(view)&&e->table.native.Get()==bindings.resources.Get()&&e->table.first==context.table.first)return;
            auto e=std::make_shared<Environment>();e->view=view;e->list=Identity(list);e->queue=Identity(context.queue);
            if(surfaceReplacement){
                // This exact cube is consumed with b1[15].y by the reviewed
                // environment composite. The opaque-pass provider can be zero
                // while ray-traced environment lighting remains enabled.
                e->scaleAddress=bindings.arguments[3].value+244;
                if((!PTNativeView::uploads.Read(e->scaleAddress,&e->scale,4)&&
                    !PTSceneLive::owner.Resources().ReadUpload(e->scaleAddress,&e->scale,4))||
                   !std::isfinite(e->scale)||e->scale<0){Reject("environment_scale_snapshot");return;}
            }
            if(!e->gi&&!PTTableMirror::bank.Read(bindings.resources.Get(),context.table.first,1ull<<13,e->table)){Reject("environment_descriptor");return;}
            while(environments.size()>=16)environments.erase(environments.begin());environments.push_back(std::move(e));return;
        }
        if(z!=1||x!=(view.width+7)/8||y!=(view.height+7)/8||!primary->root->Matches(bindings.layout->Root())){Reject("primary_dispatch_shape_or_root");return;}
        struct RetireAttempt {
            const View& view;
            ~RetireAttempt(){for(auto it=candidates.begin();it!=candidates.end();){
                if(it->second.view.Same(view)){handoff.Cancelled(it->first);it=candidates.erase(it);}else ++it;
            }}
        } retire{view};
        Candidate* candidate=nullptr;
        for(auto& c:candidates)if(c.second.view.Same(view)){if(candidate){Reject("ambiguous_scene_view");return;}candidate=&c.second;}
        std::shared_ptr<Environment> environment;
        for(const auto& e:environments)if(e->view.Same(view)&&e->queue.Get()==pinnedQueue.Get()){
            if(environment){Reject("ambiguous_environment_view");return;}environment=e;
        }
        if(!candidate||!environment){Reject("scene_or_environment_not_ready");return;}
        LARGE_INTEGER prepareStart{},readyDone{},leaseStart{},leaseDone{},recordDone{},sbtStart{},cameraStart{},metricsStart{},encodeStart{};QueryPerformanceCounter(&prepareStart);
        UINT64 serial=0;
        if(!handoff.FindReady(candidate->ticket.engineFrame,view.width,view.height,candidate->ticket.pipeline,serial)){
            std::size_t row=0;const char* reason=handoff.NotReady(candidate->ticket.serial,row);
            static UINT64 failures=0;if(++failures<=16||failures%600==0){
                Log("PT_PRIMARY_HANDOFF ticket=%llu frame=%llu reason=%s row=%llu native_list=%p lighting_list=%p",candidate->ticket.serial,candidate->ticket.engineFrame,reason,static_cast<UINT64>(row),reinterpret_cast<void*>(candidate->ticket.list),list);
                const auto detail=handoff.DescriptorFailure(candidate->ticket.serial);
                const auto index=PTSceneLive::owner.DescriptorStatistics();
                Log("PT_PRIMARY_DESCRIPTOR reason=%s handle=%llx address=%llx token=%llu bytes=%llu current_token=%llu index_healthy=%u index_failure=%s index_entries=%llu writes=%llu as_writes=%llu copies=%llu",detail.reason,detail.handle,detail.address,detail.token,detail.bytes,detail.currentToken,unsigned(index.healthy),index.failure,index.entries,index.writes,index.asWrites,index.copies);
            }
            Reject(reason);return;
        }
        QueryPerformanceCounter(&readyDone);
        auto* recording=RecordingFor(list);
        if(!recording||recording->submitted){Reject("frame_budget_or_duplicate");return;}
        auto a=std::make_shared<Assets>();a->bindings=bindings;a->boundView=view;a->environment=environment;a->engineFrame=candidate->ticket.engineFrame;a->probe=!transportValidated;
        if((!PTNativeView::uploads.Read(bindings.arguments[4].value,a->boundLights.data(),a->boundLights.size())&&
            !PTSceneLive::owner.Resources().ReadUpload(bindings.arguments[4].value,a->boundLights.data(),a->boundLights.size()))||
            !viewProviders.ReadLights(view,a->boundLights,a->lightConstants)){Reject("light_provider_snapshot");return;}
        if(surfaceReplacement)std::memcpy(a->lightConstants.data()+244,&environment->scale,4);
        static UINT64 lightSnapshots=0;if(++lightSnapshots<=8||lightSnapshots%240==0){UINT totals[3]{};float scale=0;
            std::memcpy(totals,a->lightConstants.data()+256,12);std::memcpy(&scale,a->lightConstants.data()+244,4);
            Log("PT_PRIMARY_LIGHTS frame=%u points=%u spots=%u suns=%u environment=%.9g source=authenticated_provider environment_source=%s primary_candidates=%s",view.frame,totals[0],totals[1],totals[2],scale,surfaceReplacement?"bound_reflection_composite":"opaque_provider",surfaceReplacement?"enumerate_up_to_64":"uniform");}
        // t0,t1,t3,t7,t8,t10,t14,t15,t17,t18 and native outputs.
        constexpr UINT64 required=(1ull<<5)|(1ull<<6)|(1ull<<8)|(1ull<<12)|(1ull<<13)|(1ull<<15)|(1ull<<19)|(1ull<<20)|(1ull<<22)|(1ull<<23)|(1ull<<41)|(1ull<<42);
        if(!PTTableMirror::bank.Read(bindings.resources.Get(),context.table.first,required,a->primary)||
           !PTTableMirror::bank.Unchanged(environment->table)||!Fresh(context,&a->privateTable)||!Disjoint(a->privateTable,context.table.first)){Reject("primary_descriptor_allocation");return;}
        if(!surfaceReplacement&&!PTTableMirror::bank.TypedLoadSupported(a->primary,42)){Reject("native_specular_typed_load_unproven");return;}
        // CPU-owned descriptor values are immutable. Keep scene/recording ownership
        // serialized, but allow native descriptor workers through during preparation.
        descriptorLock.reset();
        LARGE_INTEGER descriptorReleased{},descriptorReacquire{},descriptorAcquired{};
        QueryPerformanceCounter(&descriptorReleased);
        auto& pipeline=privatePipelines[bindings.layout->Root()];
        if(!pipeline){
            if(privatePipelines.size()>8){Reject("pipeline_budget");return;}ComPtr<ID3D12Device5> rayDevice;
            if(FAILED(device.As(&rayDevice)))return;auto p=std::make_shared<PTScenePipeline::Pipeline>();
            const HRESULT hr=p->Initialize(rayDevice.Get(),bindings.layout->Root(),library.data(),library.size(),L"PTPrimaryPathRayGeneration",nullptr);
            if(FAILED(hr)){Log("PT_PRIMARY_PIPELINE_FAILED hr=0x%08lX stage=%s device_before=0x%08lX device_after=0x%08lX terminal_gi_callable=%u",static_cast<unsigned long>(hr),p->failureStage,static_cast<unsigned long>(p->deviceBefore),static_cast<unsigned long>(p->deviceAfter),0u);return;}
            Log("PT_PRIMARY_PRIVATE_PIPELINE_READY terminal_gi_callable=%u",unsigned(p->HasCallable()));pipeline=std::move(p);
        }
        PTNativeShaderTable::Identifiers identifiers;PTSceneDispatch::ExportSet exports;
        if(!handoff.ReadyIdentifiers(serial,identifiers)||!pipeline->Exports(candidate->ticket.pipeline,identifiers.hit,exports)){Reject("private_exports");return;}
        profile.Next(PTHotpathTiming::Kind::primary_lease);QueryPerformanceCounter(&leaseStart);
        PTPreparationMetrics::Costs preparationCosts{};
        const auto status=handoff.PrepareRecording(serial,exports,a->scene,a->proof,&preparationCosts);
        QueryPerformanceCounter(&leaseDone);profile.Next(PTHotpathTiming::Kind::primary_setup);
        if(status!=PTSceneHandoff::Status::pending)candidates.erase(serial);
        if(status!=PTSceneHandoff::Status::prepared){
            std::size_t row=0;const char* reason=handoff.NotReady(serial,row);
            static UINT64 failures=0;if(++failures<=16||failures%600==0){const auto b=handoff.BindingFailure(serial);
                Log("PT_PRIMARY_PREFLIGHT ticket=%llu reason=%s row=%llu indices=%llx attributes=%llx index_bytes=%u attribute_bytes=%u",serial,reason,static_cast<UINT64>(row),b.indices,b.attributes,b.constants[8],b.constants[9]);
                const auto wait=PTSceneLive::owner.Scenes().PendingWait(serial);
                Log("PT_PRIMARY_PENDING_FENCE ticket=%llu fence=%llu required=%llu last_signal=%llu graph_completed=%llu actual_completed=%llu polled=%u lane=%llu stamp=%llu",serial,wait.graph.fence,wait.graph.value,wait.graph.signalValue,wait.graph.completed,wait.actual,unsigned(wait.polled),wait.graph.lane,wait.graph.stamp);
            }
            Reject(reason);return;
        }
        const auto& plan=a->proof->DispatchPlan();
        if(surfaceReplacement){
            if(!PTEmitters::Plan(a->proof->SceneSnapshot(),plan,pipeline->SurfaceAlpha(),a->emitterGeometry)||
               FAILED(PTEmitters::Storage::Acquire(device.Get(),a->emitters))){Reject("emitter_scene_plan");return;}
            auto& builder=emitterBuilders[bindings.layout->Root()];
            if(!builder){auto fresh=std::make_shared<PTEmitters::Builder>();
                if(FAILED(fresh->Initialize(device.Get(),*bindings.layout,emitterLibrary.data(),emitterLibrary.size()))){Reject("emitter_pipeline");return;}
                builder=std::move(fresh);
            }
            a->emitterBuilder=builder;
        }
        // Canonicalization is CPU-only and uses immutable prepared records.
        // Keep it in the unlocked preparation interval, not the descriptor gate.
        std::optional<PTMaterialHistory::Keys> materialKeys;
        if(surfaceReplacement)materialKeys=PTMaterialHistory::GeometryKeys(plan.records);
        auto frame=FrameFor(view,context.queue);
        if(!frame||frame->cancelled||frame->producerRecorded){Reject("frame_budget_or_duplicate");return;}
        // The output owner retains scene dependencies through producer and surface
        // consumer completion; the surface variant also owns radiance storage.
        profile.Next(PTHotpathTiming::Kind::primary_sbt);QueryPerformanceCounter(&sbtStart);
        if(FAILED(PTScenePipeline::Dispatch::Create(device.Get(),pipeline,plan,view.width,view.height,bindings.resources.Get(),a->dispatch))){Reject("private_sbt");return;}
        profile.Next(PTHotpathTiming::Kind::primary_camera);QueryPerformanceCounter(&cameraStart);
        if(FAILED(PTUploadPool::Buffer::Acquire(device.Get(),2048,a->cameraStorage))){Reject("camera_upload");return;}
        a->camera=a->cameraStorage->Resource();
        void* cameraData=nullptr;const D3D12_RANGE noRead{};
        if(FAILED(a->camera->Map(0,&noRead,&cameraData))||!cameraData){Reject("camera_map");return;}
        std::memcpy(cameraData,view.bytes.data(),view.bytes.size());
        std::memcpy(static_cast<unsigned char*>(cameraData)+1280,a->lightConstants.data(),a->lightConstants.size());
        std::memcpy(static_cast<unsigned char*>(cameraData)+1792,environment->giConstants.data(),64);
        const UINT giExtent[]={view.width,view.height};std::memcpy(static_cast<unsigned char*>(cameraData)+1848,giExtent,8);a->camera->Unmap(0,nullptr);
        profile.Next(PTHotpathTiming::Kind::primary_metrics);QueryPerformanceCounter(&metricsStart);
        if(!a->metrics.Initialize(device.Get())){Reject("coverage_allocation");return;}
        QueryPerformanceCounter(&descriptorReacquire);
        // Mute suppresses nested hooks, including Scope acquisition. Restore it
        // only AFTER reacquiring the gate, or revalidation would run unlocked.
        privateWrites.reset();descriptorLock.emplace(true);privateWrites.emplace();
        QueryPerformanceCounter(&descriptorAcquired);
        if(!PTTableMirror::bank.Unchanged(a->primary)||!PTTableMirror::bank.Unchanged(environment->table)){
            Reject("primary_descriptors_changed_during_prepare");return;
        }
        profile.Next(PTHotpathTiming::Kind::primary_record);QueryPerformanceCounter(&encodeStart);
        device->CopyDescriptorsSimple(49,Cpu(a->privateTable),a->primary.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1,Cpu(a->privateTable,47),a->metrics.Uav(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(surfaceReplacement){
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_TYPELESS;srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
            srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=PTEmitters::Storage::Bytes/4;srv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
            device->CreateShaderResourceView(a->emitters->Resource(),&srv,Cpu(a->privateTable,39));
            device->CopyDescriptorsSimple(1,Cpu(a->privateTable,45),a->emitters->Uav(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        if(surfaceReplacement){
            D3D12_CONSTANT_BUFFER_VIEW_DESC giParameters{};giParameters.BufferLocation=a->camera->GetGPUVirtualAddress()+1792;giParameters.SizeInBytes=256;
            device->CreateConstantBufferView(&giParameters,Cpu(a->privateTable,1)); // private b5
            if(environment->gi){
                device->CopyDescriptorsSimple(1,Cpu(a->privateTable,0),environment->table.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // b4 volume transforms
                device->CopyDescriptorsSimple(9,Cpu(a->privateTable,26),environment->table.At(21),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // t21..29
            }
        }
        D3D12_SHADER_RESOURCE_VIEW_DESC as{};as.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        as.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;as.RaytracingAccelerationStructure.Location=plan.selection.tlas;
        device->CreateShaderResourceView(nullptr,&as,Cpu(a->privateTable,40));
        device->CopyDescriptorsSimple(1,Cpu(a->privateTable,48),(surfaceReplacement?frame->raw:frame->output)->Uav(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(surfaceReplacement){
            if(!PTCommandBindings::tracker.GraphicsForRestore(list,nativeRoots,a->graphicsRestore)){Reject("surface_graphics_restore");return;}
            a->history=historyOutput;a->reconstruction=std::make_shared<PTSurfaceComposition::ReconstructionPass>();
            if(!a->reconstruction->Initialize(device.Get(),reconstructionPipeline,view,a->history?&historyView:nullptr,
                frame->raw->Texture(),a->history?a->history->Texture():nullptr,{a->primary.At(5),a->primary.At(6),a->primary.At(8),a->primary.At(12),a->primary.At(13)},*materialKeys,historyMaterials.Get(),historySignals,control_rr::rrPTInputRoute.Fresh(view.engineFrame))){
                const auto pool=PTSurfaceComposition::SignalStorage::Statistics();
                static UINT64 failures=0;if(++failures<=16||failures%60==0)
                    Log("PT_PRIMARY_RECONSTRUCTION_FAILED frame=%u stage=%s hr=0x%08lX signal_resident_bytes=%llu signal_idle_bytes=%llu signal_budget_bytes=%llu budget_failures=%llu device_failures=%llu",view.frame,a->reconstruction->failure,static_cast<unsigned long>(a->reconstruction->failureHr),pool.residentBytes,pool.idleBytes,PTSurfaceComposition::SignalStorage::MaximumBytes,pool.budgetFailures,pool.deviceFailures);
                Reject("surface_reconstruction_setup");return;
            }
        }
        if(surfaceReplacement){
            // Reuse reconstruction's two 32-byte/pixel buffers before filtering.
            // They retain the same fence-owned lifetime; no new image allocation.
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_TYPELESS;uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            uav.Buffer.NumElements=view.width*view.height*8;uav.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
            device->CreateUnorderedAccessView(a->reconstruction->scratch->resource.Get(),nullptr,&uav,Cpu(a->privateTable,46)); // u5
        }
        if(nextTicket==UINT64_MAX){Reject("ticket_exhausted");return;}a->ticket=nextTicket++;
        if(!PTSceneLive::owner.Scenes().Mark(list,a->ticket)){Reject("private_scene_cut");return;}
        // Attach every owner before recording the first GPU reference. Even a
        // failed encoder retains these through the list's native submission.
        recording->producers.push_back(frame);frame->assets=a;frame->producerRecorded=true;
        const bool encoded=surfaceReplacement?
            (frame->raw->RecordInline(list,a.get(),&Encode,a)&&frame->output->RecordInline(list,a.get(),&Reconstruct,a)):
            frame->output->RecordInline(list,a.get(),&Encode,a);
        if(!encoded){frame->output->Invalidate(context.queue);Reject("inline_record");return;}
        if(surfaceReplacement){static UINT64 reconstructed=0;if(++reconstructed<=8||reconstructed%240==0)
            Log("PT_PRIMARY_RECONSTRUCTION frame=%u history=%u previous_frame=%u dimensions=%ux%u binding=root_buffer surface_descriptor_allocations=0 fresh_rr_input=%u engine_frame=%llu",view.frame,unsigned(a->reconstruction->historyEnabled),a->history?historyView.frame:0,view.width,view.height,unsigned(a->reconstruction->freshInput),view.engineFrame);}
        QueryPerformanceCounter(&recordDone);
        a->cpuCosts={CpuMilliseconds(prepareStart,recordDone),CpuMilliseconds(leaseStart,leaseDone),
            CpuMilliseconds(sbtStart,cameraStart),CpuMilliseconds(cameraStart,metricsStart),
            CpuMilliseconds(metricsStart,encodeStart),CpuMilliseconds(encodeStart,recordDone)};
        std::copy(preparationCosts.begin(),preparationCosts.end(),a->cpuCosts.begin()+6);
        pendingMetrics.push_back(frame);
        static UINT64 cpuRecorded=0;if(++cpuRecorded<=8||cpuRecorded%240==0)
            Log("PT_PRIMARY_PREPARE_CPU frame=%u total_ms=%.6f find_ready_ms=%.6f lease_ms=%.6f descriptor_unlocked_ms=%.6f descriptor_relock_ms=%.6f",view.frame,CpuMilliseconds(prepareStart,recordDone),CpuMilliseconds(prepareStart,readyDone),CpuMilliseconds(leaseStart,leaseDone),CpuMilliseconds(descriptorReleased,descriptorReacquire),CpuMilliseconds(descriptorReacquire,descriptorAcquired));
        static UINT64 recorded=0;if(++recorded<=8||recorded%240==0)Log("PT_PRIMARY_RECORDED count=%llu frame=%u dimensions=%ux%u hits=4 spp=1 output=%s",recorded,view.frame,view.width,view.height,outputName);
    }catch(...){Reject("dispatch_allocation");}
}
// Changing roots invalidates all root arguments. Replay every captured native
// argument both on entry and on exit; heap and IA state never change.
static void BindGraphics(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root,const Bindings& bindings){
    list->SetGraphicsRootSignature(root);
    for(UINT n=0;n<bindings.arguments.size();++n){const auto& a=bindings.arguments[n];
        if(a.kind==PTCommandBindings::ArgumentKind::table)list->SetGraphicsRootDescriptorTable(n,{a.value});
        else if(a.kind==PTCommandBindings::ArgumentKind::constant)list->SetGraphicsRootConstantBufferView(n,a.value);
    }
}
struct Restore {ID3D12GraphicsCommandList* list=nullptr;Bindings bindings;};
static thread_local Restore restore;
static void Drawing(void*,ID3D12GraphicsCommandList* list) noexcept {
    if(!active||ptReflectionUserState.load(std::memory_order_acquire)!=1)return;
    try {
        if(restore.list)return;
        // Filter non-surface PSOs before scene/frame locks on every native draw.
        const auto pso=PTCommandBindings::tracker.PipelineFor(list);
        auto pipeline=surfacePipelines.Find(pso.Get());if(!pipeline)return;
        if(!PTSceneLive::owner.Scenes().Healthy())return;
        // A frame may be reserved before its producer is recorded. Its output
        // starts at alpha zero; only ordered submission can authorize PT.
        std::lock_guard<std::recursive_mutex> ordered(PTSceneQueues::queueMutex);PTSceneQueues::Scope privateQueues;
        std::lock_guard<std::recursive_mutex> guard(mutex);
        Bindings bindings;
        PTCommandBindings::ReadFailure bindingFailure;
        if(!PTCommandBindings::tracker.Read(list,nativeRoots,true,pipeline->RootRequirements(),bindings,&bindingFailure,true)){
            static UINT64 failures[19]{};const UINT bucket=bindingFailure.parameter<18?bindingFailure.parameter:18;
            const auto count=++failures[bucket];
            if(count<=8||count%600==0)Log("PT_PRIMARY_SURFACE_BINDING_FAILURE reason=%s parameter=%u address=%llx required_count=%u heap_count=%u count=%llu list=%p argument_origin=%s previous_address=%llx shader=%s",bindingFailure.reason,bindingFailure.parameter,bindingFailure.address,bindingFailure.requiredCount,bindingFailure.heapCount,count,list,bindingFailure.argumentOrigin,bindingFailure.previousAddress,pipeline->ShaderIdentity());
            Reject("surface_bindings");return;
        }
        if(!bindings.layout->Matches(pipeline->Root())){Reject("surface_root_mismatch");return;}
        // Match the draw's actual CBV to a retained producer before consulting
        // allocator TLS or cloning descriptors. Worker TLS may already describe
        // another view; it is not the authority for this recorded surface.
        View view;std::array<unsigned char,1136> raw{};UINT64 engineFrame=0;DWORD fault=0;
        if(!ReadEngineFrameSafe(&engineFrame,&fault)){Reject("surface_engine_frame");return;}
        if(!PTNativeView::uploads.Read(bindings.arguments[12].value,raw.data(),raw.size())&&
           !PTSceneLive::owner.Resources().ReadUpload(bindings.arguments[12].value,raw.data(),raw.size())){Reject("surface_view_upload");return;}
        UINT64 vertexViewAddress=0;
        if(pipeline->VertexCamera()){
            vertexViewAddress=bindings.arguments[8].value;std::array<unsigned char,1136> vertex{};
            if(!PTNativeView::uploads.Read(vertexViewAddress,vertex.data(),vertex.size())&&
               !PTSceneLive::owner.Resources().ReadUpload(vertexViewAddress,vertex.data(),vertex.size())){Reject("surface_vertex_view_upload");return;}
            std::memcpy(raw.data()+96,vertex.data()+96,64);
        }
        std::shared_ptr<Frame> matched;bool pendingCamera=false;
        for(const auto& weak:frames){const auto f=weak.lock();
            if(!f||f->cancelled||f->view.engineFrame!=engineFrame||!f->view.MatchesSurface(raw))continue;
            if(matched){Reject("surface_ambiguous_frame");return;}matched=f;
        }
        if(!matched){
            // Native workers may record a surface before the opaque ray callback
            // reserves its output. Retain this worker's stable complete camera
            // as a candidate until its actual CBV passes submission validation.
            View early;early.bytes=raw;
            if(!pinnedQueue||!viewProviders.Read(early,PTNativeView::Pass::surface,true)||
               early.engineFrame!=engineFrame){
                static UINT64 failures=0;const auto count=++failures;
                if(count<=8||count%600==0)Log("PT_PRIMARY_SURFACE_CAMERA_FAILURE reason=%s offset=%u engine_frame=%llu provider_engine_frame=%llu queue_ready=%u count=%llu",early.failure,early.failureOffset,engineFrame,early.engineFrame,unsigned(bool(pinnedQueue)),count);
                Reject("surface_early_camera_unavailable");return;
            }
            pendingCamera=!early.MatchesSurface(raw);
            // A stable TLS snapshot only selects a candidate. Reuse an exact
            // same-frame reservation if present; a different view is rejected.
            bool otherFrame=false;
            for(const auto& weak:frames){const auto f=weak.lock();
                if(f&&f->view.engineFrame==engineFrame&&!f->view.Same(early)){otherFrame=true;break;}}
            if(otherFrame){Reject("surface_other_view_or_cancelled_frame");return;}
            ComPtr<ID3D12CommandQueue> queue;
            if(FAILED(pinnedQueue.As(&queue))){Reject("surface_early_queue_unavailable");return;}
            PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
            matched=FrameFor(early,queue.Get());
            if(!matched||matched->cancelled){Reject("surface_early_reservation_failed");return;}
            static UINT64 earlyReservations=0;
            if(++earlyReservations<=8||earlyReservations%240==0)
                Log("PT_PRIMARY_SURFACE_EARLY_RESERVED frame=%u engine_frame=%llu count=%llu producer_required=1",early.frame,early.engineFrame,earlyReservations);
        }
        view=matched->view;view.pass=PTNativeView::Pass::surface;
        // Never bless the possibly stale recording-time bytes. The final CBV
        // must match the producer camera at submission, including pending draws.
        view.bound=view.bytes;
        const auto base=bindings.resources->GetGPUDescriptorHandleForHeapStart().ptr;
        const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto address=bindings.arguments[1].value;
        if(!step||address<base||(address-base)%step||
           (address-base)/step+49>bindings.resources->GetDesc().NumDescriptors||
           !matched->output->Address()||!PTSceneQueues::WatchList(list)){Reject("surface_table_or_output");return;}
        const UINT first=static_cast<UINT>((address-base)/step);
        auto* recording=RecordingFor(list);if(!recording||recording->submitted||recording->draws.size()>=4096){Reject("surface_recording_unavailable");return;}
        // View matching and upload reads do not touch descriptor values.
        PTTableMirror::Scope descriptorLock(true);
        std::shared_ptr<Draw> draw;
        for(const auto& d:recording->draws)if(d->pipeline==pipeline&&d->viewAddress==bindings.arguments[12].value&&d->vertexViewAddress==vertexViewAddress&&
            d->original.native.Get()==bindings.resources.Get()&&d->original.first==first&&d->frame->view.Same(view)){draw=d;break;}
        if(!draw){
            auto frame=matched;
            draw=std::make_shared<Draw>();draw->frame=frame;draw->pipeline=pipeline;draw->viewAddress=bindings.arguments[12].value;draw->boundView=view;draw->vertexViewAddress=vertexViewAddress;draw->pendingCamera=pendingCamera;
            if(!PTTableMirror::bank.Read(bindings.resources.Get(),first,pipeline->Required(),draw->original)){Reject("surface_descriptor_snapshot");return;}
            // Proven consumers may share their recording/frame guard. A pending
            // upload gets its own guard: a wrong candidate must not disable
            // already-matching draws in this same command list.
            if(!pendingCamera)for(const auto& d:recording->draws)
                if(d->frame==frame&&!d->pendingCamera){draw->consumer=d->consumer;break;}
            if(!draw->consumer){
                if(FAILED(Output::Create(frame->queue.Get(),1,1,draw->consumer,true,true))||
                   !draw->consumer->EnableConsumer(frame->queue.Get())){Reject("surface_consumer_guard");return;}
            }
            recording->draws.push_back(draw);
            static UINT64 composed=0;if(++composed<=8||composed%600==0)Log("PT_PRIMARY_SURFACE_RECORDED frame=%u count=%llu binding=root_buffer descriptor_copies=0",frame->view.frame,composed);
        }
        restore={list,bindings};
        BindGraphics(list,pipeline->ComposedRoot(),bindings);
        if(bindings.requiresSamplerReplay){
            static UINT64 replays=0;const auto count=++replays;
            if(count<=8||count%600==0)Log("PT_PRIMARY_SURFACE_SAMPLER_REPLAY frame=%u parameter=16 address=%llx count=%llu list=%p shader=%s same_sampler_heap=1 explicit_rebind=1",draw->frame->view.frame,bindings.arguments[16].value,count,list,pipeline->ShaderIdentity());
        }
        list->SetGraphicsRootShaderResourceView(18,draw->frame->output->Address());
        list->SetGraphicsRootShaderResourceView(20,draw->consumer->Address());
        const UINT extent[]={draw->frame->view.width,draw->frame->view.height};list->SetGraphicsRoot32BitConstants(19,2,extent,0);
        list->SetPipelineState(pipeline->Composed());
    }catch(...){Reject("surface_allocation");}
}
static void Drawn(void*,ID3D12GraphicsCommandList* list) noexcept {
    if(restore.list!=list)return;BindGraphics(list,restore.bindings.layout->Root(),restore.bindings);list->SetPipelineState(restore.bindings.pipeline.Get());restore={};
}
static void Submitting(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);auto key=Identity(queue);
        if(TraceSchedule())Log("PT_PRIMARY_SCHEDULE operation=execute queue=%p type=%u lists=%u",queue,unsigned(queue->GetDesc().Type),count);
        if(sceneUseValue&&key&&key.Get()!=pinnedQueue.Get()){
            auto& waited=sceneWaits[key.Get()];
            if(waited.second<sceneUseValue){
                waited.first=key;
                if(FAILED(queue->Wait(sceneUseFence.Get(),sceneUseValue))||
                   !PTSceneLive::owner.Scenes().Wait(queue,sceneUseFence.Get(),sceneUseValue)){
                    PTSceneLive::owner.Scenes().Invalidate();Reject("scene_use_wait");
                }else waited.second=sceneUseValue;
            }
        }
        handoff.Submitting(count,lists);
    }catch(...){PTSceneLive::owner.Scenes().Invalidate();}
}
static bool SameLights(const Assets& a){
    std::array<unsigned char,416> current{};
    if(!PTNativeView::uploads.Read(a.bindings.arguments[4].value,current.data(),current.size())&&
       !PTSceneLive::owner.Resources().ReadUpload(a.bindings.arguments[4].value,current.data(),current.size()))return false;
    // Actual b2 reads of both native primary shader variants, as decoded from
    // their instruction streams. Unrequested point/spot/ambient lanes are absent.
    return !std::memcmp(current.data()+160,a.boundLights.data()+160,12)&&
        !std::memcmp(current.data()+192,a.boundLights.data()+192,12)&&
        !std::memcmp(current.data()+216,a.boundLights.data()+216,16)&&
        !std::memcmp(current.data()+264,a.boundLights.data()+264,4);
}
static bool SameEnvironmentScale(const Environment& e){
    if(!surfaceReplacement)return true;float scale=0;
    if(e.gi){
        std::array<unsigned char,48> current{};float giScale=0;
        if((!PTNativeView::uploads.Read(e.giAddress,current.data(),current.size())&&
            !PTSceneLive::owner.Resources().ReadUpload(e.giAddress,current.data(),current.size()))||
           (!PTNativeView::uploads.Read(e.giScaleAddress,&giScale,4)&&
            !PTSceneLive::owner.Resources().ReadUpload(e.giScaleAddress,&giScale,4))||
           std::memcmp(current.data(),e.giConstants.data(),48)||std::memcmp(&giScale,e.giConstants.data()+48,4))return false;
    }
    return (PTNativeView::uploads.Read(e.scaleAddress,&scale,4)||
        PTSceneLive::owner.Resources().ReadUpload(e.scaleAddress,&scale,4))&&
        !std::memcmp(&scale,&e.scale,4);
}
static bool InputsUnchanged(const std::shared_ptr<Frame>& frame){
    const auto& a=frame->assets;
    return !frame->cancelled&&ptReflectionUserState.load()==1u&&a&&a->environment->submitted&&
        SameView(a->bindings.arguments[2].value,a->boundView)&&SameLights(*a)&&SameEnvironmentScale(*a->environment)&&PTTableMirror::bank.Unchanged(a->primary)&&
        PTTableMirror::bank.Unchanged(a->environment->table);
}
static void CancelFrame(const std::shared_ptr<Frame>& frame,ID3D12CommandQueue* queue){
    frame->cancelled=true;frame->deferred=false;
    if(frame->assets&&!frame->assets->metrics.SetAdmission(queue,false))PTSceneLive::owner.Scenes().Invalidate();
    frame->output->Invalidate(queue);
    if(frame->producerSubmitted&&frame->assets)PTSceneLive::owner.Scenes().Forget(frame->assets->ticket);
}
static bool AdmitFrame(const std::shared_ptr<Frame>& frame,ID3D12CommandQueue* queue,bool late){
    if(!frame->output->AdmitInline()||!frame->assets->metrics.SetAdmission(queue,true,frame->assets->probe))return false;
    frame->armed=true;frame->deferred=false;
    if(PTSurfaceHistory::Publish(frame,surfaceReplacement,historyOutput,historyView)){historyMaterials=frame->assets->reconstruction->materials;historySignals=frame->assets->reconstruction->signals;}
    if(late)sceneUseValue=(std::max)(sceneUseValue,frame->sceneUse);
    static UINT64 submitted=0;if(++submitted<=8||submitted%240==0)
        Log("PT_PRIMARY_SUBMITTED count=%llu frame=%u scene=exact admission_word=armed late_signal=%u",submitted,frame->view.frame,unsigned(late));
    return true;
}
// Called under queueMutex after prospective Signal metadata, but BEFORE native
// Signal. The exact unsatisfied fence must still block this graphics consumer.
static void Signalling(void*,ID3D12CommandQueue* queue,ID3D12Fence* fence,UINT64 value) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        if(pendingMetrics.empty())return;
        for(const auto& frame:pendingMetrics){
            if(!frame->deferred||!frame->producerSubmitted||!frame->assets)continue;
            const auto& a=frame->assets;const auto blocked=frame->wait;
            if(value<blocked.graph.value||!PTSceneLive::owner.Scenes().MatchesFence(blocked.graph.fence,fence))continue;
            // Most signals cannot release a deferred PT frame. They must not
            // take the global descriptor gate merely to inspect queue metadata.
            PTTableMirror::Scope descriptorLock(true);
            const bool release=fence->GetCompletedValue()<blocked.graph.value&&queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_COPY;
            const char* reason="late_submission_inputs_changed";
            LARGE_INTEGER validateStart{},validateEnd{};QueryPerformanceCounter(&validateStart);
            const auto status=InputsUnchanged(frame)?handoff.Revalidate(a->ticket,a->engineFrame,frame->view.width,frame->view.height,a->proof,&reason):PTSceneHandoff::Status::rejected;
            QueryPerformanceCounter(&validateEnd);
            static UINT64 validated=0;if(++validated<=8||validated%240==0)
                Log("PT_PRIMARY_VALIDATE_CPU frame=%u ms=%.6f proof=%s",frame->view.frame,CpuMilliseconds(validateStart,validateEnd),reason);
            if(status==PTSceneHandoff::Status::pending){frame->wait=PTSceneLive::owner.Scenes().PendingWait(a->ticket);
                static UINT64 pending=0;if(++pending<=8||pending%240==0)
                    Log("PT_PRIMARY_LATE_PENDING ticket=%llu reason=%s fence=%llu required=%llu",a->ticket,reason,frame->wait.graph.fence,frame->wait.graph.value);
                continue;}
            if(status!=PTSceneHandoff::Status::prepared||!release||!frame->sceneUse||!AdmitFrame(frame,queue,true)){
                // The flag is still zero: do not enqueue work on the blocked
                // direct queue from this producer callback.
                frame->deferred=false;frame->cancelled=true;
                static UINT64 rejected=0;if(++rejected<=16||rejected%600==0)
                    Log("PT_PRIMARY_LATE_REJECT ticket=%llu reason=%s release=%u",a->ticket,reason,unsigned(release));
            }
            PTSceneLive::owner.Scenes().Forget(a->ticket);
        }
    }catch(...){PTSceneLive::owner.Scenes().Invalidate();Reject("late_admission_exception");}
}
static void Selected(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists,bool admitted) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        if(recordings.empty()&&environments.empty())return;
        if(!lists||count>256)return;const auto queueIdentity=Identity(queue);
        for(UINT n=0;n<count;++n){const auto key=Identity(lists[n]);
            for(auto& e:environments)if(e->list.Get()==key.Get()&&e->queue.Get()==queueIdentity.Get())e->submitted=true;
            auto found=recordings.find(key.Get());if(found==recordings.end())continue;auto& recording=found->second;
            if(recording.producers.empty()&&recording.draws.empty()&&recording.guides.empty())continue;
            // Environment-only and unrelated lists require no descriptor access.
            // Hold the gate across all checks and admission for an actual consumer.
            PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
            for(const auto& frame:recording.producers){const auto& a=frame->assets;
                const char* admissionReason="submission_inputs_changed";
                const bool inputs=admitted&&!recording.submitted&&!frame->producerSubmitted&&
                    queueIdentity.Get()==pinnedQueue.Get()&&InputsUnchanged(frame);
                const auto status=inputs?handoff.Revalidate(a->ticket,a->engineFrame,frame->view.width,frame->view.height,a->proof,&admissionReason):PTSceneHandoff::Status::rejected;
                if(status==PTSceneHandoff::Status::pending){
                    frame->wait=PTSceneLive::owner.Scenes().PendingWait(a->ticket);
                    if(frame->wait.identity&&frame->wait.polled&&frame->wait.actual<frame->wait.graph.value){
                        frame->deferred=true;
                        static UINT64 deferred=0;if(++deferred<=8||deferred%240==0)
                            Log("PT_PRIMARY_DEFERRED ticket=%llu fence=%p required=%llu completed=%llu",a->ticket,frame->wait.identity,frame->wait.graph.value,frame->wait.actual);
                        continue;
                    }
                }
                if(status!=PTSceneHandoff::Status::prepared||!AdmitFrame(frame,queue,false)){
                    if(status==PTSceneHandoff::Status::prepared)admissionReason="submission_arm_failed";
                    static UINT64 failures=0;if(++failures<=16||failures%600==0)
                        Log("PT_PRIMARY_ADMISSION ticket=%llu reason=%s",a?a->ticket:0,admissionReason);
                    CancelFrame(frame,queue);Reject(admissionReason);
                }
            }
            for(const auto& guide:recording.guides){
                const auto& f=guide->frame;
                const unsigned reason=PTGuideAdmission::Reject(f->producerRecorded,f->armed,f->deferred,f->cancelled,
                    recording.submitted||guide->selected,queueIdentity.Get()==pinnedQueue.Get(),admitted);
                guide->selected=true;
                const bool okay=reason?guide->guard->InvalidateBefore(queue):guide->guard->EnableConsumer(queue);
                if(!okay){PTSceneLive::owner.Scenes().Invalidate();Reject("rr_guide_guard_failed");}
                static UINT64 checks=0;if(++checks<=8||checks%240==0||reason)
                    Log("RR_PT_GUIDE_SUBMIT frame=%llu accepted=%u reasons=%u guard_ok=%u producer_submitted=%u deferred=%u batch_index=%u",f->view.engineFrame,unsigned(reason==0&&okay),reason,unsigned(okay),unsigned(f->producerSubmitted),unsigned(f->deferred),n);
            }
            for(const auto& draw:recording.draws){
                const bool changed=!PTTableMirror::bank.Unchanged(draw->original);
                // CPU recording may precede the producer, GPU execution may not.
                // Producer lists earlier in this batch have armed/deferred above.
                const bool orderedProducer=draw->frame->producerRecorded&&(draw->frame->armed||draw->frame->deferred)&&!draw->frame->cancelled;
                auto pixelView=draw->boundView,vertexView=draw->boundView;
                if(draw->vertexViewAddress){pixelView.pass=PTNativeView::Pass::surfacePixel;vertexView.pass=PTNativeView::Pass::surfaceVertex;}
                const bool cameraMatches=SameView(draw->viewAddress,pixelView)&&
                    (!draw->vertexViewAddress||SameView(draw->vertexViewAddress,vertexView));
                const UINT reason=PTNativeView::SurfaceAdmission(orderedProducer,recording.submitted,
                    queueIdentity.Get()==pinnedQueue.Get(),cameraMatches,!changed,admitted);
                if(draw->pendingCamera){
                    static UINT64 accepted=0,rejected=0;const auto checks=reason?++rejected:++accepted;
                    if(checks<=8||checks%600==0)Log("PT_PRIMARY_SURFACE_CAMERA_RESOLVED frame=%u accepted=%u reasons=%u count=%llu final_cbv_checked=1",draw->frame->view.frame,unsigned(reason==0),reason,checks);
                }
                if(reason){
                    // Producer invalidation is handled above using scene/input proofs.
                    // Consumer-only failure selects native for this recording group;
                    // clearing shared output here used to invalidate ALL consumers.
                    if(!draw->consumer->Invalidate(queue)){
                        CancelFrame(draw->frame,queue);PTSceneLive::owner.Scenes().Invalidate();Reject("surface_consumer_guard_failed");continue;
                    }
                    static UINT64 rejected=0;static UINT lastRejectedFrame=0;
                    const bool newRejectedFrame=lastRejectedFrame!=draw->frame->view.frame;
                    if(newRejectedFrame){lastRejectedFrame=draw->frame->view.frame;++rejected;}
                    if(newRejectedFrame&&(rejected<=32||rejected%60==0))
                        Log("PT_PRIMARY_CONSUMER_REJECT frame=%u reasons=%u ordered=%u replay=%u queue=%u view=%u descriptors=%u producer_recorded=%u producer_submitted=%u armed=%u deferred=%u cancelled=%u batch_index=%u batch_lists=%u scope=recording_group shared_output_preserved=1",draw->frame->view.frame,reason,unsigned((reason&1)==0),unsigned((reason&2)!=0),unsigned((reason&4)==0),unsigned((reason&8)==0),unsigned((reason&16)==0),unsigned(draw->frame->producerRecorded),unsigned(draw->frame->producerSubmitted),unsigned(draw->frame->armed),unsigned(draw->frame->deferred),unsigned(draw->frame->cancelled),n,count);
                }
            }
        }
    }catch(...){PTSceneLive::owner.Scenes().Invalidate();Reject("submission_exception");}
}
static void Submitted(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists,bool) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);if(!lists||count>256)return;
        std::vector<Output*> signalled;bool traversed=false;std::vector<std::shared_ptr<Frame>> uses;
        const auto signal=[&](const std::shared_ptr<Output>& output){
            if(std::find(signalled.begin(),signalled.end(),output.get())!=signalled.end())return;
            output->NativeSubmitted(queue);signalled.push_back(output.get());
        };
        for(UINT n=0;n<count;++n){auto key=Identity(lists[n]);auto found=recordings.find(key.Get());if(found==recordings.end())continue;
            auto& r=found->second;for(const auto& f:r.producers){f->producerSubmitted=true;traversed=traversed||f->armed;
                if((f->armed||f->deferred)&&!f->sceneUse)uses.push_back(f);
                signal(f->output);
                if(f->raw)f->raw->NativeSubmitted(queue);
                if(f->assets&&f->assets->history)f->assets->history->NativeSubmitted(queue);
                if(PTSurfaceHistory::Publish(f,surfaceReplacement,historyOutput,historyView)){historyMaterials=f->assets->reconstruction->materials;historySignals=f->assets->reconstruction->signals;}
                if(f->assets&&!f->deferred)PTSceneLive::owner.Scenes().Forget(f->assets->ticket);}
            for(const auto& d:r.draws){signal(d->frame->output);signal(d->consumer);}
            for(const auto& g:r.guides){signal(g->guard);signal(g->frame->output);signal(g->frame->raw);}r.submitted=true;
        }
        if(!uses.empty()){
            const auto value=sceneUseNext+1;
            if(sceneUseNext>=UINT64_MAX-1||FAILED(queue->Signal(sceneUseFence.Get(),value))){
                PTSceneLive::owner.Scenes().Invalidate();Reject("scene_use_signal");
            }else {
                sceneUseNext=value;for(const auto& f:uses)f->sceneUse=value;
                // Reserve retirement now, but protect later queues only AFTER
                // admission. Waiting here would deadlock the late copy producer.
                if(traversed)sceneUseValue=value;
                if(!PTSceneLive::owner.Scenes().Signal(queue,sceneUseFence.Get(),value))PTSceneLive::owner.Scenes().Invalidate();
            }
        }
        static UINT64 submissions=0;const auto sequence=++submissions;
        if(sequence<=4||sequence%240==0){const auto t=PTSceneLive::owner.Scenes().Statistics();
            const auto pool=PTSceneLive::owner.Resources().GeometryStatistics();
            const auto owners=PTSceneLive::owner.Resources().Statistics();
            const auto descriptorStats=PTSceneLive::owner.DescriptorStatistics();
            Log("PT_PRIMARY_DESCRIPTOR_STATS healthy=%u failure=%s entries=%llu writes=%llu as_writes=%llu copies=%llu",unsigned(descriptorStats.healthy),descriptorStats.failure,descriptorStats.entries,descriptorStats.writes,descriptorStats.asWrites,descriptorStats.copies);
            Log("PT_PRIMARY_OWNERS retained=%llu rejected=%llu budget_rejected=%llu peak_bytes=%llu untracked_destinations=%llu destination_tombstones=%llu last_untracked_destination=%llx",owners.retained,owners.rejected,owners.budget,owners.peakBytes,t.untrackedDestinations,t.destinationTombstones,t.lastUntrackedDestination);
            Log("PT_PRIMARY_CAPTURE healthy=%u failure=%s skipped_invalid=%llu copy_failures=%llu copy_failure=%s pooled_idle_bytes=%llu buffer_allocations=%llu buffer_reuses=%llu pool_evictions=%llu history_versions=%llu history_rows=%llu history_nodes=%llu copy_bytes=%llu off_updates_preserved=%llu completed_waits=%llu",unsigned(t.healthy),t.failure,t.skippedInvalid,t.copyFailures,PTSceneLive::owner.Resources().GeometryFailure(),pool.bytes,pool.allocations,pool.reuses,pool.evictions,t.historyVersions,t.historyRows,t.historyNodes,t.copyBytes,t.offUpdatesPreserved,t.completedWaits);
            Log("PT_PRIMARY_RETIREMENT fences=%llu signals=%llu retired=%llu pending=%llu failures=%llu",t.retirement.fences,t.retirement.signals,t.retirement.retired,t.retirement.pending,t.retirement.failures);
            Log("PT_PRIMARY_COPY_TOTAL copied_bytes=%llu whole_index_bytes=%llu",owners.geometryCopyBytes,owners.geometryWholeIndexBytes);
            Log("PT_PRIMARY_CPU submissions=%llu builds=%llu build_total_ms=%llu build_max_ms=%llu checkpoints=%llu checkpoint_total_ms=%llu checkpoint_max_ms=%llu blocking_checkpoint_waits=0",sequence,t.builds.calls,t.builds.totalMs,t.builds.maxMs,t.checkpoints.calls,t.checkpoints.totalMs,t.checkpoints.maxMs);
            const auto mirror=PTTableMirror::bank.Statistics();
            Log("PT_PRIMARY_MIRROR_COPY single_calls=%llu batch_calls=%llu descriptors=%llu redundant_skipped=%llu",mirror.single,mirror.batch,mirror.descriptors,mirror.redundant);
            Log("PT_PRIMARY_PROGRESS signals=%llu skipped=%llu interval=64",t.progressSignals,t.progressSkipped);
            LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);const double ms=1000.0/static_cast<double>(frequency.QuadPart);
            for(unsigned kind=0;kind<static_cast<unsigned>(PTHotpathTiming::Kind::count);++kind){
                const auto& hot=PTHotpathTiming::counters[kind];
                Log("PT_PRIMARY_HOT_CPU kind=%s samples=%llu sampled_ms=%.6f sampled_max_ms=%.6f interval=64 cumulative=1 scope=%s",PTHotpathTiming::Name(kind),
                    hot.samples.load(std::memory_order_relaxed),hot.ticks.load(std::memory_order_relaxed)*ms,hot.maximum.load(std::memory_order_relaxed)*ms,
                    "inclusive_wall_cross_thread_not_frame_time");
            }
            for(unsigned on=0;on<2;++on)for(unsigned execute=0;execute<2;++execute){const auto& q=t.queues[on][execute];
                Log("PT_PRIMARY_QUEUE_CPU pt=%u operation=%s calls=%llu lock_ms=%.6f observer_ms=%.6f native_ms=%.6f observer_max_ms=%.6f native_max_ms=%.6f cumulative=1",on,execute?"execute":"fence",q.calls,q.lockTicks*ms,q.observerTicks*ms,q.nativeTicks*ms,q.maxObserverTicks*ms,q.maxNativeTicks*ms);
            }
        }
        if(sequence%240==0){
            Log("PT_PRIMARY_ROUTE bindings_healthy=%u jobs=%llu job_off=%llu job_unknown_pso=%llu opaque=%llu compute=%llu compute_off=%llu no_pso=%llu other_pso=%llu primary=%llu environment=%llu",
                unsigned(PTCommandBindings::tracker.Healthy()),jobCallbacks.load(),jobOff.load(),jobUnknownPipeline.load(),opaqueAccepted.load(),computeCallbacks.load(),computeOff.load(),computeNoPipeline.load(),computeOtherPipeline.load(),primarySeen.load(),environmentSeen.load());
            PTNativeJobRuntime::Diagnostics([](const char* format,auto... args){Log(format,args...);});
        }
        PollMetrics();Output::Poll();
    }catch(...){Reject("retirement_exception");}
}
static void Reset(void*,ID3D12GraphicsCommandList* list) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);handoff.Reset(list);auto key=Identity(list);auto found=recordings.find(key.Get());
        if(found!=recordings.end()){
            for(const auto& f:found->second.producers){
                if(f->assets&&!f->deferred)PTSceneLive::owner.Scenes().Forget(f->assets->ticket);
                if(!f->producerSubmitted)pendingMetrics.erase(std::remove(pendingMetrics.begin(),pendingMetrics.end(),f),pendingMetrics.end());
            }
            recordings.erase(found);
        }
        PollMetrics();Output::Poll();
    }catch(...){Reject("reset_exception");}
}
static bool RenderingRequested(void*) noexcept {return ptReflectionUserState.load(std::memory_order_acquire)==1;}
static const PTSceneQueues::Consumer queueConsumer{&handoff,&Submitting,&Selected,&Submitted,&Reset,&Signalling,&FenceObserved,&RenderingRequested};
static void PipelineBound(void*,ID3D12GraphicsCommandList*,ID3D12PipelineState* pipeline) noexcept {
    if(!active||ptReflectionUserState.load(std::memory_order_acquire)!=1)return;
    try{PTPrimaryPipelineProbe::Used(pipeline,0,0,0,false);}
    catch(...){Log("PT_PRIMARY_PSO_PROBE_FAILED stage=binding");}
}
// Published once in Start, before native list hooks are installed. PT selection
// is restart-only: an off session does not install graphics observation hooks.
static PTCommandBindings::Consumer commandConsumer{};
static void MirrorModeChanged(bool) noexcept {
    try {std::lock_guard<PTTableMirror::DescriptorGate> transition(PTTableMirror::gate);
        const bool requested=PTReflectionUserRequested();
        PTTableMirror::bank.SetCopying(requested);
        Log("PT_PRIMARY_MIRROR_MODE copying=%u fresh_tables_required=1 geometry_retained=1",unsigned(requested));
    }catch(...){PTSceneLive::owner.Scenes().Invalidate();}
}
static bool Start(){
    if(active||!device||!enabled||!PTNativeJobRuntime::workerHooksReady||!PTNativeJobRuntime::dispatchHookReady)return false;
    if(!viewProviders.Initialize(verifiedRenderer,verifiedD3d)){Log("PT_PRIMARY_PROVIDER_INIT_FAILED");return false;}
    char sha[65]{};bool loaded=false;
    if(!PTReflectionLoadSidecar(surfaceReplacement?L"pt_primary_surface.dxil":L"pt_primary_path.dxil","PRIMARY_PATH",library,sha,loaded))return false;
    if(surfaceReplacement){
        std::vector<unsigned char> filter;char filterSha[65]{};bool found=false;
        if(!PTReflectionLoadSidecar(L"pt_emitter_build.dxil","EMITTER_BUILD",emitterLibrary,filterSha,found))return false;
        if(!PTReflectionLoadSidecar(L"pt_surface_reconstruct.dxil","SURFACE_RECONSTRUCTION",filter,filterSha,found))return false;
        reconstructionPipeline=std::make_shared<PTSurfaceComposition::ReconstructionPipeline>();
        if(FAILED(reconstructionPipeline->Initialize(device.Get(),filter.data(),filter.size())))return false;
    }
    const bool compose=surfaceReplacement&&PTReflectionUserRequested();
    commandConsumer={&handoff,&Dispatched,compose?&Drawing:nullptr,compose?&Drawn:nullptr,&PipelineBound,!compose};
    if(surfaceReplacement)Log("PT_PRIMARY_SURFACE_MODE replacement=1 binding=root_buffer reconstruction=spatial_temporal native_passes_retained=1 unsupported_materials=native graphics_tracking=%u",unsigned(compose));
#if defined(CONTROL_PT_RECOVERY_BUILD)
    Log("PT_PRIMARY_RECOVERY baseline=M12R66 surface_replacement=0 pt_indirect=1 graphics_tracking=0 reason=r67_r68_surface_regression");
#endif
    pendingMetrics.reserve(8);
    PTCommandBindings::Mute privateRecording;PTTableMirror::Mute privateWrites;
    const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&sceneUseFence))))return false;
    if(!PTSceneQueues::Start(&PTSceneLive::owner.Scenes())||!PTSceneQueues::Register(&PTSceneLive::owner.Scenes(),&queueConsumer)||
       !PTCommandBindings::Register(&commandConsumer)||!PTNativeJobRuntime::Register(&Job)||!PTSceneLive::owner.Start(step,true))return false;
    ptReflectionModeObserver.store(&MirrorModeChanged,std::memory_order_release);
    MirrorModeChanged(PTReflectionUserRequested());
    active=true;Log("PT_PRIMARY_SESSION_READY build=M12R103DEV ready=1 baked_gi=0 probe_lighting=0 direct_lobe=native_radius_broadening shadow_policy=all_traced hits=4 spp=1 roulette_start=3 material=opaque_general secondary_mesh_normals=1 geometric_spawn_and_hemisphere=1 native_fallback=1 primary_lighting=%s transport=%s readiness=per_material graphics_tracking=%u",surfaceReplacement?"pt":"native",surfaceReplacement?"full_surface":"diffuse_indirect",unsigned(compose));return true;
}
}
