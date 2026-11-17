#pragma once
#ifdef CONTROL_GI_AUTO_TARGETS
#include "gi_reuse/d3d12_auto_invocation.h"
#include "gi_reuse/auto_temporal_audit.h"
#include "gi_reuse/d3d12_split_submission_router.h"
#include "gi_reuse/d3d12_native_gi_recording.h"
namespace gi_auto_runtime {
using Owner=gi_reuse::D3D12AutoInvocation;
using Router=gi_reuse::D3D12SplitSubmissionRouter<Owner,8,128ull*1024*1024>;
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
inline Microsoft::WRL::ComPtr<ID3D12Device> guideDevice;
inline gi_reuse::GIAutoGuideSession guideSession;
inline gi_reuse::GIAuditPairCache pixelPairs;
inline std::atomic<bool> halted{false},initialized{false};
inline std::atomic<UINT64> produced{0},shaded{0};
struct Pending {UINT64 scope=0,id=0,frame=0;UINT width=0,height=0;bool consumed=false;ID3D12GraphicsCommandList4* producer=nullptr;UINT64 generation=0;};
inline thread_local Pending pending{};
inline std::atomic<UINT64> reportedSkips{0};
inline bool Skip(unsigned bit,const char* reason)noexcept {
 const UINT64 mask=1ull<<bit;
 if(!(reportedSkips.load(std::memory_order_relaxed)&mask)&&!(reportedSkips.fetch_or(mask,std::memory_order_relaxed)&mask))
  Log("GI_AUTO_R454 skipped=%s once=1",reason);
 return false;
}
inline void Stop(const char* reason)noexcept {if(!halted.exchange(true))Log("GI_AUTO_R454 halted=1 reason=%s restart_required=1",reason);}
inline void Collect()noexcept {
 if(!initialized.load())return;
 std::array<std::unique_ptr<Owner>,8> retired;std::array<bool,8> submitted{};router->CollectReady(retired,&submitted);
 std::lock_guard<std::mutex> lock(mutex);
 for(unsigned i=0;i<retired.size();++i){auto& owner=retired[i];if(!submitted[i]||!owner||!owner->shaded)continue;
  if(owner->audit){
   void* mapped=nullptr;D3D12_RANGE read{0,2*sizeof(gi_reuse::GIAutoTargets)};
   const auto hr=owner->audit->Map(0,&read,&mapped);
   if(SUCCEEDED(hr)&&mapped){gi_reuse::GIAutoTargets t{};std::memcpy(&t,mapped,sizeof(t));gi_reuse::GIAutoTargets input{};std::memcpy(&input,static_cast<const unsigned char*>(mapped)+sizeof(t),sizeof(input));D3D12_RANGE empty{};owner->audit->Unmap(0,&empty);
    Log("GI_AUTO_R454 targets frame=%u mask=%u header_valid=%u count0=%u count1=%u score0=%u score1=%u",owner->output.frame,t.validMask,unsigned(gi_reuse::GIAutoMatches(t,owner->output.epoch,owner->output.frame)),t.counts[0],t.counts[1],t.scores[0],t.scores[1]);
    for(unsigned k=0;k<2;++k)if(t.validMask&(1u<<k))Log("GI_AUTO_R454 target frame=%u index=%u center=(%.6f,%.6f,%.6f) radius=%.6f",owner->output.frame,k,t.bounds[k].center.x,t.bounds[k].center.y,t.bounds[k].center.z,t.bounds[k].radius);
    Log("GI_AUTO_R454 input_targets frame=%u source=%u mask=%u usable=%u",owner->output.frame,owner->target.Selection().sourceFrame,input.validMask,unsigned(gi_reuse::GIAutoUsable(input,owner->target.Selection())));
    for(unsigned k=0;k<2;++k)if(input.validMask&(1u<<k))Log("GI_AUTO_R454 input_bound frame=%u index=%u center=(%.6f,%.6f,%.6f) radius=%.6f",owner->output.frame,k,input.bounds[k].center.x,input.bounds[k].center.y,input.bounds[k].center.z,input.bounds[k].radius);
   }else Log("GI_AUTO_R454 target_readback_failed=1 hr=0x%08lX",static_cast<unsigned long>(hr));
  }
  if(owner->pixelAudit){
   void* mapped=nullptr;D3D12_RANGE read{0,SIZE_T(owner->pixelBytes)};
   const auto hr=owner->pixelAudit->Map(0,&read,&mapped);
   if(SUCCEEDED(hr)&&mapped){
    try {
     const auto* pixels=static_cast<const gi_reuse::GIAutoPixelAudit*>(mapped);
     const auto w=gi_reuse::GIAuditWidth(owner->width),h=gi_reuse::GIAuditHeight(owner->height);
     const auto x=gi_reuse::GIAuditX(owner->width),y=gi_reuse::GIAuditY(owner->height);
     const auto s=gi_reuse::GIAuditSummarize(pixels,w,h);
     Log("GI_AUTO_R454 pixels frame=%u source=%u roi=(%u,%u,%u,%u) valid=%u guided=%u invalid=%u spikes=%u downweighted_spikes=%u raw_sum=%.9g weighted_sum=%.9g",owner->output.frame,owner->target.Selection().sourceFrame,x,y,w,h,s.valid,s.guided,s.invalid,s.spikes,s.downweightedSpikes,s.rawSum,s.weightedSum);
     Log("GI_AUTO_R454 small frame=%u floor=0.00001 ratio2=%u ratio4=%u ratio10=%u",owner->output.frame,s.small2,s.small4,s.small10);
     for(const auto& e:s.smallExamples)if(e.luminance>0){const auto& v=pixels[e.index];
      Log("GI_AUTO_R454 small_pixel frame=%u pixel=(%u,%u) luma=%.9g ratio=%.5g dominant_fraction=%.5g weight=%.9g material=%u ray=%u hit=(%.6f,%.6f,%.6f)",owner->output.frame,x+e.index%w,y+e.index/w,e.luminance,e.ratio,v.weightedPeak/e.luminance,v.weight,v.material,v.ray,v.position[0],v.position[1],v.position[2]);
     }
     gi_reuse::GIAuditTemporalSummary temporal;UINT first=0,second=0;
     if(pixelPairs.Add(owner->auditOrdinal,owner->output.frame,owner->output.epoch,w,h,pixels,temporal,first,second)){
      Log("GI_AUTO_R454 temporal first=%u second=%u motion_compensated=0 compared=%u invalid=%u changed25=%u changed50=%u changed100=%u mean_rgb_abs=%.9g max_rgb_abs=%.9g",first,second,temporal.compared,temporal.invalid,temporal.changed25,temporal.changed50,temporal.changed100,temporal.compared?temporal.rgbAbsSum/(3.0*temporal.compared):0,temporal.maxRgbDelta);
      for(const auto& e:temporal.examples)if(e.absolute>0){
       Log("GI_AUTO_R454 delta first=%u second=%u pixel=(%u,%u) rgb_abs=%.9g relative=%.5g before_rgb=(%.9g,%.9g,%.9g) after_rgb=(%.9g,%.9g,%.9g) before_weight=%.9g after_weight=%.9g before_material=%u after_material=%u",first,second,x+e.index%w,y+e.index/w,e.absolute,e.relative,e.before.weighted[0],e.before.weighted[1],e.before.weighted[2],e.after.weighted[0],e.after.weighted[1],e.after.weighted[2],e.before.weight,e.after.weight,e.before.material,e.after.material);
      }
     }
     for(const auto& e:s.examples)if(e.luminance>0){const auto& v=pixels[e.index];
      Log("GI_AUTO_R454 spike frame=%u pixel=(%u,%u) luma=%.9g ratio=%.5g dominant_fraction=%.5g raw_luma=%.9g raw_peak=%.9g weighted_peak=%.9g weight=%.9g material=%u ray=%u downweighted_rays=%u hit=(%.6f,%.6f,%.6f)",owner->output.frame,x+e.index%w,y+e.index/w,e.luminance,e.ratio,v.weightedPeak/e.luminance,gi_reuse::GIAuditLuma(v.raw),v.rawPeak,v.weightedPeak,v.weight,v.material,v.ray,v.covered,v.position[0],v.position[1],v.position[2]);
     }
    }catch(...){Log("GI_AUTO_R454 pixel_summary_failed=1");}
    D3D12_RANGE empty{};owner->pixelAudit->Unmap(0,&empty);
   }else Log("GI_AUTO_R454 pixel_readback_failed=1 hr=0x%08lX",static_cast<unsigned long>(hr));
  }
  if(!latest.buffer||owner->output.epoch!=latest.epoch||owner->output.frame>latest.frame)latest=owner->output;
 }
}
inline UINT64 Before(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists)noexcept{return initialized.load()?router->Before(q,count,lists):0;}
inline void After(ID3D12CommandQueue* q,UINT64 token,bool submitted)noexcept{if(initialized.load())router->After(q,token,submitted);}
inline void Seal(ID3D12CommandList* list,UINT64 generation)noexcept{if(initialized.load())router->Reset(list,generation);}
inline const gi_frozen_submission::Callbacks callbacks{&Before,&After};
inline void Leave(const gi_native_render_scope::Scope& scope,bool)noexcept {
 if(pending.id&&pending.scope==scope.token){if(!pending.consumed){router->CancelUnattached(pending.id);Stop("missing_matching_evaluator");}pending={};}
}
inline bool Ray(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,ID3D12StateObject* state)noexcept {
 if(control_diagnostics::Active())return Skip(0,"diagnostics_active");
 if(halted.load())return false;
 if(!gi_reuse::guidedRequested.load())return Skip(1,"gi_fixed_not_selected");
 if(!gi_reuse::singleEvaluatorReady.load())return Skip(2,"owned_evaluator_not_ready");
 if(!list||!desc||!state||native_lighting::gi_compute_hooks::Muted()||desc->Depth!=1)return false;
 if(pending.id)return Skip(3,"previous_pair_pending");
 auto scope=gi_native_render_scope::current;
 if(!scope.token||!scope.frame||scope.frame>UINT_MAX)return Skip(4,"native_render_scope_unavailable");
 UINT rays=0,pass=0,passes=0;
 if(!Provider("g_uRTDiffuseRayCount",rays))return Skip(5,"ray_count_provider");
 if(rays!=10)return Skip(6,"ray_count_not_ten");
 if(!Provider("g_uDGIPass",pass)||pass!=0)return Skip(7,"batch_not_zero");
 if(!Provider("g_uDGIPassCount",passes)||passes!=1)return Skip(8,"pass_count_not_one");
 try {
  gi_reuse::ComputeBindingSnapshot bindings;UINT64 ignored=0;
  if(!native_lighting::gi_compute_hooks::Capture(list,bindings,ignored)||bindings.rays.Get()!=state)return Skip(9,"producer_binding_snapshot");
  auto bundle=native_lighting::gi_auto_prepare::ForRoot(bindings.root.Get());if(!bundle)return Skip(10,"matching_evaluator_root_missing");
  gi_reuse::NativeGIRecording context;
  if(!gi_reuse::ReadCurrentNativeGIRecording(reinterpret_cast<const unsigned char*>(verifiedD3d),list,scope.frame,&context))return Skip(11,"native_recording_context");
  const auto generation=native_lighting::gi_compute_hooks::RecordingGeneration(list);
  if(!generation||!native_lighting::gi_compute_hooks::ObserveDestruction(list,generation))return Skip(12,"producer_recording_lifetime");
  UINT guideVersion=0;
  {std::lock_guard<std::mutex> lock(mutex);
   if(!initialized.load()){if(!native_lighting::rt_spot_table_audit::InstallGameQueue(context.queue))return Skip(13,"queue_observer");if(FAILED(router->Initialize(context.queue,&native_lighting::gi_compute_hooks::RecordingGeneration)))return Skip(14,"submission_router");queue=context.queue;guideDevice=context.device;initialized=true;}
   if(queue.Get()!=context.queue||guideDevice.Get()!=context.device)return Skip(15,"device_queue_identity");
   guideVersion=guideSession.Admit(reinterpret_cast<std::uintptr_t>(context.device),reinterpret_cast<std::uintptr_t>(context.queue));
   if(!guideVersion)return Skip(23,"guide_session_identity");
  }
  Collect();gi_reuse::GIAutoPublishedTarget prior;
  {std::lock_guard<std::mutex> lock(mutex);prior=latest;}
  const auto ordinal=produced.load();const bool auditPair=gi_reuse::GIAuditPairDue(ordinal);
  auto owner=std::make_unique<Owner>();
  if(FAILED(owner->Initialize(context.device,state,bundle.Get(),prior,desc->Width,desc->Height,UINT(scope.frame),guideVersion,ordinal<4||auditPair,auditPair)))return Skip(16,"invocation_or_ray_record_prepare");
  owner->auditOrdinal=ordinal;
  const auto record=owner->record.At(state);if(!record.StartAddress)return Skip(17,"ray_record_missing");
  const auto dispatch=native_lighting::rt_shadow_capture::Lookup(list).dispatch;if(!dispatch)return Skip(18,"native_ray_dispatch_missing");
  const auto selected=owner->target.Selection();
  const auto bytes=owner->Bytes();const auto id=router->Access([&](auto& pool){return pool.Admit(scope.frame,{reinterpret_cast<UINT64>(list),generation},bytes,owner);});
  if(!id)return Skip(19,"invocation_pool_full");
  pending={scope.token,id,scope.frame,desc->Width,desc->Height,false,list,generation};
  auto replacement=*desc;replacement.RayGenerationShaderRecord=record;
  native_lighting::gi_compute_hooks::Mute mute;dispatch(list,&replacement);
  const auto n=++produced;if(n<=4||n%600==0)Log("GI_AUTO_R454 producer=%llu frame=%llu target_source=%u guide_version=%u native_heap_epoch=%llu prior_frame=%u prior_version=%u rays=10 guide_fraction=0.4 estimator=stratified_balance",n,scope.frame,selected.sourceFrame,selected.version,static_cast<unsigned long long>(context.heaps.deviceEpoch),prior.frame,prior.epoch);
  return true;
 }catch(...){if(pending.id){Stop("producer_exception_after_admission");return true;}return Skip(20,"producer_prepare_exception");}
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
  const bool rootMatches=owner&&owner->pipelines->nativeRoot.Get()==bundle->nativeRoot.Get();
  if(!rootMatches||!pool.AttachAndClaimConsumer(pending.id,pending.frame,queue.Get(),{reinterpret_cast<UINT64>(list),generation})){
   Log("GI_AUTO_R454 pair_rejected=1 owner_present=%u root_matches=%u same_list=%u same_generation=%u",unsigned(owner!=nullptr),unsigned(rootMatches),unsigned(pending.producer==cmd.Get()),unsigned(pending.generation==generation));return false;
  }
  pending.consumed=true;native_lighting::gi_compute_hooks::Mute mute;return owner->Shade(cmd.Get(),bindings);
 });
 if(!ok)Stop("consumer_recording");
 else {const auto n=++shaded;if(n<=4||n%600==0)Log("GI_AUTO_R454 evaluator=%llu frame=%llu training_recorded=1",n,pending.frame);}
 return true;
}
inline bool Install()noexcept {
 if(!gi_auto_dispatch::Enabled()||!router||!gi_reuse::guidedScopeReady.load()||gi_native_render_scope::leaving.load()||
    gi_frozen_submission::callbacks.load()||native_lighting::gi_compute_hooks::onFrozenRecordingReset.load()||
    native_lighting::gi_compute_hooks::onFrozenRecordingDestroyed.load())return Skip(21,"install_preconditions");
 if(!provider.Initialize(verifiedRenderer,verifiedD3d))return Skip(22,"provider_initialization");
 native_lighting::gi_compute_hooks::EnableForDenoiser();
 native_lighting::gi_compute_hooks::onFrozenRecordingReset=&Seal;
 native_lighting::gi_compute_hooks::onFrozenRecordingDestroyed=&Seal;
 gi_frozen_submission::callbacks=&callbacks;gi_native_render_scope::leaving=&Leave;
 gi_auto_dispatch::ray=&Ray;gi_auto_dispatch::shade=&Shade;return true;
}
}
#endif
