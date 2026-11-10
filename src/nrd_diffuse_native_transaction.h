#pragma once
#include "nrd_diffuse_pipeline.h"
#include "gi_reuse/d3d12_compute_bindings.h"

namespace control_nrd {
// Enclose RELAX in a native compute-state transaction. The caller obtains the
// snapshot from a fully covered command-list recording, and must mute its
// binding observer around this call. Never repair bindings by changing heaps:
// doing so would invalidate the game's graphics root tables as well.
inline void RestoreNativeHeaps(ID3D12GraphicsCommandList4* list,const gi_reuse::ComputeBindingSnapshot& native,const gi_reuse::GraphicsTableState& graphics)noexcept {
 ID3D12DescriptorHeap* heaps[2]{};UINT count=0;for(const auto& h:native.heaps)if(h)heaps[count++]=h.Get();
 list->SetDescriptorHeaps(count,heaps);
 graphics.Replay([&](unsigned index,std::uint64_t address){list->SetGraphicsRootDescriptorTable(index,{address});});
}
struct NativeTransaction {
 struct Work {
  DiffusePipeline::Work diffuse;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> privateHeap;
  gi_reuse::ComputeBindingSnapshot native;
  void RetireForReuse(){native={};diffuse.RetireForReuse();}
 };
 static bool Record(DiffusePipeline& pipeline,ID3D12GraphicsCommandList4* list,
                    const gi_reuse::ComputeBindingSnapshot& native,
                    const Frame& frame,std::uint64_t queue,
                    DiffusePipeline::Bindings& bindings,DiffusePipeline::Constants conversion,
                    D3D12Backend::Arena arena,Work& work,DiffusePipeline::ReserveArena reserve=nullptr,const void* reserveContext=nullptr,const gi_reuse::GraphicsTableState* graphics=nullptr,const Timing* timing=nullptr) {
  if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||!native.Ready()||
     !arena.heap||native.heaps[0].Get()!=arena.heap||
     work.native.root||work.diffuse.heap)return false;
  work.native=native;
  struct Restore {
   ID3D12GraphicsCommandList4* list;
   const gi_reuse::ComputeBindingSnapshot& state;
   const gi_reuse::GraphicsTableState* graphics;const DiffusePipeline::Work& work;
   ~Restore(){if(work.heapSwitched&&graphics)RestoreNativeHeaps(list,state,*graphics);state.Restore(list);}
  } restore{list,work.native,graphics,work.diffuse};
  // Exceptions propagate after restoring state. The owner must keep Work and
  // retire/reset the failed recording; it must not release recorded resources.
  return pipeline.Record(list,frame,queue,bindings,conversion,work.diffuse,arena,reserve,reserveContext,graphics&&graphics->known,timing);
 }
};
}
