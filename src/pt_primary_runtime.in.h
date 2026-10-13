// Included inside PTGlassRuntime after the native context/allocator adapters.
// One session owner. Every native call is still forwarded by its outer hook.
namespace Primary {
using View=PTNativeView::View;
using Bindings=PTCommandBindings::Snapshot;
using Table=PTTableMirror::Bank::Snapshot;
using Output=PTSurfaceComposition::Output;
static PTSceneHandoff::Runtime handoff{PTSceneLive::owner};
static std::atomic<bool> active{false};
static std::recursive_mutex mutex;
static std::vector<unsigned char> library;
static PTNativeView::Providers viewProviders;
static std::uint64_t nextTicket=1ull<<63;
static ComPtr<IUnknown> pinnedQueue;
// Extend native AS/attribute read lifetime through the private traversal. Every
// later submission on another native queue waits for the latest PT use. The
// scene cut rejects ALREADY submitted unordered AS writes before arming rays.
static ComPtr<ID3D12Fence> sceneUseFence;
static UINT64 sceneUseValue=0;
static std::map<IUnknown*,std::pair<ComPtr<IUnknown>,UINT64>> sceneWaits;
static std::map<ID3D12RootSignature*,std::shared_ptr<PTScenePipeline::Pipeline>> privatePipelines;
static std::atomic<UINT64> jobCallbacks{0},jobOff{0},jobUnknownPipeline{0},opaqueAccepted{0};
static std::atomic<UINT64> computeCallbacks{0},computeOff{0},computeNoPipeline{0},computeOtherPipeline{0},primarySeen{0},environmentSeen{0};
static void Reject(const char* reason){
    static std::atomic<UINT64> count{0};const auto n=++count;
    if(n<=32||n%600==0)Log("PT_PRIMARY_SKIP reason=%s count=%llu native_surface=retained",reason,n);
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
struct Candidate {PTNativeJobRoute::Ticket ticket;View view;};
static std::map<UINT64,Candidate> candidates;
struct Environment {
    View view;Table table;ComPtr<IUnknown> list,queue;bool submitted=false;
};
static std::vector<std::shared_ptr<Environment>> environments;
struct Assets {
    PTPrimaryMetrics::Counters metrics;
    ComPtr<ID3D12Resource> camera;View boundView;
    Bindings bindings;Table primary;std::shared_ptr<Environment> environment;
    PTOwnedScene::LeaseBatch scene;
    std::shared_ptr<const PTSceneHandoff::Prepared> proof;
    std::shared_ptr<PTScenePipeline::Dispatch> dispatch;
    PTGlass::DescriptorRange privateTable;
    UINT64 ticket=0,engineFrame=0;
};
struct Frame {
    View view;std::shared_ptr<Output> output;ComPtr<ID3D12CommandQueue> queue;
    std::shared_ptr<Assets> assets;bool producerRecorded=false,producerSubmitted=false,armed=false;
};
struct Draw {
    std::shared_ptr<Frame> frame;std::shared_ptr<PTSurfaceComposition::Pipeline> pipeline;
    Table original;PTGlass::DescriptorRange privateTable;UINT64 viewAddress=0;View boundView;
};
struct Recording {
    ComPtr<IUnknown> identity;std::vector<std::shared_ptr<Draw>> draws;
    std::vector<std::shared_ptr<Frame>> producers;bool submitted=false;
};
static std::map<IUnknown*,Recording> recordings;
static std::vector<std::weak_ptr<Frame>> frames;
static std::vector<std::shared_ptr<Frame>> pendingMetrics;
static void PollMetrics(){
    for(auto it=pendingMetrics.begin();it!=pendingMetrics.end();){auto& f=**it;
        if(!f.producerSubmitted||!f.output->Completed()){++it;continue;}
        std::array<UINT,8> counts{};const bool good=f.assets&&f.assets->metrics.Read(counts);
        static UINT64 samples=0;const auto n=++samples;
        if(n<=8||n%240==0)Log("PT_PRIMARY_GPU frame=%u readback=%u armed=%u sampled=%u dimensions=%u primary_hit=%u material_match=%u depth_match=%u integrated=%u covered=%u fault=%u sparse_step=16",f.view.frame,unsigned(good),unsigned(f.armed),counts[0],counts[1],counts[2],counts[3],counts[4],counts[5],counts[6],counts[7]);
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
    if(FAILED(Output::Create(queue,view.width,view.height,f->output)))return {};
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
static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource*,D3D12_CPU_DESCRIPTOR_HANDLE){
    auto& a=*static_cast<Assets*>(context);ComPtr<ID3D12GraphicsCommandList4> rays;if(FAILED(list->QueryInterface(IID_PPV_ARGS(&rays))))return false;
    auto counter=Gpu(a.privateTable);counter.ptr+=47ull*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    a.metrics.Begin(list,counter);
    list->SetComputeRootConstantBufferView(2,a.camera->GetGPUVirtualAddress());
    const bool recorded=a.dispatch->Record(rays.Get(),a.bindings.layout->Root(),a.bindings.pipeline.Get(),{a.bindings.arguments[0].value},Gpu(a.privateTable));
    list->SetComputeRootConstantBufferView(2,a.bindings.arguments[2].value);
    a.metrics.End(list);return recorded;
}
static void Job(PTNativeJobRuntime::Phase phase,const PTNativeJobRoute::Ticket& ticket) noexcept {
    if(!active)return;
    try {
        // WriterReady owns its input until this callback returns. It never waits
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
        if(handoff.Recorded(ticket,bindings.resources.Get(),context.table.first,5))candidates.emplace(ticket.serial,Candidate{ticket,view});
        else Reject("opaque_handoff");
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
        std::lock_guard<std::recursive_mutex> ordered(PTSceneQueues::queueMutex);PTSceneQueues::Scope privateQueues;
        std::lock_guard<std::recursive_mutex> guard(mutex);PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
        Bindings bindings;if(!PTCommandBindings::tracker.Read(list,nativeRoots,false,0xd5,bindings)){Reject("lighting_bindings");return;}
        View view;Context context;
        if(!ReadView(bindings.arguments[2].value,view,environmentPass?PTNativeView::Pass::environment:PTNativeView::Pass::primary)||!ContextFor(list,bindings,0,context)){Reject("lighting_view_or_context");return;}
        if(environmentPass){
            // Prefer the first observed native/P2 dispatch for this immutable
            // table/view, not a guessed globally most recent environment.
            for(const auto& e:environments)if(e->view.Same(view)&&e->table.native.Get()==bindings.resources.Get()&&e->table.first==context.table.first)return;
            auto e=std::make_shared<Environment>();e->view=view;e->list=Identity(list);e->queue=Identity(context.queue);
            if(!PTTableMirror::bank.Read(bindings.resources.Get(),context.table.first,1ull<<13,e->table)){Reject("environment_descriptor");return;}
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
        UINT64 serial=0;
        if(!handoff.FindReady(candidate->ticket.engineFrame,view.width,view.height,candidate->ticket.pipeline,serial)){Reject("writer_or_submission_not_ready");return;}
        auto frame=FrameFor(view,context.queue);auto* recording=RecordingFor(list);
        if(!frame||!recording||frame->producerRecorded||recording->submitted){Reject("frame_budget_or_duplicate");return;}
        auto a=std::make_shared<Assets>();a->bindings=bindings;a->boundView=view;a->environment=environment;a->engineFrame=candidate->ticket.engineFrame;
        // t0,t1,t3,t7,t8,t10,t14,t15,t17,t18. Private t34/t35/u7 are installed below.
        constexpr UINT64 required=(1ull<<5)|(1ull<<6)|(1ull<<8)|(1ull<<12)|(1ull<<13)|(1ull<<15)|(1ull<<19)|(1ull<<20)|(1ull<<22)|(1ull<<23);
        if(!PTTableMirror::bank.Read(bindings.resources.Get(),context.table.first,required,a->primary)||
           !PTTableMirror::bank.Unchanged(environment->table)||!Fresh(context,&a->privateTable)||!Disjoint(a->privateTable,context.table.first)){Reject("primary_descriptor_allocation");return;}
        auto& pipeline=privatePipelines[bindings.layout->Root()];
        if(!pipeline){
            if(privatePipelines.size()>8){Reject("pipeline_budget");return;}ComPtr<ID3D12Device5> rayDevice;
            if(FAILED(device.As(&rayDevice)))return;auto p=std::make_shared<PTScenePipeline::Pipeline>();
            const HRESULT hr=p->Initialize(rayDevice.Get(),bindings.layout->Root(),library.data(),library.size(),L"PTPrimaryPathRayGeneration");
            if(FAILED(hr)){Log("PT_PRIMARY_PIPELINE_FAILED hr=0x%08lX",static_cast<unsigned long>(hr));return;}pipeline=std::move(p);
        }
        PTNativeShaderTable::Identifiers identifiers;PTSceneDispatch::ExportSet exports;PTSceneDispatch::Plan plan;
        if(!handoff.ReadyIdentifiers(serial,identifiers)||!pipeline->Exports(candidate->ticket.pipeline,identifiers.hit,exports)){Reject("private_exports");return;}
        const auto status=handoff.Prepare(serial,exports,plan,a->scene,&a->proof);
        if(status!=PTSceneHandoff::Status::pending)candidates.erase(serial);
        if(status!=PTSceneHandoff::Status::prepared){Reject("scene_prepare");return;}
        if(FAILED(PTScenePipeline::Dispatch::Create(device.Get(),pipeline,plan,view.width,view.height,bindings.resources.Get(),a->dispatch))){Reject("private_sbt");return;}
        D3D12_HEAP_PROPERTIES cameraHeap{};cameraHeap.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC cameraDesc{};cameraDesc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;cameraDesc.Width=1280;
        cameraDesc.Height=1;cameraDesc.DepthOrArraySize=cameraDesc.MipLevels=1;cameraDesc.SampleDesc.Count=1;cameraDesc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(FAILED(device->CreateCommittedResource(&cameraHeap,D3D12_HEAP_FLAG_NONE,&cameraDesc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&a->camera)))){Reject("camera_upload");return;}
        void* cameraData=nullptr;const D3D12_RANGE noRead{};
        if(FAILED(a->camera->Map(0,&noRead,&cameraData))||!cameraData){Reject("camera_map");return;}
        std::memcpy(cameraData,view.bytes.data(),view.bytes.size());a->camera->Unmap(0,nullptr);
        if(!a->metrics.Initialize(device.Get())){Reject("coverage_allocation");return;}
        device->CopyDescriptorsSimple(49,Cpu(a->privateTable),a->primary.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1,Cpu(a->privateTable,47),a->metrics.Uav(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1,Cpu(a->privateTable,39),environment->table.At(13),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_SHADER_RESOURCE_VIEW_DESC as{};as.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        as.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;as.RaytracingAccelerationStructure.Location=plan.selection.tlas;
        device->CreateShaderResourceView(nullptr,&as,Cpu(a->privateTable,40));
        device->CopyDescriptorsSimple(1,Cpu(a->privateTable,48),frame->output->Uav(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(nextTicket==UINT64_MAX){Reject("ticket_exhausted");return;}a->ticket=nextTicket++;
        if(!PTSceneLive::owner.Scenes().Mark(list,a->ticket)){Reject("private_scene_cut");return;}
        // Attach every owner before recording the first GPU reference. Even a
        // failed encoder retains these through the list's native submission.
        recording->producers.push_back(frame);frame->assets=a;frame->producerRecorded=true;
        if(!frame->output->RecordInline(list,a.get(),&Encode,a)){frame->output->Invalidate(context.queue);Reject("inline_record");return;}
        pendingMetrics.push_back(frame);
        static UINT64 recorded=0;if(++recorded<=8||recorded%240==0)Log("PT_PRIMARY_RECORDED count=%llu frame=%u dimensions=%ux%u hits=4 spp=1",recorded,view.frame,view.width,view.height);
    }catch(...){Reject("dispatch_allocation");}
}
struct Restore {ID3D12GraphicsCommandList* list=nullptr;ComPtr<ID3D12PipelineState> pipeline;D3D12_GPU_DESCRIPTOR_HANDLE table{};};
static thread_local Restore restore;
static void Drawing(void*,ID3D12GraphicsCommandList* list) noexcept {
    if(!active||ptReflectionUserState.load(std::memory_order_acquire)!=1||!PTSceneLive::owner.Scenes().Healthy())return;
    try {
        if(restore.list)return;
        // No surface can consume PT before a producer exists. Avoid mapping
        // camera buffers and allocating descriptors for every native draw
        // when opaque/lighting admission has already failed this frame.
        {std::lock_guard<std::recursive_mutex> guard(mutex);
            bool produced=false;for(const auto& weak:frames){const auto frame=weak.lock();if(frame&&frame->producerRecorded){produced=true;break;}}
            if(!produced)return;
        }
        const auto pso=PTCommandBindings::tracker.PipelineFor(list);
        auto pipeline=surfacePipelines.Find(pso.Get());if(!pipeline)return;
        std::lock_guard<std::recursive_mutex> ordered(PTSceneQueues::queueMutex);PTSceneQueues::Scope privateQueues;
        std::lock_guard<std::recursive_mutex> guard(mutex);PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
        Bindings bindings;
        if(!PTCommandBindings::tracker.Read(list,nativeRoots,true,0x31002,bindings)){Reject("surface_bindings");return;}
        if(!bindings.layout->Matches(pipeline->Root()))return;
        View view;Context context;if(!ReadView(bindings.arguments[12].value,view,PTNativeView::Pass::surface)||!ContextFor(list,bindings,1,context)){Reject("surface_view_or_context");return;}
        auto* recording=RecordingFor(list);if(!recording||recording->submitted||recording->draws.size()>=4096)return;
        std::shared_ptr<Draw> draw;
        for(const auto& d:recording->draws)if(d->pipeline==pipeline&&d->viewAddress==bindings.arguments[12].value&&
            d->original.native.Get()==bindings.resources.Get()&&d->original.first==context.table.first&&d->frame->view.Same(view)){draw=d;break;}
        if(!draw){
            auto frame=FrameFor(view,context.queue);if(!frame){Reject("surface_frame_budget");return;}
            draw=std::make_shared<Draw>();draw->frame=frame;draw->pipeline=pipeline;draw->viewAddress=bindings.arguments[12].value;draw->boundView=view;
            if(!PTTableMirror::bank.Read(bindings.resources.Get(),context.table.first,pipeline->Required(),draw->original)||
               !Fresh(context,&draw->privateTable)||!Disjoint(draw->privateTable,context.table.first)){Reject("surface_descriptor_allocation");return;}
            device->CopyDescriptorsSimple(49,Cpu(draw->privateTable),draw->original.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            device->CopyDescriptorsSimple(1,Cpu(draw->privateTable,40),frame->output->Srv(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            recording->draws.push_back(draw);
        }
        restore={list,bindings.pipeline,{bindings.arguments[1].value}};
        list->SetPipelineState(pipeline->Composed());list->SetGraphicsRootDescriptorTable(1,Gpu(draw->privateTable));
    }catch(...){Reject("surface_allocation");}
}
static void Drawn(void*,ID3D12GraphicsCommandList* list) noexcept {
    if(restore.list!=list)return;list->SetGraphicsRootDescriptorTable(1,restore.table);list->SetPipelineState(restore.pipeline.Get());restore={};
}
static void Submitting(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);auto key=Identity(queue);
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
static void Selected(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists,bool admitted) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);PTTableMirror::Scope descriptorLock(true);PTTableMirror::Mute privateWrites;
        if(!lists||count>256)return;const auto queueIdentity=Identity(queue);
        for(UINT n=0;n<count;++n){const auto key=Identity(lists[n]);
            for(auto& e:environments)if(e->list.Get()==key.Get()&&e->queue.Get()==queueIdentity.Get())e->submitted=true;
            auto found=recordings.find(key.Get());if(found==recordings.end())continue;auto& recording=found->second;
            for(const auto& frame:recording.producers){const auto& a=frame->assets;
                const bool good=admitted&&!recording.submitted&&!frame->producerSubmitted&&(ptReflectionUserState.load()==1u)&&a&&
                    queueIdentity.Get()==pinnedQueue.Get()&&a->environment->submitted&&
                    SameView(a->bindings.arguments[2].value,a->boundView)&&PTTableMirror::bank.Unchanged(a->primary)&&
                    PTTableMirror::bank.Unchanged(a->environment->table)&&
                    handoff.Revalidate(a->ticket,a->engineFrame,frame->view.width,frame->view.height,a->proof)==PTSceneHandoff::Status::prepared;
                if(!good||!frame->output->Arm(queue)){frame->output->Invalidate(queue);Reject("submission_rejected");}
                else {frame->armed=true;static UINT64 submitted=0;if(++submitted<=8||submitted%240==0)Log("PT_PRIMARY_SUBMITTED count=%llu frame=%u scene=exact alpha_guard=armed",submitted,frame->view.frame);}
            }
            for(const auto& draw:recording.draws){
                const bool changed=!PTTableMirror::bank.Unchanged(draw->original);
                if(changed&&PTTableMirror::bank.Refresh(draw->original)){
                    device->CopyDescriptorsSimple(49,Cpu(draw->privateTable),draw->original.At(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1,Cpu(draw->privateTable,40),draw->frame->output->Srv(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
                if(recording.submitted||queueIdentity.Get()!=pinnedQueue.Get()||!SameView(draw->viewAddress,draw->boundView)||changed){
                    draw->frame->output->Invalidate(queue);Reject("surface_submission_changed");
                }
            }
        }
    }catch(...){PTSceneLive::owner.Scenes().Invalidate();Reject("submission_exception");}
}
static void Submitted(void*,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists,bool) noexcept {
    if(!active)return;
    try {
        std::lock_guard<std::recursive_mutex> guard(mutex);if(!lists||count>256)return;
        std::array<Output*,8> signalled{};UINT signals=0;bool traversed=false;
        const auto signal=[&](const std::shared_ptr<Output>& output){
            for(UINT i=0;i<signals;++i)if(signalled[i]==output.get())return;
            output->NativeSubmitted(queue);if(signals<signalled.size())signalled[signals++]=output.get();
        };
        for(UINT n=0;n<count;++n){auto key=Identity(lists[n]);auto found=recordings.find(key.Get());if(found==recordings.end())continue;
            auto& r=found->second;for(const auto& f:r.producers){f->producerSubmitted=true;traversed=traversed||f->armed;signal(f->output);if(f->assets)PTSceneLive::owner.Scenes().Forget(f->assets->ticket);}
            for(const auto& d:r.draws)signal(d->frame->output);r.submitted=true;
        }
        if(traversed){
            const auto value=sceneUseValue+1;
            if(sceneUseValue>=UINT64_MAX-1||FAILED(queue->Signal(sceneUseFence.Get(),value))){
                PTSceneLive::owner.Scenes().Invalidate();Reject("scene_use_signal");
            }else {
                sceneUseValue=value;
                if(!PTSceneLive::owner.Scenes().Signal(queue,sceneUseFence.Get(),value))PTSceneLive::owner.Scenes().Invalidate();
            }
        }
        static UINT64 submissions=0;const auto sequence=++submissions;
        if(sequence<=4||sequence%240==0){const auto t=PTSceneLive::owner.Scenes().Statistics();
            const auto pool=PTSceneLive::owner.Resources().GeometryStatistics();
            Log("PT_PRIMARY_CAPTURE healthy=%u failure=%s skipped_invalid=%llu copy_failures=%llu copy_failure=%s pooled_idle_bytes=%llu buffer_allocations=%llu buffer_reuses=%llu history_versions=%llu history_rows=%llu history_nodes=%llu copy_bytes=%llu",unsigned(t.healthy),t.failure,t.skippedInvalid,t.copyFailures,PTSceneLive::owner.Resources().GeometryFailure(),pool.bytes,pool.allocations,pool.reuses,t.historyVersions,t.historyRows,t.historyNodes,t.copyBytes);
            Log("PT_PRIMARY_CPU submissions=%llu builds=%llu build_total_ms=%llu build_max_ms=%llu checkpoints=%llu checkpoint_total_ms=%llu checkpoint_max_ms=%llu blocking_checkpoint_waits=0",sequence,t.builds.calls,t.builds.totalMs,t.builds.maxMs,t.checkpoints.calls,t.checkpoints.totalMs,t.checkpoints.maxMs);}
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
                if(f->assets)PTSceneLive::owner.Scenes().Forget(f->assets->ticket);
                if(!f->producerSubmitted)pendingMetrics.erase(std::remove(pendingMetrics.begin(),pendingMetrics.end(),f),pendingMetrics.end());
            }
            recordings.erase(found);
        }
        PollMetrics();Output::Poll();
    }catch(...){Reject("reset_exception");}
}
static const PTSceneQueues::Consumer queueConsumer{&handoff,&Submitting,&Selected,&Submitted,&Reset};
static const PTCommandBindings::Consumer commandConsumer{&handoff,&Dispatched,&Drawing,&Drawn};
static bool Start(){
    if(active||!device||!enabled||!PTNativeJobRuntime::workerHooksReady||!PTNativeJobRuntime::dispatchHookReady)return false;
    if(!viewProviders.Initialize(verifiedRenderer,verifiedD3d)){Log("PT_PRIMARY_PROVIDER_INIT_FAILED");return false;}
    char sha[65]{};bool loaded=false;
    if(!PTReflectionLoadSidecar(L"pt_primary_path.dxil","PRIMARY_PATH",library,sha,loaded))return false;
    pendingMetrics.reserve(8);
    PTCommandBindings::Mute privateRecording;PTTableMirror::Mute privateWrites;
    const auto step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&sceneUseFence))))return false;
    if(!PTSceneQueues::Start(&PTSceneLive::owner.Scenes())||!PTSceneQueues::Register(&PTSceneLive::owner.Scenes(),&queueConsumer)||
       !PTCommandBindings::Register(&commandConsumer)||!PTNativeJobRuntime::Register(&Job)||!PTSceneLive::owner.Start(step,true))return false;
    active=true;Log("PT_PRIMARY_SESSION_READY build=M12R7 ready=1 hits=4 spp=1 material=opaque_general geometric_normals=1 native_fallback=1");return true;
}
}
