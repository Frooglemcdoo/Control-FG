#pragma once
#include "d3d12_compute_bindings.h"
namespace gi_reuse {
// Temporary switch between two independently retained recording heap pairs.
// No ownership of descriptors' referenced resources is inferred here. Capture
// graphics tables at the consumer site; mute injected hook observers throughout.
struct D3D12SavedHeapSwitch {
 static bool Same(const ComputeBindingSnapshot& a,const ComputeBindingSnapshot& b)noexcept {
  return a.heaps[0].Get()==b.heaps[0].Get()&&a.heaps[1].Get()==b.heaps[1].Get();
 }
 static bool Valid(const ComputeBindingSnapshot& bindings)noexcept {
  if(!bindings.Ready()||!bindings.heaps[0])return false;
  for(unsigned i=0;i<2;++i)if(bindings.heaps[i]){
   const auto d=bindings.heaps[i]->GetDesc();
   if(!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
      d.Type!=(i?D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))return false;
  }
  return true;
 }
 static bool Accept(const ComputeBindingSnapshot& ray,const ComputeBindingSnapshot& shade,
                    const GraphicsTableState& graphics)noexcept {
  return Valid(ray)&&Valid(shade)&&(Same(ray,shade)||graphics.known);
 }
 static void Bind(ID3D12GraphicsCommandList* list,const ComputeBindingSnapshot& bindings)noexcept {
  ID3D12DescriptorHeap* heaps[2]{};UINT count=0;
  for(const auto& heap:bindings.heaps)if(heap)heaps[count++]=heap.Get();
  list->SetDescriptorHeaps(count,heaps);
 }
 // Admission for BOTH directions must succeed before the first switch.
 static bool ToRay(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& ray,
                   const ComputeBindingSnapshot& shade,const GraphicsTableState& graphics)noexcept {
  if(!list||!Accept(ray,shade,graphics))return false;
  if(!Same(ray,shade))Bind(list,ray);
  return ray.Restore(list);
 }
 static bool ToShade(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& ray,
                     const ComputeBindingSnapshot& shade,const GraphicsTableState& graphics)noexcept {
  if(!list||!Accept(ray,shade,graphics))return false;
  if(!Same(ray,shade)){
   Bind(list,shade);
   // Switching back does not resurrect graphics descriptor tables. Graphics
   // root/constants/PSO/raster state were not changed by these compute passes.
   graphics.Replay([&](unsigned index,std::uint64_t address)noexcept{
    list->SetGraphicsRootDescriptorTable(index,{address});
   });
  }
  return shade.Restore(list);
 }
};
}
