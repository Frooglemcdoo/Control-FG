#pragma once
#ifdef CONTROL_GI_LONG_SEQUENCE
#include "gi_frozen_lifetime.h"
#include "gi_frozen_dispatch_bridge.h"
#include <climits>
#include <type_traits>
// Included after the native lighting hooks and exact scope/window adapters.
// The coordinator does not certify scene immutability itself. Its installer
// requires the independent native scene contract; nullptr leaves it dormant.
namespace gi_frozen_runtime {
using Owner=gi_reuse::D3D12FrozenInvocation;
using Scope=gi_native_render_scope::Scope;
using Proof=bool(*)(const Scope&,const gi_reuse::NativeGIRecording&)noexcept;
inline Proof sceneProof=nullptr;
inline std::atomic<bool> failed{false},installed{false};
inline std::atomic<UINT64> rejected{0},produced{0},completed{0};
struct Pending {
 Scope scope{};UINT64 id=0;UINT width=0,height=0,epoch=0;
 gi_reuse::NativeGIRecording original{};
 bool consumed=false,producerReady=false;
};
inline thread_local Pending pending{};
inline void Reject(const char* reason,HRESULT hr=E_INVALIDARG)noexcept {
 const auto n=++rejected;
 if(n<=8||n%600==0)Log("GI_FROZEN_R428 rejected=%llu reason=%s hr=0x%08lX",n,reason,static_cast<unsigned long>(hr));
}
inline bool SameScope(const Scope& a,const Scope& b)noexcept {
 return a.token&&a.token==b.token&&a.frame==b.frame&&a.owner==b.owner&&a.pass==b.pass;
}
inline const void* Output()noexcept {
 __try {return verifiedRenderer?*reinterpret_cast<const void*const*>(reinterpret_cast<const unsigned char*>(verifiedRenderer)+0x90e9f8):nullptr;}
 __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
inline void Leave(const Scope& scope,bool)noexcept {
 if(!pending.id||!SameScope(pending.scope,scope))return;
 if(!pending.consumed)gi_frozen_lifetime::router->CancelUnattached(pending.id);
 pending={};
}
inline bool Ray(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,ID3D12StateObject* pipeline)noexcept {
 if(failed.load()||!sceneProof||!gi_reuse::ownedRequested.load()||!gi_reuse::singleEvaluatorReady.load()||!list||!desc||!pipeline||
    native_lighting::gi_compute_hooks::Muted()||!gi_reuse::SingleProducerEnabled())return false;
 const auto scope=gi_native_render_scope::current;
 if(!scope.token||!scope.frame||scope.frame>UINT_MAX||pending.id||desc->Depth!=1||desc->Width<2||(desc->Width&1))return false;
 UINT tag=0,size=sizeof(tag);
 if(FAILED(pipeline->GetPrivateData(gi_reuse::nativeGIVariantKey,&size,&tag))||size!=sizeof(tag)||tag!=1)return false;
 if(!gi_native_scene_window::TryAcquire()){Reject("scene_window");return false;}
 struct SceneGuard {UINT64 token;bool transferred=false;~SceneGuard(){if(!transferred)gi_native_scene_window::ReleaseAfterConsumerOrCancellation(token);}} guard{scope.token};
 bool suppressOnException=false;
 try {
  gi_reuse::NativeGIRecording original;
  if(!gi_reuse::ReadCurrentNativeGIRecording(reinterpret_cast<const unsigned char*>(verifiedD3d),list,scope.frame,&original)||
     !original.heaps.deviceEpoch||original.heaps.deviceEpoch>UINT_MAX||!sceneProof(scope,original)){
   Reject("native_scene_contract");return false;}
  gi_reuse::ComputeBindingSnapshot bindings;UINT64 bindingGeneration=0;
  if(!native_lighting::gi_compute_hooks::Capture(list,bindings,bindingGeneration)||bindings.rays.Get()!=pipeline){Reject("ray_bindings");return false;}
  const auto generation=native_lighting::gi_compute_hooks::RecordingGeneration(list);
  if(!generation){Reject("recording_generation");return false;}
  auto bundle=native_lighting::gi_frozen_prepare::AcquireForRay(bindings.root.Get());
  if(!bundle){Reject("pipelines_not_ready");return false;}
  GINativeJobRoute::Ticket writer;gi_reuse::NativeGIWriterInputs inputs;const char* reason=nullptr;
  if(!GINativeJobRuntime::SnapshotOriginalDispatch(list,desc,pipeline,writer,inputs,&reason)||writer.engineFrame!=scope.frame){Reject(reason?reason:"writer_frame");return false;}
  std::vector<gi_reuse::D3D12LiveSceneOwners::HitSource> acquired;
  if(!GINativeSceneOwners::owners.AcquireHitSources(inputs.bindings,acquired,&reason)){Reject(reason?reason:"hit_sources");return false;}
  std::vector<gi_reuse::D3D12NativeHitSnapshot::Source> sources;sources.reserve(acquired.size());
  for(auto& source:acquired)sources.push_back({source.token,std::move(source.storage),source.state});
  gi_reuse::D3D12LiveSceneOwners::Identity sbtIdentity;gi_reuse::BufferStorageLease sbt;UINT64 unusedWrapper=0;
  if(!GINativeSceneOwners::owners.Identify(writer.layout.buffer,writer.layout.bytes,sbtIdentity)||
     !GINativeSceneOwners::owners.Inspect(sbtIdentity.token,sbt,unusedWrapper)||
     sbt.address!=writer.layout.buffer||sbt.bytes!=writer.layout.bytes){Reject("native_sbt_storage");return false;}
  if(!gi_frozen_lifetime::Initialize(original.queue)){Reject("native_queue");return false;}
  Owner::Preparation preparation;
  preparation.device=original.device;preparation.pipelines=bundle.Get();preparation.nativeSbt=sbt.resource.Get();
  preparation.nativeOwner=scope.owner;preparation.recording=original;preparation.nativeHitTable=&inputs.table;
  preparation.nativeBindings=&inputs.bindings;preparation.hitSources=&sources;preparation.writerVersion=writer.serial;
  preparation.dispatch=*desc;preparation.budget=768ull*1024*1024;
  auto& ray=preparation.ray;ray.width=desc->Width;ray.height=desc->Height;ray.slots=65536;
  ray.frame=static_cast<UINT>(scope.frame);ray.epoch=static_cast<UINT>(original.heaps.deviceEpoch);ray.trainingParity=ray.frame&1;
  ray.nativeSingleRayAbi=true;ray.sceneAndDescriptorLease=true;ray.ray=std::move(bindings);
  std::unique_ptr<Owner> owner;const auto hr=gi_frozen_lifetime::PrepareOwner(preparation,owner);
  if(FAILED(hr)){Reject("preparation",hr);return false;}
  const auto trace=native_lighting::rt_shadow_capture::Lookup(list).dispatch;
  if(!sceneProof(scope,original)||!owner->AcceptProducer(list,trace)){Reject("producer_preflight");return false;}
  const auto id=gi_frozen_lifetime::Admit(scope,list,generation,owner);
  if(!id){Reject("owner_admission");return false;}
  guard.transferred=true;pending={scope,id,ray.width,ray.height,ray.epoch,original,false};
  suppressOnException=true;
  const auto result=gi_frozen_lifetime::router->Access([&](auto& pool){
   auto* active=pool.View(id);if(!active)return gi_reuse::D3D12FrozenProducer::Result::RejectedBeforeCommands;
   native_lighting::gi_compute_hooks::Mute mute;return active->RecordProducer(list,trace);
  });
  if(result==gi_reuse::D3D12FrozenProducer::Result::RejectedBeforeCommands){
   gi_frozen_lifetime::router->CancelUnattached(id);pending={};Reject("producer_admitted_rejection");return false;
  }
  if(result!=gi_reuse::D3D12FrozenProducer::Result::Recorded){failed=true;Reject("producer_partial_failure");return true;}
  pending.producerReady=true;
  gi_reuse::singleRayApplied.store(true);++gi_reuse::singleRayDispatches;
  settings_check::Record(settings_check::OwnedRays);
  const auto n=++produced;if(n<=4||n%600==0)Log("GI_FROZEN_R428 producer=%llu width=%u height=%u held_bytes=%llu native_ray_suppressed=1",
   n,ray.width,ray.height,gi_frozen_lifetime::cache->HeldBytes());
  return true;
 }catch(...){if(suppressOnException)failed=true;Reject("producer_exception",E_OUTOFMEMORY);return suppressOnException;}
}
inline bool Shade(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z)noexcept {
 if(!pending.id||pending.consumed||!SameScope(pending.scope,gi_native_render_scope::current)||
    !list||native_lighting::gi_compute_hooks::Muted())return false;
 gi_reuse::ComputeBindingSnapshot bindings;UINT64 bindingGeneration=0;
 if(!native_lighting::gi_compute_hooks::Capture(list,bindings,bindingGeneration)||!bindings.pipeline)return false;
 if(!native_lighting::gi_frozen_prepare::AcquireForShade(bindings.pipeline.Get()))return false; // Not native GI6.
 // Once any producer commands exist, ordinary GI6 must not consume an
 // incomplete hit set. Fatal failure disables future replacements; this frame
 // is aborted rather than pretending native shading can repair missing rays.
 if(!pending.producerReady){failed=true;Reject("consumer_after_failed_producer");return true;}
 if(x!=(pending.width+7)/8||y!=(pending.height+15)/16||z!=1||!sceneProof(pending.scope,pending.original)){
  failed=true;Reject("consumer_scope_or_contract");return true;}
 Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> rays;
 if(FAILED(list->QueryInterface(IID_PPV_ARGS(&rays)))){failed=true;Reject("consumer_interface");return true;}
 const auto generation=native_lighting::gi_compute_hooks::RecordingGeneration(list);
 if(!generation||!native_lighting::gi_compute_hooks::ObserveDestruction(list,generation)){failed=true;Reject("consumer_recording");return true;}
 Owner::Consumption input;input.nativeOwner=pending.scope.owner;input.outputContainer=Output();
 UINT shadeMode=0,shadeModeBytes=sizeof(shadeMode);
 input.uniformShadeCompatible=SUCCEEDED(bindings.pipeline->GetPrivateData(native_lighting::gi_evaluator::kMode,&shadeModeBytes,&shadeMode))&&
  shadeModeBytes==sizeof(shadeMode)&&shadeMode==2;
 // A mid-invocation toggle may bind native GI6. Its reconstruction basis is
 // not the owned ray basis, so it cannot shade a recovered uniform complement.

 input.producerSubmitted=true;input.sceneAndDescriptorLease=true;input.queue=pending.original.queue;
 input.frame=static_cast<UINT>(pending.scope.frame);input.epoch=pending.epoch;input.shade=std::move(bindings);
 native_lighting::gi_compute_hooks::CaptureGraphics(list,input.graphics);
 const auto trace=native_lighting::rt_shadow_capture::Lookup(rays.Get()).dispatch;
 const auto result=gi_frozen_lifetime::router->Access([&](auto& pool){
  using Result=gi_reuse::D3D12FrozenCompletion::Result;
  const typename std::remove_reference_t<decltype(pool)>::Recording recording{reinterpret_cast<UINT64>(list),generation};
  auto* owner=pool.View(pending.id);
  if(!owner||!pool.AttachAndClaimConsumer(pending.id,pending.scope.frame,pending.original.queue,recording))return Result::RejectedBeforeCommands;
  pending.consumed=true;native_lighting::gi_compute_hooks::Mute mute;
  return owner->RecordConsumer(rays.Get(),input,trace);
 });
 using Result=gi_reuse::D3D12FrozenCompletion::Result;
 if(result==Result::Recorded||result==Result::NativeShadeRequired){
  const auto n=++completed;if(n<=4||n%600==0)Log("GI_FROZEN_R428 consumer=%llu guided=%u native_complement=%u",n,
   unsigned(result==Result::Recorded),unsigned(result==Result::NativeShadeRequired));
  return gi_reuse::FrozenSuppressNativeShade(result);
 }
 failed=true;Reject(result==Result::FailedAfterCommands?"consumer_partial_failure":"consumer_rejected");
 return gi_reuse::FrozenSuppressNativeShade(result);
}
inline bool Install(Proof proof)noexcept {
 if(installed.load())return proof&&proof==sceneProof;
 if(!proof||!gi_frozen_lifetime::installed.load()||!GINativeSceneOwners::ready.load()||
    !gi_reuse::runtime::FrozenSourcesEnabled()||gi_frozen_dispatch::ray.load()||
    gi_frozen_dispatch::shade.load()||gi_native_render_scope::leaving.load())return false;
 // Startup-only installation, before any native render workers begin.
 sceneProof=proof;gi_native_render_scope::leaving.store(&Leave);
 gi_frozen_dispatch::ray.store(&Ray);gi_frozen_dispatch::shade.store(&Shade);installed=true;return true;
}
}
#endif
