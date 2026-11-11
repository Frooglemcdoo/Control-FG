#pragma once
#include "d3d12_compute_bindings.h"
namespace gi_reuse {
// Immutable per-invocation binding VALUES. This retains root/PSO/heap objects,
// NOT descriptor contents, CBV bytes, referenced resources or native SBT writers.
// The runtime must hold those separate leases through GPU completion.
class D3D12PhaseBindingPair {
 ComputeBindingSnapshot phase_[2];
 std::uint64_t invocation_=0,frame_=0;
 unsigned mask_=0;
 bool sealed_=false;
public:
 bool Remember(unsigned phase,std::uint64_t invocation,std::uint64_t frame,
               const ComputeBindingSnapshot& bindings) noexcept {
  if(sealed_||phase>1||!invocation||!frame||!bindings.Ready()||(mask_&(1u<<phase)))return false;
  if(mask_&&(invocation!=invocation_||frame!=frame_))return false;
  phase_[phase]=bindings;invocation_=invocation;frame_=frame;mask_|=1u<<phase;return true;
 }
 bool Seal() noexcept {
  if(sealed_||mask_!=3||phase_[0].root.Get()!=phase_[1].root.Get())return false;
  for(unsigned i=0;i<2;++i)if(phase_[0].heaps[i].Get()!=phase_[1].heaps[i].Get())return false;
  sealed_=true;return true;
 }
 bool Restore(unsigned phase,std::uint64_t invocation,std::uint64_t frame,
              ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& current)const noexcept {
  if(!sealed_||phase>1||!list||invocation!=invocation_||frame!=frame_||!current.Ready())return false;
  for(unsigned i=0;i<2;++i)if(current.heaps[i].Get()!=phase_[phase].heaps[i].Get())return false;
  return phase_[phase].Restore(list);
 }
 bool BindingValuesReady()const noexcept{return sealed_;}
};
}
