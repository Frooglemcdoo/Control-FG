#pragma once
#include "d3d12_frozen_raygen.h"
#include "d3d12_frozen_working_set.h"
#include "d3d12_ray_constant_snapshot.h"
#include "d3d12_sbt_snapshot.h"
namespace gi_reuse {
// Phase A runs at the original native ray site, with the native hit textures
// already in UAV state. All storage is prepared BEFORE admission. The host
// retains the whole invocation through both recording resets and final GPU
// completion, mutes observers, and supplies the original DispatchRays function.
// This class neither creates a native scene lease nor suppresses any game call.
class D3D12FrozenProducer {
public:
 using Trace=void (*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
 enum class Result {RejectedBeforeCommands,Recorded,FailedAfterCommands};
 struct Inputs {
  UINT width=0,height=0,slots=0,frame=0,epoch=0,trainingParity=0;
  bool nativeSingleRayAbi=false,sceneAndDescriptorLease=false;
  ComputeBindingSnapshot ray;
 };
 static bool Accept(ID3D12GraphicsCommandList4* list,const Inputs& p,
                    const D3D12FrozenWorkingSet& work,const D3D12GIRayConstantSnapshot& constants,
                    const D3D12GISbtSnapshot& tables,const D3D12FrozenRaygenRecords& records,
                    Trace trace)noexcept {
  if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||!trace||
     !p.nativeSingleRayAbi||!p.sceneAndDescriptorLease||!p.ray.Ready()||!p.ray.rays||
     !p.frame||!p.epoch||p.trainingParity>1||!work.Matches(p.width,p.height,p.slots)||
     !tables.MatchesExtent(p.width,p.height)||!tables.CanRecord(list)||
     !constants.CanRecord(list,p.ray)||!records.MatchesStateObject(p.ray.rays.Get()))return false;
  return records.Matches(p.width,p.height,p.frame,p.epoch,p.slots,p.trainingParity,
   work.Get(D3D12FrozenWorkingSet::Guides)->GetGPUVirtualAddress(),
   work.Get(D3D12FrozenWorkingSet::Weights)->GetGPUVirtualAddress());
 }
 static Result Record(ID3D12GraphicsCommandList4* list,const Inputs& p,
                      D3D12FrozenWorkingSet& work,D3D12GIRayConstantSnapshot& constants,
                      D3D12GISbtSnapshot& tables,const D3D12FrozenRaygenRecords& records,
                      Trace trace,ComputeBindingSnapshot& savedRay)noexcept {
  if(!Accept(list,p,work,constants,tables,records,trace))return Result::RejectedBeforeCommands;
  // No native shader-table/constant-buffer storage is borrowed for phase B.
  // A failure after the first command pins the owner; no full native retry.
  if(!tables.Record(list)||!constants.RecordAndRestore(list,p.ray))return Result::FailedAfterCommands;
  auto preserved=p.ray;
  if(!constants.ApplyTo(preserved)||!FrozenRayConstants::Matches(preserved.arguments,
      constants.Resource()->GetGPUVirtualAddress()))return Result::FailedAfterCommands;
  D3D12_RESOURCE_BARRIER barriers[2]{};
  for(auto& barrier:barriers)barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[0].Transition={work.Get(D3D12FrozenWorkingSet::Weights),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
   D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};
  barriers[1].Transition={work.Get(D3D12FrozenWorkingSet::Guides),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
   D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
  list->ResourceBarrier(2,barriers);
  if(!preserved.Restore(list))return Result::FailedAfterCommands;
  auto dispatch=*tables.Dispatch();dispatch.Width=p.width/2;dispatch.RayGenerationShaderRecord=records.At(0);
  trace(list,&dispatch);
  if(!p.ray.Restore(list))return Result::FailedAfterCommands;
  savedRay=std::move(preserved);
  // The separate Execute boundary supplies buffer decay to COMMON. The host
  // must observe that actual submission before admitting phase-B recording.
  return Result::Recorded;
 }
};
}
