#pragma once
#include "gi_native_gi_bindings.h"
#include "gi_owned_pipeline_runtime.h"
namespace GIOwnedPass {
using Microsoft::WRL::ComPtr;
static const auto scratchBudget=std::make_shared<PTOwnedScene::LeaseBudget>(512ull*1024*1024);
class Pass final:public GINativeSceneOwners::Use {
public:
 ComPtr<GIOwnedPipeline::Owner> pipeline;
 ComPtr<ID3D12Device> device;
 ComPtr<GINativeGIBindings::Snapshot> native;
 gi_reuse::ComputeBindingSnapshot bindings;
 gi_reuse::D3D12InitialGITile tile;
 gi_reuse::D3D12ProducerTable table;
 gi_reuse::D3D12LiveSceneOwners::Lease scene;
 UINT64 bytes=0;
 ~Pass(){if(bytes)scratchBudget->Release(bytes);}
 bool Reject(const char* reason,unsigned detail=0) noexcept {
  GIOwnedPipeline::active=false;const auto n=++GIOwnedPipeline::rejected;
  if(n<=12||n%600==0)Log("GI_OWNED_ADMISSION rejected=%s detail=%u writer=%llu count=%llu",reason,detail,native->writer->ticket.serial,n);
  return false;
 }
 bool Admit(const gi_reuse::GILiveSceneCatalog& catalog,UINT64 cut) noexcept override {
  try {
   const auto& writer=*native->writer;gi_reuse::GILiveSceneCatalog::Selection selected;
   if(!catalog.Select(cut,native->generation,writer.scene.token,writer.scene.address,selected))return Reject("ordered_scene");
   // Every triangle must be tied to its BLAS-build-time copies, including hair.
   if(selected.geometry.empty())return Reject("empty_scene");
   for(const auto& geometry:selected.geometry)if(!geometry.frozen||!geometry.frozen->owner)return Reject("unpreserved_geometry");
   PTShaderTableAdapter::Exports exports{writer.opaque,writer.alpha,pipeline->pipeline.Producer().Opaque(),pipeline->pipeline.Producer().Alpha()};
   const auto prepared=GINativeSceneOwners::owners.Prepare(catalog.History(),cut,native->generation,selected.top,selected.instances,selected.perInstance,
      selected.geometry,writer.inputs.table,writer.inputs.bindings,reinterpret_cast<UINT_PTR>(pipeline->pipeline.Producer().State()),exports,scene);
   if(prepared!=gi_reuse::GISceneProducerError::None)return Reject("private_scene_table",unsigned(prepared));
   std::vector<ID3D12Resource*> resources;resources.reserve(scene.buffers.size()+scene.frozen.size()*2);
   for(const auto& lease:scene.buffers)resources.push_back(lease.resource.Get());
   std::set<const void*> storages;
   for(const auto& geometry:scene.frozen)if(storages.insert(geometry->owner.get()).second){
    const auto storage=std::static_pointer_cast<GIGeometrySnapshot::Storage>(geometry->owner);
    for(const auto& copy:storage->copies)resources.push_back(copy.second.Get());
   }
   const auto committed=table.Commit(device.Get(),scene.plan.table,resources);if(FAILED(committed))return Reject("table_commit",unsigned(committed));
   const auto armed=tile.CommitAdmission(table);if(FAILED(armed))return Reject("controls",unsigned(armed));
   const auto n=++GIOwnedPipeline::admitted;GIOwnedPipeline::active=true;
   if(n<=4||n%600==0)Log("GI_OWNED_ADMITTED writer=%llu triangles=%llu backing_buffers=%llu count=%llu gpu_completion=0",writer.ticket.serial,
    static_cast<UINT64>(selected.geometry.size()),static_cast<UINT64>(resources.size()),n);
   return true;
  }catch(...){Cancel();return Reject("exception");}
 }
 bool RecordGate(ID3D12GraphicsCommandList* prefix) noexcept override{return tile.RecordGate(prefix);}
 void Cancel() noexcept override{tile.CancelAdmission();GIOwnedPipeline::active=false;}
};
static void Skip(const char* reason) noexcept {
 GIOwnedPipeline::active=false;
 const auto count=++GIOwnedPipeline::rejected;if(count<=8||count%600==0)Log("GI_OWNED_SKIP reason=%s count=%llu native_output_retained=1",reason,count);
}
static void AfterCompute(ID3D12GraphicsCommandList4* list,const gi_reuse::ComputeBindingSnapshot& bindings,UINT x,UINT y,UINT z) noexcept {
 if(GIOwnedPipeline::recording||!GIOwnedPipeline::Enabled()||!GIOwnedPipeline::requested.load())return;
 auto pipeline=GIOwnedPipeline::Read(bindings.pipeline.Get());if(!pipeline)return;
 auto native=GINativeGIBindings::Take(list);if(!native){Skip("missing_native_bind");return;}
 const auto& writer=*native->writer;
 if(!writer.nativeInputs||writer.pass||writer.passes!=1||!writer.ticket.width||!writer.ticket.height||z!=1||x!=(writer.ticket.width+7)/8||y!=(writer.ticket.height+15)/16||
    !native->shadow.Has(native->generation,gi_reuse::D3D12InitialGITile::NativeRequired)){Skip("native_pass_contract");return;}
 const auto count=UINT64(writer.ticket.width)*writer.ticket.height;
 if(count>UINT32_MAX||!UINT(writer.ticket.serial)){Skip("image_or_revision_range");return;}
 auto recording=GINativeSceneOwners::ReadRecording(list,false);
 if(!recording||!recording->complete||recording->submitted||recording->generation!=native->generation){Skip("recording_generation");return;}
 struct Mute {Mute(){GIOwnedPipeline::recording=true;}~Mute(){GIOwnedPipeline::recording=false;}} mute;
 try {
  auto pass=std::make_shared<Pass>();pass->pipeline=pipeline;pass->native=native;pass->bindings=bindings;
  if(FAILED(list->GetDevice(IID_PPV_ARGS(&pass->device)))){Skip("device");return;}
  GINativeSceneDescriptors::Mute descriptors;
  if(FAILED(pass->table.Reserve(pass->device.Get(),pipeline->pipeline.Producer().State(),static_cast<UINT>(writer.inputs.bindings.size()),native->generation,writer.ticket.serial))||
     FAILED(pass->tile.Initialize(pass->device.Get(),native->shadow,native->generation,native->generation,writer.ticket.serial,writer.scene.address,
      UINT(writer.ticket.engineFrame),UINT(writer.ticket.serial),writer.ticket.width,writer.ticket.height,0,UINT(count),writer.rays,writer.baseRange,true,true))){Skip("allocation");return;}
  const auto bytes=pass->tile.RetainedBytes()+pass->table.RetainedBytes();
  if(!scratchBudget->Acquire(bytes)){Skip("scratch_budget");return;}pass->bytes=bytes;
  std::array<PTGlass::DescriptorRange,5> fresh{};
  for(auto& range:fresh)if(!PTGlassRuntime::Fresh(native->context,&range)){Skip("native_descriptor_capacity");return;}
  if(!pass->tile.Publish(native->context.table,fresh)){Skip("descriptor_publication");return;}
  // Install lifetime ownership before the first injected GPU command. A false
  // Record result guarantees no command referenced this pass and may remove it.
  recording->events.reserve(recording->events.size()+1);GINativeSceneOwners::Event use;use.use=pass;
  recording->events.push_back(std::move(use));
  if(!pass->tile.Record(list,pipeline->pipeline,pass->table,bindings)){recording->events.pop_back();Skip("native_root_state");return;}
  const auto n=++GIOwnedPipeline::recorded;
  if(n<=4||n%600==0)Log("GI_OWNED_RECORDED frame=%llu writer=%llu pixels=%llu rays=%u scratch=%llu deferred_admission=1 count=%llu",
   writer.ticket.engineFrame,writer.ticket.serial,count,writer.rays,bytes,n);
 }catch(...){Skip("recording_exception");}
}
[[maybe_unused]] static const bool registered=[] {GIOwnedPipeline::dispatchObserver=&AfterCompute;return true;}();
}
