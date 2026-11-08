#pragma once
#include "nrd_diffuse_pipeline.h"
#include "gi_reuse/d3d12_compute_bindings.h"

namespace control_nrd {
// Enclose RELAX in a native compute-state transaction. The caller obtains the
// snapshot from a fully covered command-list recording, and must mute its
// binding observer around this call. Never repair bindings by changing heaps:
// doing so would invalidate the game's graphics root tables as well.
struct NativeTransaction {
 struct Work {
  DiffusePipeline::Work diffuse;
  gi_reuse::ComputeBindingSnapshot native;
 };
 static bool Record(DiffusePipeline& pipeline,ID3D12GraphicsCommandList4* list,
                    const gi_reuse::ComputeBindingSnapshot& native,
                    const Frame& frame,std::uint64_t queue,
                    DiffusePipeline::Bindings& bindings,DiffusePipeline::Constants conversion,
                    D3D12Backend::Arena arena,Work& work) {
  if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||!native.Ready()||
     !arena.heap||native.heaps[0].Get()!=arena.heap||arena.count<pipeline.MaxDescriptors()||
     work.native.root||work.diffuse.heap)return false;
  work.native=native;
  struct Restore {
   ID3D12GraphicsCommandList4* list;
   const gi_reuse::ComputeBindingSnapshot& state;
   ~Restore(){state.Restore(list);}
  } restore{list,work.native};
  // Exceptions propagate after restoring state. The owner must keep Work and
  // retire/reset the failed recording; it must not release recorded resources.
  return pipeline.Record(list,frame,queue,bindings,conversion,work.diffuse,arena);
 }
};
}
