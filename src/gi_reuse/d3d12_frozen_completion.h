#pragma once
#include "frozen_completion_result.h"
#include "d3d12_frozen_evaluator.h"
#include "d3d12_frozen_guide_build.h"
#include "d3d12_frozen_raygen.h"
#include "d3d12_frozen_working_set.h"
#include "frozen_ray_constants.h"
#include "d3d12_saved_heap_switch.h"
namespace gi_reuse {
// Actual GI6 replacement command sequence. The host supplies independently
// owned native bindings/SBT/scene and recording leases. This does not infer
// resource ownership from pointers. Must be called with observers muted.
// Only supports a previously submitted producer on the SAME ordered queue;
// its private buffers have decayed to COMMON at that Execute boundary.
class D3D12FrozenCompletion {
 static void Transition(ID3D12GraphicsCommandList4* list,ID3D12Resource* resource,
                        D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after)noexcept {
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
 }
public:
 using Trace=void (*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
 using Result=FrozenCompletionResult;
 struct Inputs {
  UINT width=0,height=0,slots=0,frame=0,epoch=0,trainingParity=0;
  // Host checks actual queue order plus immutable scene/descriptor ownership.
  // This explicit admission is required; existing pointer snapshots alone do
  // not qualify. No current game hook supplies this proof yet.
  bool separateProducerSubmitted=false,sceneAndDescriptorLease=false;
  // The restored full shader must reconstruct the same sampling basis.
  bool uniformShadeCompatible=false;
  ComputeBindingSnapshot ray,shade;
  GraphicsTableState graphics; // Current consumer graphics tables, not producer state.
  D3D12_DISPATCH_RAYS_DESC preservedTables{};
  D3D12_GPU_VIRTUAL_ADDRESS rayConstants=0;
  std::array<ID3D12Resource*,3> hits{};
  std::array<D3D12_RESOURCE_STATES,3> hitStates{};
  ID3D12Resource* output=nullptr;
 };
 // Shared admission for guided completion and uniform complement recovery.
 // All referenced scene data must still be leased; recovery cannot repair a
 // lost scene lease or missing submission. Different recorded heap pairs
 // require current graphics-table coverage for restoration after the switch.
 static bool AcceptReplay(const Inputs& p,const D3D12FrozenWorkingSet& work,
                          const D3D12FrozenRaygenRecords& records,Trace trace)noexcept {
  if(!trace||!p.separateProducerSubmitted||!p.sceneAndDescriptorLease||!p.ray.Ready()||!p.ray.rays||
     !p.shade.Ready()||!p.shade.pipeline||p.shade.rays||!work.Matches(p.width,p.height,p.slots)||
     !p.frame||!p.epoch||p.trainingParity>1||!FrozenRayConstants::Matches(p.ray.arguments,p.rayConstants)||
     !records.At(1).StartAddress||!records.NativeComplement().StartAddress)return false;
  if(!records.MatchesStateObject(p.ray.rays.Get()))return false;
  if(!records.Matches(p.width,p.height,p.frame,p.epoch,p.slots,p.trainingParity,
      work.Get(D3D12FrozenWorkingSet::Guides)->GetGPUVirtualAddress(),work.Get(D3D12FrozenWorkingSet::Weights)->GetGPUVirtualAddress()))return false;
  if(!D3D12SavedHeapSwitch::Accept(p.ray,p.shade,p.graphics))return false;
  const auto& d=p.preservedTables;
  if(d.Width!=p.width||d.Height!=p.height||d.Depth!=1||!d.MissShaderTable.StartAddress||
     d.MissShaderTable.SizeInBytes!=32||d.MissShaderTable.StrideInBytes!=32||
     !d.HitGroupTable.StartAddress||!d.HitGroupTable.SizeInBytes||d.HitGroupTable.StrideInBytes!=96||
     d.HitGroupTable.SizeInBytes%96||d.CallableShaderTable.SizeInBytes)return false;
  for(unsigned i=0;i<3;++i){
   if(!p.hits[i]||p.hits[i]==p.output)return false;
   for(unsigned j=0;j<i;++j)if(p.hits[i]==p.hits[j])return false;
   const auto h=p.hits[i]->GetDesc();
   if(h.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||h.Width!=p.width||h.Height!=p.height||
      h.DepthOrArraySize!=1||h.MipLevels!=1||!(h.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||
      !(p.hitStates[i]&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)||
      (p.hitStates[i]&~(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)))return false;
  }
  return true;
 }
 static bool Accept(const Inputs& p,const D3D12FrozenWorkingSet& work,
                    const D3D12FrozenEvaluator& evaluator,const D3D12FrozenGuideBuild& builder,
                    const D3D12FrozenRaygenRecords& records,Trace trace)noexcept {
  return GuidedRejection(p,work,evaluator,builder,records,trace)==nullptr;
 }
 static const char* GuidedRejection(const Inputs& p,const D3D12FrozenWorkingSet& work,
                    const D3D12FrozenEvaluator& evaluator,const D3D12FrozenGuideBuild& builder,
                    const D3D12FrozenRaygenRecords& records,Trace trace)noexcept {
  if(!AcceptReplay(p,work,records,trace))return "replay_contract";
  if(!evaluator.Accept(p.shade))return "evaluator_bindings";
  if(!builder.Ready())return "guide_builder";
  if(!p.output)return "output_acquisition";
  const auto o=p.output->GetDesc();
  if(o.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D)return "output_dimension";
  if(o.Width!=p.width||o.Height!=p.height)return "output_extent";
  if(o.DepthOrArraySize!=1||!o.MipLevels)return "output_subresources";
  if(!(o.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))return "output_uav";
  return nullptr;
 }

 // Call exactly once for an admitted consumer recording. This decides BEFORE
 // any completion commands. It never retries a partially recorded completion.
 // NativeShadeRequired means the uniform missing half is now traced and native
 // FULL GI6 shading must run once using the restored current shade bindings.
 // The native FULL ray pass must never run after either successful result.
 static Result RecordOrRecover(ID3D12GraphicsCommandList4* list,const Inputs& p,D3D12FrozenWorkingSet& work,
                      const D3D12FrozenEvaluator& evaluator,const D3D12FrozenGuideBuild& builder,
                      const D3D12FrozenRaygenRecords& records,Trace trace)noexcept {
  if(!list||(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))
   return Result::RejectedBeforeCommands;
  if(Accept(p,work,evaluator,builder,records,trace))
   return Record(list,p,work,evaluator,builder,records,trace);
  if(!p.uniformShadeCompatible||!AcceptReplay(p,work,records,trace))return Result::RejectedBeforeCommands;
  // The producer was submitted separately, so both private buffers are COMMON.
  // The third immutable raygen record disables guiding before any guide read.
  Transition(list,work.Get(D3D12FrozenWorkingSet::Weights),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(list,work.Get(D3D12FrozenWorkingSet::Guides),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  for(unsigned i=0;i<3;++i)Transition(list,p.hits[i],p.hitStates[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  if(!D3D12SavedHeapSwitch::ToRay(list,p.ray,p.shade,p.graphics))return Result::FailedAfterCommands;
  auto dispatch=p.preservedTables;dispatch.Width=p.width/2;dispatch.RayGenerationShaderRecord=records.NativeComplement();
  trace(list,&dispatch);
  for(unsigned i=0;i<3;++i)Transition(list,p.hits[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,p.hitStates[i]);
  if(!D3D12SavedHeapSwitch::ToShade(list,p.ray,p.shade,p.graphics))return Result::FailedAfterCommands;
  return Result::NativeShadeRequired;
 }
 static Result Record(ID3D12GraphicsCommandList4* list,const Inputs& p,D3D12FrozenWorkingSet& work,
                      const D3D12FrozenEvaluator& evaluator,const D3D12FrozenGuideBuild& builder,
                      const D3D12FrozenRaygenRecords& records,Trace trace)noexcept {
  if(!list||(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE)||
     !Accept(p,work,evaluator,builder,records,trace))return Result::RejectedBeforeCommands;
  auto* training=work.Get(D3D12FrozenWorkingSet::Training);auto* weights=work.Get(D3D12FrozenWorkingSet::Weights);
  auto* cells=work.Get(D3D12FrozenWorkingSet::Cells);auto* guides=work.Get(D3D12FrozenWorkingSet::Guides);
  Transition(list,training,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(list,weights,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  if(!evaluator.Record(list,p.shade,p.width,p.height,p.frame,p.epoch,p.trainingParity,0,
      p.rayConstants,training->GetGPUVirtualAddress(),weights->GetGPUVirtualAddress(),false))return Result::FailedAfterCommands;
  Transition(list,training,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(list,cells,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(list,guides,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  if(!builder.Record(list,training,work.Count(),cells,guides,p.slots,p.frame,p.epoch,p.shade))return Result::FailedAfterCommands;
  Transition(list,weights,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  for(unsigned i=0;i<3;++i)Transition(list,p.hits[i],p.hitStates[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  if(!D3D12SavedHeapSwitch::ToRay(list,p.ray,p.shade,p.graphics))return Result::FailedAfterCommands;
  auto dispatch=p.preservedTables;dispatch.Width=p.width/2;dispatch.RayGenerationShaderRecord=records.At(1);trace(list,&dispatch);
  for(unsigned i=0;i<3;++i)Transition(list,p.hits[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,p.hitStates[i]);
  Transition(list,weights,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  if(!D3D12SavedHeapSwitch::ToShade(list,p.ray,p.shade,p.graphics))return Result::FailedAfterCommands;
  // Phase B does not train, but the same shader still binds a training UAV.
  Transition(list,training,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=p.output;list->ResourceBarrier(1,&b);
  if(!evaluator.Record(list,p.shade,p.width,p.height,p.frame,p.epoch,p.trainingParity^1u,1,
      p.rayConstants,training->GetGPUVirtualAddress(),weights->GetGPUVirtualAddress(),true))return Result::FailedAfterCommands;
  // One full output exists only now. The host suppresses original GI6 only
  // on Recorded; FailedAfterCommands is a quarantine, NOT full-native retry.
  return Result::Recorded;
 }
};
}
