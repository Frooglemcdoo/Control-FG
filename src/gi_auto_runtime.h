#pragma once
#ifdef CONTROL_GI_AUTO_TARGETS
#include "gi_reuse/d3d12_auto_invocation.h"
#include "gi_reuse/d3d12_split_submission_router.h"
#include "gi_reuse/d3d12_native_gi_recording.h"
namespace gi_auto_runtime {
using Owner=gi_reuse::D3D12AutoInvocation;
using Router=gi_reuse::D3D12SplitSubmissionRouter<Owner,8,64ull*1024*1024>;
inline Router* const router=new(std::nothrow) Router;
inline std::mutex mutex;
inline control_rr_native_clamp::Api provider;
inline bool Provider(const char* name,UINT& value)noexcept {
 value=0;__try {if(!provider.ready||!provider.resolve)return false;
  struct Slot {int id,tag;} slot{-1,0};provider.resolve(&slot,name);return provider.ReadCurrent(slot.id,value);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
inline gi_reuse::GIAutoPublishedTarget latest;
inline std::atomic<bool> halted{false},initialized{false};
inline std::atomic<UINT64> produced{0},shaded{0};
struct Pending {UINT64 scope=0,id=0,frame=0;UINT width=0,height=0;bool consumed=false;};
inline thread_local Pending pending{};
inline void Stop(const char* reason)noexcept {if(!halted.exchange(true))Log("GI_AUTO_R449 halted=1 reason=%s restart_required=1",reason);}
inline void Collect()noexcept {
 if(!initialized.load())return;
 std::array<std::unique_ptr<Owner>,8> retired;std::array<bool,8> submitted{};router->CollectReady(retired,&submitted);
 std::lock_guard<std::mutex> lock(mutex);
 for(unsigned i=0;i<retired.size();++i){auto& owner=retired[i];if(submitted[i]&&owner&&owner->shaded&&(!latest.buffer||owner->output.epoch!=latest.epoch||owner->output.frame>latest.frame))latest=owner->output;}
}
inline UINT64 Before(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists)noexcept{return initialized.load()?router->Before(q,count,lists):0;}
inline void After(ID3D12CommandQueue* q,UINT64 token,bool submitted)noexcept{if(initialized.load())router->After(q,token,submitted);}
inline void Seal(ID3D12CommandList* list,UINT64 generation)noexcept{if(initialized.load())router->Reset(list,generation);}
inline const gi_frozen_submission::Callbacks callbacks{&Before,&After};
inline void Leave(const gi_native_render_scope::Scope& scope,bool)noexcept {
 if(pending.id&&pending.scope==scope.token){if(!pending.consumed){router->CancelUnattached(pending.id);Stop("missing_matching_evaluator");}pending={};}
}
inline bool Ray(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,ID3D12StateObject* state)noexcept {
 if(control_diagnostics::Active()||halted.load()||!gi_reuse::guidedRequested.load()||!gi_reuse::singleEvaluatorReady.load()||!list||!desc||!state||pending.id||
    native_lighting::gi_compute_hooks::Muted()||desc->Depth!=1)return false;
 auto scope=gi_native_render_scope::current;if(!scope.token||!scope.frame||scope.frame>UINT_MAX)return false;
 UINT rays=0,pass=0,passes=0;
 if(!Provider("g_uRTDiffuseRayCount",rays)||rays!=10||
    !Provider("g_uDGIPass",pass)||pass!=0||
    !Provider("g_uDGIPassCount",passes)||passes!=1)return false;
 try {
  gi_reuse::ComputeBindingSnapshot bindings;UINT64 ignored=0;
  if(!native_lighting::gi_compute_hooks::Capture(list,bindings,ignored)||bindings.rays.Get()!=state)return false;
  auto bundle=native_lighting::gi_auto_prepare::ForRoot(bindings.root.Get());if(!bundle)return false;
  gi_reuse::NativeGIRecording context;
  if(!gi_reuse::ReadCurrentNativeGIRecording(reinterpret_cast<const unsigned char*>(verifiedD3d),list,scope.frame,&context)||
     context.heaps.deviceEpoch>UINT_MAX)return false;
  const auto generation=native_lighting::gi_compute_hooks::RecordingGeneration(list);
  if(!generation||!native_lighting::gi_compute_hooks::ObserveDestruction(list,generation))return false;
  {std::lock_guard<std::mutex> lock(mutex);
   if(!initialized.load()){if(!native_lighting::rt_spot_table_audit::InstallGameQueue(context.queue))return false;if(FAILED(router->Initialize(context.queue,&native_lighting::gi_compute_hooks::RecordingGeneration)))return false;queue=context.queue;initialized=true;}
   if(queue.Get()!=context.queue)return false;
  }
  Collect();gi_reuse::GIAutoPublishedTarget prior;
  {std::lock_guard<std::mutex> lock(mutex);prior=latest;}
  auto owner=std::make_unique<Owner>();
  if(FAILED(owner->Initialize(context.device,state,bundle.Get(),prior,desc->Width,desc->Height,UINT(scope.frame),UINT(context.heaps.deviceEpoch))))return false;
  const auto record=owner->record.At(state);if(!record.StartAddress)return false;
  const auto dispatch=native_lighting::rt_shadow_capture::Lookup(list).dispatch;if(!dispatch)return false;
  const auto bytes=owner->Bytes();const auto id=router->Access([&](auto& pool){return pool.Admit(scope.frame,{reinterpret_cast<UINT64>(list),generation},bytes,owner);});
  if(!id)return false;
  pending={scope.token,id,scope.frame,desc->Width,desc->Height,false};
  auto replacement=*desc;replacement.RayGenerationShaderRecord=record;
  native_lighting::gi_compute_hooks::Mute mute;dispatch(list,&replacement);
  const auto n=++produced;if(n<=4||n%600==0)Log("GI_AUTO_R449 producer=%llu frame=%llu target_source=%u rays=10 guide_fraction=0.4 estimator=stratified_balance",n,scope.frame,prior.frame);
  return true;
 }catch(...){if(pending.id){Stop("producer_exception_after_admission");return true;}return false;}
}
inline bool Shade(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z)noexcept {
 if(!pending.id||pending.consumed||gi_native_render_scope::current.token!=pending.scope||!list||native_lighting::gi_compute_hooks::Muted())return false;
 UINT mode=0,modeBytes=sizeof(mode);
 if(FAILED(list->GetPrivateData(native_lighting::gi_evaluator::kMode,&modeBytes,&mode))||modeBytes!=sizeof(mode)||!mode)return false;
 // Identify GI6 before snapshot capture; never suppress unrelated compute work.
 gi_reuse::ComputeBindingSnapshot bindings;UINT64 ignored=0;
 if(!native_lighting::gi_compute_hooks::Capture(list,bindings,ignored)){
  Stop("consumer_binding_capture");return true;
 }
 auto bundle=native_lighting::gi_auto_prepare::ForPSO(bindings.pipeline.Get());if(!bundle)return false;
 if(z!=1||x!=(pending.width+7)/8||y!=(pending.height+15)/16||!bundle->evaluator.Accept(bindings)){Stop("consumer_contract");return true;}
 Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> cmd;
 if(FAILED(list->QueryInterface(IID_PPV_ARGS(&cmd)))){Stop("consumer_interface");return true;}
 const auto generation=native_lighting::gi_compute_hooks::RecordingGeneration(list);
 if(!generation||!native_lighting::gi_compute_hooks::ObserveDestruction(list,generation)){Stop("consumer_generation");return true;}
 const auto ok=router->Access([&](auto& pool){
  auto* owner=pool.View(pending.id);
  if(!owner||owner->pipelines->nativeRoot.Get()!=bundle->nativeRoot.Get()||!pool.AttachAndClaimConsumer(pending.id,pending.frame,queue.Get(),{reinterpret_cast<UINT64>(list),generation}))return false;
  pending.consumed=true;native_lighting::gi_compute_hooks::Mute mute;return owner->Shade(cmd.Get(),bindings);
 });
 if(!ok)Stop("consumer_recording");
 else {const auto n=++shaded;if(n<=4||n%600==0)Log("GI_AUTO_R449 evaluator=%llu frame=%llu training_recorded=1",n,pending.frame);}
 return true;
}
inline bool Install()noexcept {
 if(!gi_auto_dispatch::Enabled()||!router||!gi_reuse::guidedScopeReady.load()||gi_native_render_scope::leaving.load()||
    gi_frozen_submission::callbacks.load()||native_lighting::gi_compute_hooks::onFrozenRecordingReset.load()||
    native_lighting::gi_compute_hooks::onFrozenRecordingDestroyed.load())return false;
 if(!provider.Initialize(verifiedRenderer,verifiedD3d))return false;
 native_lighting::gi_compute_hooks::EnableForDenoiser();
 native_lighting::gi_compute_hooks::onFrozenRecordingReset=&Seal;
 native_lighting::gi_compute_hooks::onFrozenRecordingDestroyed=&Seal;
 gi_frozen_submission::callbacks=&callbacks;gi_native_render_scope::leaving=&Leave;
 gi_auto_dispatch::ray=&Ray;gi_auto_dispatch::shade=&Shade;return true;
}
}
#endif
