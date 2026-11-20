#pragma once
#ifdef CONTROL_GI_AUTO_TARGETS
#include "gi_reuse/d3d12_auto_invocation.h"
#include "gi_reuse/auto_temporal_audit.h"
#include "gi_reuse/scene_capture_grid.h"
#include "gi_scene_detail_export.h"
#include "gi_reuse/d3d12_split_submission_router.h"
#include "gi_reuse/d3d12_native_gi_recording.h"
namespace gi_auto_runtime {
using Owner=gi_reuse::D3D12AutoInvocation;
using Router=gi_reuse::D3D12SplitSubmissionRouter<Owner,8,gi_motion::MaxOwnerBudget>;
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
inline std::array<std::unique_ptr<gi_reuse::D3D12AutoScratch>,2> scratchCache;
inline std::atomic<UINT64> scratchHits{0},scratchMisses{0};
inline UINT64 timingFrequency=0;
inline UINT64 pixelPairRequest=0;
inline UINT64 captureRequest=0,captureOrdinal=0;
inline std::atomic<bool> halted{false},initialized{false};
inline std::atomic<UINT64> produced{0},shaded{0};
inline std::atomic<UINT64> bypassRays{0},bypassEvaluators{0};
inline void Bypassed(bool evaluator)noexcept {
 const auto n=evaluator?++bypassEvaluators:++bypassRays;
 if(evaluator&&(n<=4||n%600==0))Log("GI_AUTO_R484 bypass native_rays=%llu native_evaluators=%llu legacy_after=%llu",bypassRays.load(),bypassEvaluators.load(),bypassEvaluators.load());
}
struct Pending {UINT64 scope=0,id=0,frame=0;UINT width=0,height=0;bool consumed=false;ID3D12GraphicsCommandList4* producer=nullptr;UINT64 generation=0;};
inline thread_local Pending pending{};
inline std::atomic<UINT64> reportedSkips{0};
inline bool Skip(unsigned bit,const char* reason)noexcept {
 const UINT64 mask=1ull<<bit;
 if(!(reportedSkips.load(std::memory_order_relaxed)&mask)&&!(reportedSkips.fetch_or(mask,std::memory_order_relaxed)&mask))
  Log("GI_AUTO_R484 skipped=%s once=1",reason);
 return false;
}
inline void Stop(const char* reason)noexcept {if(!halted.exchange(true))Log("GI_AUTO_R484 halted=1 reason=%s restart_required=1",reason);}
inline void Collect()noexcept {
 if(!initialized.load())return;
 std::array<std::unique_ptr<Owner>,8> retired;std::array<bool,8> submitted{};router->CollectReady(retired,&submitted);
 std::lock_guard<std::mutex> lock(mutex);
 for(unsigned i=0;i<retired.size();++i){auto& owner=retired[i];if(!submitted[i]||!owner||!owner->shaded)continue;
  gi_gpu_fault::retiredFrame=owner->output.frame;
  if(owner->timing.Ready()){
   std::array<double,3> ms{};
   if(owner->timing.ReadRetired(timingFrequency,ms))Log("GI_AUTO_R484 gpu_timing frame=%u rays=%u ray_ms=%.6f evaluator_ms=%.6f guide_ms=%.6f sampled_every=120 capture=0",owner->output.frame,owner->target.Selection().rayCount,ms[0],ms[1],ms[2]);
   else Log("GI_AUTO_R484 gpu_timing_read_failed frame=%u",owner->output.frame);
  }
#ifdef CONTROL_GI_REGIONAL
  if(owner->audit){
   void* mapped=nullptr;D3D12_RANGE read{0,2*gi_reuse::GIAutoPublicationBytes};
   if(SUCCEEDED(owner->audit->Map(0,&read,&mapped))&&mapped){
    const auto* tables=static_cast<const gi_reuse::GIRegionalTarget*>(mapped);unsigned outputRegions=0,inputRegions=0,fitted=0;double radiusSum=0;float minRadius=1;
    for(unsigned slot=0;slot<4096;++slot){outputRegions+=tables[slot].targets.validMask!=0;inputRegions+=tables[4096+slot].targets.validMask!=0;const auto& t=tables[4096+slot].targets;if(t.validMask&1){radiusSum+=t.bounds[0].radius;minRadius=(std::min)(minRadius,t.bounds[0].radius);fitted+=t.bounds[0].radius<.4329f;}}
    Log("GI_AUTO_R484 regional frame=%u source=%u output_regions=%u input_regions=%u slots=4096",owner->output.frame,owner->target.Selection().sourceFrame,outputRegions,inputRegions);
    Log("GI_AUTO_R484 primary_fit frame=%u input_regions=%u fitted=%u mean_radius=%.6f min_radius=%.6f",owner->output.frame,inputRegions,fitted,inputRegions?radiusSum/inputRegions:0,inputRegions?double(minRadius):0);
    D3D12_RANGE empty{};owner->audit->Unmap(0,&empty);
   }
  }
#else
  if(owner->audit){
   void* mapped=nullptr;D3D12_RANGE read{0,2*sizeof(gi_reuse::GIAutoTargets)};
   const auto hr=owner->audit->Map(0,&read,&mapped);
   if(SUCCEEDED(hr)&&mapped){gi_reuse::GIAutoTargets t{};std::memcpy(&t,mapped,sizeof(t));gi_reuse::GIAutoTargets input{};std::memcpy(&input,static_cast<const unsigned char*>(mapped)+sizeof(t),sizeof(input));D3D12_RANGE empty{};owner->audit->Unmap(0,&empty);
    Log("GI_AUTO_R484 targets frame=%u mask=%u header_valid=%u count0=%u count1=%u score0=%u score1=%u",owner->output.frame,t.validMask,unsigned(gi_reuse::GIAutoMatches(t,owner->output.epoch,owner->output.frame)),t.counts[0],t.counts[1],t.scores[0],t.scores[1]);
    for(unsigned k=0;k<2;++k)if(t.validMask&(1u<<k))Log("GI_AUTO_R484 target frame=%u index=%u center=(%.6f,%.6f,%.6f) radius=%.6f",owner->output.frame,k,t.bounds[k].center.x,t.bounds[k].center.y,t.bounds[k].center.z,t.bounds[k].radius);
    Log("GI_AUTO_R484 input_targets frame=%u source=%u mask=%u usable=%u",owner->output.frame,owner->target.Selection().sourceFrame,input.validMask,unsigned(gi_reuse::GIAutoUsable(input,owner->target.Selection())));
    for(unsigned k=0;k<2;++k)if(input.validMask&(1u<<k))Log("GI_AUTO_R484 input_bound frame=%u index=%u center=(%.6f,%.6f,%.6f) radius=%.6f",owner->output.frame,k,input.bounds[k].center.x,input.bounds[k].center.y,input.bounds[k].center.z,input.bounds[k].radius);
   }else Log("GI_AUTO_R484 target_readback_failed=1 hr=0x%08lX",static_cast<unsigned long>(hr));
  }
#endif
  if(owner->pixelAudit){
   if(owner->sceneAudit)gi_scene_capture::QueueDetail(owner->pixelAudit.Get(),owner->pixelBytes,owner->output.frame,owner->width,owner->height,owner->target.Selection().rayCount,owner->pairAudit.Get(),owner->pairBytes,owner->audit.Get(),owner->target.Selection(),owner->motionAudit.Get(),owner->motionBytes);
   void* mapped=nullptr;D3D12_RANGE read{0,SIZE_T(owner->pixelBytes)};
   const auto hr=owner->pixelAudit->Map(0,&read,&mapped);
   if(SUCCEEDED(hr)&&mapped){
    try {
     const auto* pixels=static_cast<const gi_reuse::GIAutoPixelAudit*>(mapped);
     const auto w=gi_reuse::GIAuditWidth(owner->width),h=gi_reuse::GIAuditHeight(owner->height);
     const auto x=owner->roiX,y=owner->roiY;
     const auto px=[&](unsigned index){return owner->sceneAudit?gi_reuse::GISceneCoordinate(index%w,owner->width,w):x+index%w;};
     const auto py=[&](unsigned index){return owner->sceneAudit?gi_reuse::GISceneCoordinate(index/w,owner->height,h):y+index/w;};
     if(owner->sceneAudit)Log("GI_SCENE_R484 detail_grid frame=%u width=%u height=%u columns=%u rows=%u full_resolution_images=1",owner->output.frame,owner->width,owner->height,w,h);
     const auto s=gi_reuse::GIAuditSummarize(pixels,w,h);
     Log("GI_AUTO_R484 pixels frame=%u source=%u roi=(%u,%u,%u,%u) valid=%u guided=%u invalid=%u spikes=%u downweighted_spikes=%u raw_sum=%.9g weighted_sum=%.9g",owner->output.frame,owner->target.Selection().sourceFrame,x,y,w,h,s.valid,s.guided,s.invalid,s.spikes,s.downweightedSpikes,s.rawSum,s.weightedSum);
     Log("GI_AUTO_R484 small frame=%u floor=0.00001 ratio2=%u ratio4=%u ratio10=%u",owner->output.frame,s.small2,s.small4,s.small10);
     Log("GI_AUTO_R484 ray_contract frame=%u sample_frame=%u peak_direction_mismatch=%u peak_inside_uncovered=%u",owner->output.frame,owner->target.Selection().sampleFrame,s.peakDirectionMismatch,s.peakInsideUncovered);
     Log("GI_AUTO_R484 producer_contract frame=%u stamp_mismatch=%u weight_mismatch=%u direction_mismatch=%u hit_direction_mismatch=%u",owner->output.frame,s.producerStampMismatch,s.producerWeightMismatch,s.producerDirectionMismatch,s.producerHitMismatch);
     for(const auto& e:s.smallExamples)if(e.luminance>0){const auto& v=pixels[e.index];
      Log("GI_AUTO_R484 small_pixel frame=%u pixel=(%u,%u) luma=%.9g ratio=%.5g dominant_fraction=%.5g weight=%.9g material=%u ray=%u hit=(%.6f,%.6f,%.6f)",owner->output.frame,px(e.index),py(e.index),e.luminance,e.ratio,v.weightedPeak/e.luminance,v.weight,v.material,v.ray,v.position[0],v.position[1],v.position[2]);
     }
     if(owner->captureRequest>pixelPairRequest){pixelPairs={};pixelPairRequest=owner->captureRequest;}
     gi_reuse::GIAuditTemporalSummary temporal;UINT first=0,second=0;
     if(!owner->sceneAudit&&owner->captureRequest==pixelPairRequest&&pixelPairs.Add(owner->auditOrdinal,owner->output.frame,owner->output.epoch,w,h,pixels,temporal,first,second,x,y)){
      Log("GI_AUTO_R484 temporal first=%u second=%u motion_compensated=0 compared=%u invalid=%u changed25=%u changed50=%u changed100=%u mean_rgb_abs=%.9g max_rgb_abs=%.9g",first,second,temporal.compared,temporal.invalid,temporal.changed25,temporal.changed50,temporal.changed100,temporal.compared?temporal.rgbAbsSum/(3.0*temporal.compared):0,temporal.maxRgbDelta);
      for(const auto& e:temporal.examples)if(e.absolute>0){
       Log("GI_AUTO_R484 delta first=%u second=%u pixel=(%u,%u) rgb_abs=%.9g relative=%.5g before_rgb=(%.9g,%.9g,%.9g) after_rgb=(%.9g,%.9g,%.9g) before_weight=%.9g after_weight=%.9g before_material=%u after_material=%u",first,second,px(e.index),py(e.index),e.absolute,e.relative,e.before.weighted[0],e.before.weighted[1],e.before.weighted[2],e.after.weighted[0],e.after.weighted[1],e.after.weighted[2],e.before.weight,e.after.weight,e.before.material,e.after.material);
      }
     }
     for(const auto& e:s.examples)if(e.luminance>0){const auto& v=pixels[e.index];
#ifdef CONTROL_GI_REGIONAL
      const auto* packets=reinterpret_cast<const float*>(pixels+size_t(w)*h);
      const UINT rays=owner->target.Selection().rayCount;
      const auto* origin=packets+size_t(e.index)*(rays+1)*4;
      UINT stamp=0;std::memcpy(&stamp,origin+3,4);
      if(stamp==owner->target.Selection().sampleFrame&&v.ray<rays){const auto* direction=origin+4*(v.ray+1);
       Log("GI_AUTO_R484 producer_peak frame=%u pixel=(%u,%u) flags=%u ray=%u origin=(%.9g,%.9g,%.9g) direction=(%.9g,%.9g,%.9g) producer_weight=%.9g evaluator_weight=%.9g",owner->output.frame,px(e.index),py(e.index),v.flags,v.ray,origin[0],origin[1],origin[2],direction[0],direction[1],direction[2],direction[3],v.weight);
      }
#endif
      Log("GI_AUTO_R484 spike frame=%u pixel=(%u,%u) luma=%.9g ratio=%.5g dominant_fraction=%.5g raw_luma=%.9g raw_peak=%.9g weighted_peak=%.9g weight=%.9g material=%u ray=%u downweighted_rays=%u hit=(%.6f,%.6f,%.6f)",owner->output.frame,px(e.index),py(e.index),e.luminance,e.ratio,v.weightedPeak/e.luminance,gi_reuse::GIAuditLuma(v.raw),v.rawPeak,v.weightedPeak,v.weight,v.material,v.ray,v.covered,v.position[0],v.position[1],v.position[2]);
     }
#ifdef CONTROL_GI_REGIONAL
     const UINT redRays=owner->target.Selection().rayCount;
     const auto* redBase=reinterpret_cast<const gi_reuse::GIAutoRedAudit*>(reinterpret_cast<const unsigned char*>(pixels+size_t(w)*h)+gi_reuse::GIAuditProducerBytes(w,h,redRays));
     Log("GI_AUTO_R484 red_summary frame=%u red_outliers=%u criterion=R_gt_1.25_maxGB_local_R_ratio_gt_2 luma_floor=0.00001 rr_stage=not_captured",owner->output.frame,s.redOutliers);
     for(const auto& e:s.redExamples)if(e.luminance>0){const auto& v=pixels[e.index];const auto& red=redBase[e.index];
      Log("GI_AUTO_R484 red_trace frame=%u pixel=(%u,%u) local_red_ratio=%.6g material=%u ray=%u raw_ray_rgb=(%.9g,%.9g,%.9g) secondary_rgb=(%.9g,%.9g,%.9g) throughput_rgb=(%.9g,%.9g,%.9g) weight=%.9g fade=%.9g hit=(%.6f,%.6f,%.6f) raw_mean_rgb=(%.9g,%.9g,%.9g) final_gi_rgb=(%.9g,%.9g,%.9g)",owner->output.frame,px(e.index),py(e.index),e.ratio,red.material,red.ray,red.raw[0],red.raw[1],red.raw[2],red.secondary[0],red.secondary[1],red.secondary[2],red.throughput[0],red.throughput[1],red.throughput[2],red.weight,red.scale,red.position[0],red.position[1],red.position[2],v.raw[0],v.raw[1],v.raw[2],v.weighted[0],v.weighted[1],v.weighted[2]);
      const char* termNames[]={"texture","tint","volume","surface","point","spot","sentinel","brdf"};
      for(unsigned term=0;term<8;++term)Log("GI_AUTO_R484 red_term frame=%u pixel=(%u,%u) ray=%u term=%s rgb=(%.9g,%.9g,%.9g) aux=%.9g",owner->output.frame,px(e.index),py(e.index),red.ray,termNames[term],red.terms[term][0],red.terms[term][1],red.terms[term][2],red.terms[term][3]);
      const auto* packets=reinterpret_cast<const float*>(pixels+size_t(w)*h);const auto* origin=packets+size_t(e.index)*(redRays+1)*4;UINT stamp=0;std::memcpy(&stamp,origin+3,4);
      if(stamp==owner->target.Selection().sampleFrame&&red.ray<redRays){const auto* direction=origin+4*(red.ray+1);
       Log("GI_AUTO_R484 red_producer frame=%u pixel=(%u,%u) ray=%u origin=(%.9g,%.9g,%.9g) direction=(%.9g,%.9g,%.9g) producer_weight=%.9g evaluator_weight=%.9g",owner->output.frame,px(e.index),py(e.index),red.ray,origin[0],origin[1],origin[2],direction[0],direction[1],direction[2],direction[3],red.weight);
      }
     }
#endif
     if(owner->captureRequest==gi_auto_capture::request.load()){
      const auto done=++gi_auto_capture::completed;
      if(done>=10)gi_auto_capture::phase.store(gi_auto_capture::Complete);
     }
    }catch(...){Log("GI_AUTO_R484 pixel_summary_failed=1");}
    D3D12_RANGE empty{};owner->pixelAudit->Unmap(0,&empty);
   }else Log("GI_AUTO_R484 pixel_readback_failed=1 hr=0x%08lX",static_cast<unsigned long>(hr));
  }
  if(!latest.buffer||owner->output.epoch!=latest.epoch||owner->output.frame>latest.frame)latest=owner->output;
  // CollectReady proves completion and both recording seals. Published output
  // stays immutable; only private training/cells/ray-record storage is recycled.
  auto scratch=owner->ReleaseRetiredScratch();
  if(scratch)for(auto& slot:scratchCache)if(!slot){slot=std::move(scratch);break;}
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
 if(control_diagnostics::BlocksOwnedGI(gi_scene_capture::Enabled()))return Skip(0,"diagnostics_active");
 if(halted.load())return false;
 if(!gi_reuse::ownedRequested.load())return Skip(1,"owned_gi_not_selected");
 if(!gi_reuse::guidedRequested.load())return Skip(1,"gi_fixed_not_selected");
 if(!gi_reuse::singleEvaluatorReady.load())return Skip(2,"owned_evaluator_not_ready");
 if(!list||!desc||!state||native_lighting::gi_compute_hooks::Muted()||desc->Depth!=1)return false;
 if(pending.id)return Skip(3,"previous_pair_pending");
 auto scope=gi_native_render_scope::current;
 if(!scope.token||!scope.frame||scope.frame>UINT_MAX)return Skip(4,"native_render_scope_unavailable");
 UINT rays=0,pass=0,passes=0;
 if(!Provider("g_uRTDiffuseRayCount",rays))return Skip(5,"ray_count_provider");
 if(rays!=10&&rays!=4)return Skip(6,"ray_count_not_four_or_ten");
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
   if(!initialized.load()){if(!native_lighting::rt_spot_table_audit::InstallGameQueue(context.queue))return Skip(13,"queue_observer");if(FAILED(router->Initialize(context.queue,&native_lighting::gi_compute_hooks::RecordingGeneration,&gi_gpu_fault::Signal)))return Skip(14,"submission_router");queue=context.queue;guideDevice=context.device;gi_gpu_fault::Attach(context.device);if(FAILED(queue->GetTimestampFrequency(&timingFrequency)))timingFrequency=0;Log("GI_AUTO_R484 timing_frequency=%llu",timingFrequency);initialized=true;}
   if(queue.Get()!=context.queue||guideDevice.Get()!=context.device)return Skip(15,"device_queue_identity");
   guideVersion=guideSession.Admit(reinterpret_cast<std::uintptr_t>(context.device),reinterpret_cast<std::uintptr_t>(context.queue));
   if(!guideVersion)return Skip(23,"guide_session_identity");
  }
  Collect();gi_reuse::GIAutoPublishedTarget prior;
  {std::lock_guard<std::mutex> lock(mutex);prior=latest;}
  gi_auto_capture::dimensions.store(UINT64(desc->Width)|(UINT64(desc->Height)<<32));
  const auto requestedCapture=gi_auto_capture::request.load();
  const bool scene=gi_scene_capture::Enabled();
  const bool collecting=scene?control_rr_capture_ui::Busy():gi_auto_capture::phase.load()==gi_auto_capture::Collecting;
  if(collecting&&requestedCapture!=captureRequest){captureRequest=requestedCapture;captureOrdinal=0;}
  const auto ordinal=captureOrdinal;const bool auditPair=scene?RRSceneWantsGI(scope.frame):collecting&&gi_reuse::GIAuditPairDue(ordinal);
  const auto roi=scene?gi_reuse::GIAuditRegion(desc->Width,desc->Height,0,0):gi_auto_capture::Region(desc->Width,desc->Height);
  std::unique_ptr<gi_reuse::D3D12AutoScratch> scratch;
  if(!auditPair){std::lock_guard<std::mutex> lock(mutex);
   for(auto& slot:scratchCache)if(slot&&slot->device.Get()==context.device&&slot->width==desc->Width&&slot->height==desc->Height&&slot->rays==rays){scratch=std::move(slot);break;}
  }
  auto owner=std::make_unique<Owner>();owner->roiX=roi.x;owner->roiY=roi.y;
  if(FAILED(owner->Initialize(context.device,state,bundle.Get(),prior,desc->Width,desc->Height,UINT(scope.frame),guideVersion,produced.load()<4||auditPair,auditPair,rays,std::move(scratch),scene,gi_scene_capture::TextureMipOne())))return Skip(16,"invocation_or_ray_record_prepare");
  if(timingFrequency&&!collecting&&produced.load()%120==0&&!owner->timing.Initialize(context.device))Skip(24,"timing_allocation_failed");
  if(owner->reusedScratch)++scratchHits;else ++scratchMisses;
  owner->auditOrdinal=ordinal;owner->captureRequest=captureRequest;owner->roiX=roi.x;owner->roiY=roi.y;
  const auto record=owner->record.At(state);if(!record.StartAddress)return Skip(17,"ray_record_missing");
  const auto dispatch=native_lighting::rt_shadow_capture::Lookup(list).dispatch;if(!dispatch)return Skip(18,"native_ray_dispatch_missing");
  const auto selected=owner->target.Selection();
  auto* activeOwner=owner.get();
  const auto bytes=owner->Bytes();
  const auto budget=gi_scene_capture::Motion()?gi_motion::OwnerBudget(desc->Width,desc->Height,rays):gi_motion::NormalOwnerBudget;
  const auto id=router->Access([&](auto& pool){
   const auto reason=pool.CheckAdmission(scope.frame,{reinterpret_cast<UINT64>(list),generation},bytes,bool(owner),budget);
   using A=decltype(reason);
   if(reason!=A::Ready){
    Log("GI_CAPTURE_R490 admission_failed frame=%llu reason=%u owner_bytes=%llu retained_bytes=%llu retained_owners=%u budget=%llu",scope.frame,unsigned(reason),bytes,pool.RetainedBytes(),pool.RetainedOwners(),budget);
    if(auditPair&&gi_scene_capture::Motion()){
     gi_scene_capture::motionFailed=true;
     control_rr_capture_ui::detail=reason==A::RetainedBudget?L"Motion capture failed: retained GI memory budget exceeded. Keep files and collect logs.":reason==A::Slots?L"Motion capture failed: GI recording slots have not retired. Keep files and collect logs.":L"Motion capture failed: GI admission rejected. Keep files and collect logs.";
    }
    return UINT64(0);
   }
   return pool.Admit(scope.frame,{reinterpret_cast<UINT64>(list),generation},bytes,owner,budget);
  });
  if(!id)return Skip(19,"invocation_pool_full");
  pending={scope.token,id,scope.frame,desc->Width,desc->Height,false,list,generation};
  auto replacement=*desc;replacement.RayGenerationShaderRecord=record;
  gi_gpu_fault::Track(scope.frame,list,false);
  native_lighting::gi_compute_hooks::Mute mute;activeOwner->PrepareProducer(list);activeOwner->timing.Mark(list,0);dispatch(list,&replacement);activeOwner->timing.Mark(list,1);activeOwner->timing.Resolve(list,0,2);
  if(collecting){++captureOrdinal;gi_auto_capture::recorded.store(captureOrdinal);}
  gi_gpu_fault::recordedFrame=scope.frame;
  const auto n=++produced;if(n<=32||n%600==0)Log("GI_AUTO_R484 producer=%llu frame=%llu target_source=%u guide_version=%u native_heap_epoch=%llu prior_frame=%u prior_version=%u rays=%u guide_fraction=%.2f scratch_reused=%llu scratch_fresh=%llu estimator=stratified_balance",n,scope.frame,selected.sourceFrame,selected.version,static_cast<unsigned long long>(context.heaps.deviceEpoch),prior.frame,prior.epoch,rays,double(selected.fraction),scratchHits.load(),scratchMisses.load());
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
 bool sceneDetails=false;unsigned sceneRays=0;
 const auto ok=router->Access([&](auto& pool){
  auto* owner=pool.View(pending.id);
  const bool rootMatches=owner&&owner->pipelines->nativeRoot.Get()==bundle->nativeRoot.Get();
  if(!rootMatches||!pool.AttachAndClaimConsumer(pending.id,pending.frame,queue.Get(),{reinterpret_cast<UINT64>(list),generation})){
   Log("GI_AUTO_R484 pair_rejected=1 owner_present=%u root_matches=%u same_list=%u same_generation=%u",unsigned(owner!=nullptr),unsigned(rootMatches),unsigned(pending.producer==cmd.Get()),unsigned(pending.generation==generation));return false;
  }
  sceneDetails=owner->sceneAudit&&owner->pixelAudit;sceneRays=owner->target.Selection().rayCount;
  pending.consumed=true;gi_gpu_fault::Track(pending.frame,list,true);native_lighting::gi_compute_hooks::Mute mute;return owner->Shade(cmd.Get(),bindings);
 });
 if(ok&&sceneDetails){
  gi_scene_capture::detailedFrame.store(pending.frame);
  RRInputCaptureGIBatch(pending.frame,list,0,1,sceneRays,true);
 }
 if(!ok)Stop("consumer_recording");
 else {const auto n=++shaded;if(n<=32||n%600==0)Log("GI_AUTO_R484 evaluator=%llu frame=%llu training_recorded=1",n,pending.frame);}
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
 gi_auto_dispatch::bypassed=&Bypassed;gi_auto_dispatch::ray=&Ray;gi_auto_dispatch::shade=&Shade;return true;
}
}
#endif
