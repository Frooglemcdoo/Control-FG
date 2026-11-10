#pragma once
#ifdef CONTROL_NRD_GI
#include "nrd_diffuse_native_transaction.h"
#include "nrd_diffuse_receiver_d3d12.h"
#include "nrd_diffuse_camera.h"
#include "nrd_prepare.h"
#include "nrd_finish.h"
#include "nrd_receiver.h"
#include <mutex>
namespace control_rr {
inline void ApplyRRBundleDefaults(bool enabled) noexcept {
 gi_reuse::ownedRequested=enabled;GISampleSelect(0);
 native_lighting::specularSampleChoice=0;
 rt_surface_source::ResetManual();
 control_rr_clamp::requestedMode=control_rr_clamp::Mode::Override60;
}
}
namespace ControlNrdRuntime {
using Microsoft::WRL::ComPtr;
namespace hooks=native_lighting::gi_compute_hooks;
struct Epoch {
 UINT width=0,height=0;bool poisoned=false;
 ComPtr<ID3D12RootSignature> root;
 std::unique_ptr<control_nrd::NativeReceiver> receiver;
 std::unique_ptr<control_nrd::DiffusePipeline> filter;
};
struct Recording {
 ComPtr<ID3D12GraphicsCommandList4> list;UINT64 generation=0,fence=0;
 bool submitted=false,reset=false,filtered=false,exported=false,checkCompleted=false;
 UINT64 checkGeneration=0;
 std::shared_ptr<Epoch> epoch;
 std::vector<control_nrd::NativeReceiver::Work> exports;
 std::unique_ptr<control_nrd::NativeTransaction::Work> work;
};
static std::recursive_mutex mutex;
static std::vector<std::unique_ptr<Recording>> recordings;
static std::shared_ptr<Epoch> epoch;
static std::vector<std::unique_ptr<control_nrd::NativeTransaction::Work>> retiredWork;
static ComPtr<ID3D12CommandQueue> queue;
static ComPtr<ID3D12Fence> fence;
static UINT64 nextFence=0,serial=0,guideFrame=0,guideView=0,guideGeneration=0;
static ID3D12GraphicsCommandList* guideList=nullptr;
static unsigned guideProducer=0;
static LONGLONG lastFilterTicks=0;static UINT64 lastFilterFrame=0;
static std::atomic<bool> active{false};
struct Pass {UINT64 frame=0,view=0,serial=0;bool failed=false,producer=false;
 PTGlassRuntime::Context context{};std::unique_ptr<PTGlass::LightingTableShadow> shadow;};
static thread_local Pass pass;
struct Mute {bool observing=gi_dual_bridge::observing;hooks::Mute bindings;
 control_rr_clamp::Request* nativeRequest=control_rr_clamp::active;
 Mute(){gi_dual_bridge::observing=false;control_rr_clamp::active=nullptr;}
 ~Mute(){control_rr_clamp::active=nativeRequest;gi_dual_bridge::observing=observing;}};
static void Reason(const char* reason) noexcept {
 if(settings_check::active.load()&&strcmp(reason,"active")){
  settings_check::reason=reason;settings_check::Record(settings_check::RelaxFallback);
  unsigned pending=0;for(const auto& r:recordings)if(!r->reset&&!r->submitted)++pending;
  settings_check::RecordFailure(reason,pass.frame,presentCount.load(),guideFrame,unsigned(recordings.size()),pending,pass.view==guideView);
 }
 gi_dual_bridge::status=2;
 static const char* last=nullptr;if(last!=reason){last=reason;Log("NRD_R384 admission=%s",reason);}
}
static bool Requested() noexcept {
 if(!control_rr::RRUserRuntimeWorkRequested())return false;
 const auto request=rt_surface_source::EffectiveRequest();
 return request==7||request==3||(!request&&control_rr_gi::stageMode.load()==control_rr_gi::StageMode::BothSteadySpatial);
}
static void Invalidate(const std::shared_ptr<Epoch>& e) noexcept {
 if(e){e->poisoned=true;e->receiver->AbandonAfterReset();e->filter->AbandonAfterReset();}
 guideFrame=0;
}
static void Poll() {
 const UINT64 complete=fence?fence->GetCompletedValue():0;
 if(complete==UINT64_MAX){Invalidate(epoch);Reason("device_removed");return;}
 for(auto i=recordings.begin();i!=recordings.end();){auto& r=**i;
  if(r.submitted&&r.filtered&&!r.checkCompleted&&r.fence<=complete){settings_check::RecordFor(settings_check::RelaxCompleted,r.checkGeneration);r.checkCompleted=true;}
  if(r.reset&&(!r.submitted||r.fence<=complete)){
   if(!r.submitted&&(r.filtered||r.exported||(r.work&&r.work->diffuse.commandsRecorded)))Invalidate(r.epoch);
   // Both conditions above are mandatory: completion alone cannot rule out
   // replay of an unreset list. Never recycle failed or obsolete-epoch work.
   if(r.submitted&&r.filtered&&r.work&&r.epoch==epoch&&!epoch->poisoned&&retiredWork.size()<8){
    r.work->RetireForReuse();retiredWork.push_back(std::move(r.work));
   }
   i=recordings.erase(i);
  }else ++i;
 }
}
static bool Initialize(ID3D12CommandQueue* q) {
 if(queue)return queue.Get()==q;
 auto* d=PTGlassRuntime::device.Get();if(!d||!q)return false;
 ComPtr<ID3D12Fence> f;if(FAILED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&f)))||
    !native_lighting::rt_spot_table_audit::InstallGameQueue(q))return false;
 recordings.reserve(64);retiredWork.reserve(8);queue=q;fence=f;active=true;return true;
}
static Recording* GetRecording(ID3D12GraphicsCommandList4* list,UINT64 generation) {
 for(auto& r:recordings)if(r->list.Get()==list&&!r->reset){
  if(r->generation!=generation||r->submitted)return nullptr;return r.get();
 }
 if(recordings.size()>=64)return nullptr;
 auto r=std::make_unique<Recording>();r->list=list;r->generation=generation;
 r->exports.reserve(16);auto* result=r.get();recordings.push_back(std::move(r));return result;
}
static void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!active)return;std::lock_guard<std::recursive_mutex> lock(mutex);
 for(auto& r:recordings)if(r->list.Get()==list)r->reset=true;Poll();
}
static UINT64 Before(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!active)return 0;mutex.lock();UINT64 mask=0;
 for(unsigned i=0;i<recordings.size();++i){auto& r=*recordings[i];if(r.reset)continue;
  for(UINT j=0;j<count;++j)if(lists&&lists[j]==r.list.Get()){
   if(q!=queue.Get()||r.submitted||(mask&(1ull<<i))){Invalidate(r.epoch);Reason("queue_or_replay");continue;}
   mask|=1ull<<i;
  }
 }
 if(!mask)mutex.unlock();return mask;
}
static void After(ID3D12CommandQueue* q,UINT64 mask) noexcept {
 if(!mask)return;
 for(unsigned i=0;i<recordings.size();++i)if(mask&(1ull<<i)){
  auto& r=*recordings[i];r.submitted=true;r.fence=++nextFence;
  if(FAILED(q->Signal(fence.Get(),r.fence))||(r.filtered&&!r.epoch->filter->Submitted(reinterpret_cast<UINT64>(q),r.fence))){r.checkGeneration=0;Invalidate(r.epoch);Reason("submission_failed");}
  else if(r.filtered)settings_check::RecordFor(settings_check::RelaxSubmitted,r.checkGeneration);
 }
 mutex.unlock();
}
static void Begin(UINT64 frame,UINT64 view) noexcept {
 gi_dual_bridge::observing=false;pass.frame=frame;pass.view=view;pass.serial=0;pass.failed=false;pass.producer=false;pass.context={};
 // Temporal descriptors are already bound when Dispatch is reached. Only the
 // producer needs mirroring; native temporal inputs use authenticated providers.
}
static void End() noexcept {gi_dual_bridge::observing=false;}
static void ProducerBegin() noexcept {
 gi_dual_bridge::observing=false;pass.serial=0;pass.failed=false;pass.producer=true;pass.context={};
 if(!Requested())return;
 CameraSnapshot c{};DWORD fault=0;const char* reason=nullptr;
 if(!ReadCamera(&c,&fault,&reason))return;pass.frame=c.engineFrame;pass.view=reinterpret_cast<UINT64>(c.view);
 gi_dual_bridge::observing=true;
}
static PTGlass::LightingTableShadow* Shadow() {
 if(!gi_dual_bridge::observing||!pass.producer||pass.failed)return nullptr;
 try{if(!pass.serial){
   if(!PTGlassRuntime::ReadContext(&pass.context)){pass.failed=true;return nullptr;}
   Mute mute;if(!pass.shadow){pass.shadow=std::make_unique<PTGlass::LightingTableShadow>();
    if(FAILED(pass.shadow->Initialize(PTGlassRuntime::device.Get()))){pass.failed=true;return nullptr;}}
   {std::lock_guard<std::recursive_mutex> lock(mutex);pass.serial=++serial;}
   if(!pass.shadow->Begin(pass.serial,pass.context.table)){pass.failed=true;return nullptr;}
  }return pass.shadow.get();
 }catch(...){pass.failed=true;return nullptr;}
}
static void Copy(D3D12_CPU_DESCRIPTOR_HANDLE dst,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE src) noexcept {
 if(auto* s=Shadow()){Mute mute;s->ObserveNativeCopy(pass.serial,dst,count,src);}
}
static void Constant(D3D12_CPU_DESCRIPTOR_HANDLE dst,const D3D12_CONSTANT_BUFFER_VIEW_DESC& cb) noexcept {
 if(auto* s=Shadow()){Mute mute;s->ObserveNativeConstant(pass.serial,dst,cb);}
}
static bool NamedTexture(const char* name,control_nrd_native::Texture& out) {
 int id=-1;control_nrd_native::ShaderTexture value{};
 const control_rr_provider::Access access{nullptr,&RRProviderEntry,&RRProviderCompare};
 return control_nrd_native::Resolve(rrNativeClampApi,name,id)&&control_rr_provider::Snapshot(access,id,&RRProviderReadBound,value)&&control_nrd_native::ReadTexture(value,out);
}
static bool CameraMatches(const control_nrd_native::Inputs& input,const control_nrd::CameraMatrices& camera) {
 struct Matrix {float v[16];};
 const control_rr_provider::Access access{nullptr,&RRProviderEntry,&RRProviderCompare};
 Matrix world{},projection{};int id=-1;
 if(!control_nrd_native::Resolve(rrNativeClampApi,"g_mWorldToView",id)||!control_rr_provider::Snapshot(access,id,&RRProviderReadBound,world)||
    !control_nrd_native::Resolve(rrNativeClampApi,"g_mViewToClip",id)||!control_rr_provider::Snapshot(access,id,&RRProviderReadBound,projection))return false;
 bool plain=true,jittered=true;
 for(unsigned i=0;i<16;++i){
  if(!std::isfinite(world.v[i])||std::abs(world.v[i]-camera.worldToView[i])>1e-4f*(1+std::abs(camera.worldToView[i])))return false;
  const double expected=camera.viewToClip[i];
  const double shifted=expected+(i%4<2?input.clipShift[i%4]*camera.viewToClip[(i/4)*4+3]:0);
  plain=plain&&std::isfinite(projection.v[i])&&std::abs(projection.v[i]-expected)<=1e-4*(1+std::abs(expected));
  jittered=jittered&&std::isfinite(projection.v[i])&&std::abs(projection.v[i]-shifted)<=1e-4*(1+std::abs(shifted));
 }
 return plain||jittered;
}
static bool Bindings(PTGlassRuntime::Context& c,gi_reuse::ComputeBindingSnapshot& binding,UINT64& generation) {
 if(!PTGlassRuntime::ReadContext(&c)||!c.rays||!hooks::Capture(c.list,binding,generation)||!binding.pipeline||!binding.root||binding.heaps[0].Get()!=c.table.heap)return false;
 const auto reset=gi_dual_bridge::resetReady.load();return reset&&reset(c.list)&&Initialize(c.queue);
}
static void ProducerDispatch() noexcept {
 if(!pass.producer||!pass.serial||pass.failed||!Requested())return;
 const auto sequence=pass.serial;pass.serial=0;
 std::lock_guard<std::recursive_mutex> lock(mutex);Mute mute;
 try {
  Poll();PTGlassRuntime::Context c;gi_reuse::ComputeBindingSnapshot b;UINT64 generation=0;
  if(!Bindings(c,b,generation)||c.list!=pass.context.list||c.gpu.ptr!=pass.context.gpu.ptr){Reason("producer_bindings");return;}
  UINT mode=0,bytes=sizeof(mode);if(FAILED(b.pipeline->GetPrivateData(native_lighting::gi_evaluator::kMode,&bytes,&mode))||!mode){Reason("producer_variant");return;}
  control_nrd_native::Texture raw;if(!NamedTexture("g_rwtDiffuseGIColorTarget",raw)){Reason("producer_target");return;}
  const UINT w=UINT(raw.desc.Width),h=raw.desc.Height;
  // Never overwrite guides/history referenced by an unsubmitted earlier list.
  for(auto& r:recordings)if(!r->reset&&!r->submitted&&((r->list.Get()!=c.list&&(r->exported||r->filtered))||r->filtered)){Reason("producer_submission_pending");return;}
  if(!epoch||epoch->poisoned||epoch->width!=w||epoch->height!=h||epoch->root.Get()!=b.root.Get()){
   std::vector<Epoch*> held;for(auto& r:recordings)if(r->epoch&&std::find(held.begin(),held.end(),r->epoch.get())==held.end())held.push_back(r->epoch.get());
   if(held.size()>=3){Reason("epoch_retirement_pending");return;}
   auto e=std::make_shared<Epoch>();e->width=w;e->height=h;e->root=b.root;
   e->receiver=std::make_unique<control_nrd::NativeReceiver>(PTGlassRuntime::device.Get(),b.root.Get(),D3D12_SHADER_BYTECODE{kNrd_receiver,sizeof(kNrd_receiver)},w,h);
   e->filter=std::make_unique<control_nrd::DiffusePipeline>(PTGlassRuntime::device.Get(),w,h,D3D12_SHADER_BYTECODE{kNrd_prepare,sizeof(kNrd_prepare)},D3D12_SHADER_BYTECODE{kNrd_finish,sizeof(kNrd_finish)});retiredWork.clear();epoch=e;
  }
  auto* r=GetRecording(c.rays,generation);if(!r||r->exports.size()>=16){Reason("producer_recording_capacity");return;}
  if(r->epoch&&r->epoch!=epoch){Reason("producer_epoch_change");return;}
  r->epoch=epoch;PTGlass::DescriptorRange fresh;const char* allocationFailure=nullptr;
  const bool allocated=PTGlassRuntime::FreshCount(c,PTGlass::LightingTableLayout::Count,&fresh,&allocationFailure);
  gi_reuse::GraphicsTableState graphics;
  if(!allocated&&(strcmp(allocationFailure,"filter_heap_rollover")||!hooks::CaptureGraphics(c.list,graphics))){Reason(allocationFailure);return;}
  r->exports.emplace_back();
  native_lighting::performance::Scope timing(c.list,native_lighting::performance::Stage::ReceiverExport,w,h,1,0);
  const bool exported=allocated?epoch->receiver->Record(c.list,b.pipeline.Get(),c.gpu,*pass.shadow,sequence,fresh,r->exports.back()):
   epoch->receiver->RecordPrivate(c.rays,b,graphics,*pass.shadow,sequence,r->exports.back());
  if(!exported){r->exports.pop_back();Reason(allocated?"producer_descriptors":"producer_private_heap");return;}
  if(!allocated)settings_check::Record(settings_check::ReceiverHeapRecovered);
  settings_check::Record(settings_check::ReceiverExport);
  r->exported=true;guideFrame=pass.frame;guideView=pass.view;guideList=c.list;guideGeneration=generation;guideProducer=mode;
 }catch(...){Invalidate(epoch);Reason("producer_exception");}
}
static bool Dispatch(ID3D12GraphicsCommandList* list,ID3D12PipelineState*,ID3D12PipelineState*,UINT x,UINT y,UINT z,gi_dual_bridge::Dispatch) noexcept {
 std::lock_guard<std::recursive_mutex> lock(mutex);Mute mute;
 Recording* record=nullptr;
 try {
  Poll();if(!epoch||epoch->poisoned||!guideFrame||guideFrame!=pass.frame||guideView!=pass.view||z!=1){Reason("same_frame_guides");return false;}
  PTGlassRuntime::Context c;gi_reuse::ComputeBindingSnapshot b;UINT64 generation=0;
  if(!Bindings(c,b,generation)||c.list!=list){Reason("filter_bindings");return false;}
  bool ordered=false;for(auto& r:recordings)if(r->list.Get()==guideList&&r->generation==guideGeneration&&r->exported&&r->epoch==epoch)
   ordered=r->submitted||(r->list.Get()==list&&!r->reset);
  if(!ordered){Reason("guide_submission_pending");return false;}
  control_nrd_native::Inputs in{};const char* reason=nullptr;
  if(!control_nrd_native::Read(rrNativeClampApi,pass.frame,pass.view,in,reason)){Reason(reason);return false;}
  control_nrd_native::Texture target;if(!NamedTexture("g_DLF_rwtColorTarget",target)||target.state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS){Reason("filter_target");return false;}
  const UINT w=epoch->width,h=epoch->height;if(x!=(w+7)/8||y!=(h+7)/8){Reason("filter_extent");return false;}
  control_nrd::CameraMatrices camera;std::array<float,2> jitter;
  if(!control_nrd::ConvertCamera(in.camera,camera)){Reason("camera_matrix_contract");return false;}
  if(!CameraMatches(in,camera)){Reason("camera_provider_mismatch");return false;}
  if(!control_nrd::NativeClipShiftToNrdJitter(in.clipShift[0],in.clipShift[1],w,h,jitter)){Reason("camera_jitter_range");return false;}
  control_nrd::Frame f;f.frame=pass.frame;f.view=pass.view;f.producer=guideProducer;f.width=std::uint16_t(w);f.height=std::uint16_t(h);
  f.sameFrameGuides=f.worldNormals=f.linearViewDepth=f.unjitteredMotion=f.radianceContract=f.hitDistanceContract=true;
  std::memcpy(f.settings.worldToViewMatrix,camera.worldToView.data(),64);std::memcpy(f.settings.viewToClipMatrix,camera.viewToClip.data(),64);
  f.settings.cameraJitter[0]=jitter[0];f.settings.cameraJitter[1]=jitter[1];f.settings.timeDeltaBetweenFrames=16.6667f;
  LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
  if(lastFilterTicks&&lastFilterFrame+1==f.frame&&frequency.QuadPart>0&&now.QuadPart>lastFilterTicks)
   f.settings.timeDeltaBetweenFrames=float(std::fmax(1.0,std::fmin(250.0,1000.0*(now.QuadPart-lastFilterTicks)/frequency.QuadPart)));
  control_nrd::DiffusePipeline::Bindings inputs;using E=control_nrd::D3D12Backend::External;
  inputs.input={E{in.raw.resource,in.raw.state},E{epoch->receiver->Resource(0),epoch->receiver->State(0)},E{epoch->receiver->Resource(2),epoch->receiver->State(2)},E{in.motion.resource,in.motion.state},E{in.hit.resource,in.hit.state},E{epoch->receiver->Resource(1),epoch->receiver->State(1)}};
  inputs.output={target.resource,target.state};
  record=GetRecording(c.rays,generation);if(!record||record->filtered||(record->epoch&&record->epoch!=epoch)){Reason("filter_recording");return false;}
  native_lighting::performance::Scope timing(list,native_lighting::performance::Stage::RelaxTotal,w,h,1,1);
  record->epoch=epoch;
  if(retiredWork.empty()){record->work=std::make_unique<control_nrd::NativeTransaction::Work>();settings_check::Record(settings_check::RelaxWorkCreated);}
  else {record->work=std::move(retiredWork.back());retiredWork.pop_back();settings_check::Record(settings_check::RelaxWorkReused);}
  gi_reuse::GraphicsTableState graphics;hooks::CaptureGraphics(c.list,graphics);
  struct Reservation {const PTGlassRuntime::Context* context;control_nrd::NativeTransaction::Work* work;bool canSwitch;bool* recovered;};
  bool recovered=false;Reservation reservation{&c,record->work.get(),graphics.known,&recovered};
  auto reserve=[](const void* context,UINT count,control_nrd::D3D12Backend::Arena& result)->const char* {
   const auto& request=*static_cast<const Reservation*>(context);PTGlass::DescriptorRange range;const char* why=nullptr;
   if(PTGlassRuntime::FreshCount(*request.context,count,&range,&why)){result={range.heap,range.first,range.count};return nullptr;}
   if(strcmp(why,"filter_heap_rollover")||!request.canSwitch)return why;
   auto& heap=request.work->privateHeap;
   if(!heap||heap->GetDesc().NumDescriptors<count){
    D3D12_DESCRIPTOR_HEAP_DESC desc{};desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;desc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;desc.NumDescriptors=epoch->filter->MaxDescriptors();
    if(FAILED(PTGlassRuntime::device->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&heap))))return "filter_private_heap_allocation";
   }
   result={heap.Get(),0,count};*request.recovered=true;return nullptr;
  };
  control_nrd::Timing detail;
  // Only sampled comparison frames enter the internal callback path.
  const auto interval=native_lighting::performance::sampleInterval.load();
  const bool details=control_compare::enabled.load()&&native_lighting::performance::enabled.load()&&presentCount.load()%(interval?interval:1u)==0;
  if(details){
   detail.context=c.rays;
   detail.begin=[](void* context,control_nrd::TimingStage stage,unsigned ordinal,unsigned pipeline,unsigned x,unsigned y,const char* shader) noexcept {
    namespace p=native_lighting::performance;
    p::Stage selected=p::Stage::RelaxDispatch;
    switch(stage){case control_nrd::TimingStage::Prepare:selected=p::Stage::RelaxPrepare;break;
     case control_nrd::TimingStage::Denoise:selected=p::Stage::RelaxDenoise;break;
     case control_nrd::TimingStage::Finish:selected=p::Stage::RelaxFinish;break;default:break;}
    const auto ticket=p::Begin(static_cast<ID3D12GraphicsCommandList4*>(context),selected,x,y,1);
    if(ticket!=p::kInvalid){p::samples[ticket].passOrdinal=ordinal;p::samples[ticket].pipelineIndex=pipeline;
     if(shader)for(unsigned i=0;i<95&&shader[i];++i)p::samples[ticket].passShader[i]=shader[i];}
    return ticket;
   };
   detail.end=[](void*,unsigned ticket) noexcept {native_lighting::performance::End(ticket);};
  }
  epoch->filter->BeginAttempt();
  if(!control_nrd::NativeTransaction::Record(*epoch->filter,c.rays,b,f,reinterpret_cast<UINT64>(c.queue),inputs,{}, {c.table.heap,0,0},*record->work,reserve,&reservation,&graphics,details?&detail:nullptr)){
   if(record->work->diffuse.commandsRecorded){epoch->poisoned=true;guideFrame=0;}
   else record->work.reset();
   Reason(epoch->filter->LastFailure());return false;
  }
  if(recovered)settings_check::Record(settings_check::RelaxHeapRecovered);
  settings_check::Record(settings_check::RelaxPasses,epoch->filter->DispatchCount());
  record->checkGeneration=settings_check::active.load()?settings_check::generation.load():0;settings_check::RecordFor(settings_check::RelaxRecorded,record->checkGeneration);
  record->filtered=true;lastFilterTicks=now.QuadPart;lastFilterFrame=f.frame;guideFrame=0;Reason("active");gi_dual_bridge::status=1;return true;
 }catch(...){Invalidate(epoch);Reason("filter_exception");return false;}
}
static const gi_dual_bridge::Callbacks callbacks{&Begin,&End,&Copy,&Constant,&Dispatch,&Reset,&Before,&After};
static const control_nrd_bridge::Producer producer{&ProducerBegin,&End,&ProducerDispatch};
static const bool registered=[] {hooks::EnableForDenoiser();gi_dual_bridge::callbacks=&callbacks;control_nrd_bridge::producer=&producer;return true;}();
}
#endif
