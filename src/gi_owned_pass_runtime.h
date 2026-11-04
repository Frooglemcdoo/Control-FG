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
  GIOwnedPipeline::attempts.Rejected(GetTickCount64());
  GIOwnedPipeline::active=false;const auto n=++GIOwnedPipeline::rejected;
  if(n<=12||n%600==0)Log("GI_OWNED_ADMISSION rejected=%s detail=%u writer=%llu count=%llu",reason,detail,native->writer->ticket.serial,n);
  return false;
 }
 static void ReportGeometry(const gi_reuse::GISceneProducerDiagnostic& d,UINT64 writer,unsigned result) noexcept {
  Log("GI_OWNED_GEOMETRY revision=R323 writer=%llu result=%u checked=%llu valid=%llu reused=%llu persistent=%llu samples=%llu",writer,result,d.checked,d.valid,d.reused,d.persistent,UINT64(d.samples.size()));
  const char* names[]={"witness_identity","transform","witness_missing","metadata","resolve","convert","rebase","conflict"};
  for(unsigned i=0;i<unsigned(gi_reuse::GISceneGeometryStage::Count);++i)
   Log("GI_OWNED_GEOMETRY_COUNT writer=%llu stage=%s count=%llu",writer,names[i],d.counts[i]);
  for(const auto& s:d.samples){
   Log("GI_OWNED_GEOMETRY_SAMPLE writer=%llu stage=%s instance=%u geometry=%u row=%llu blas=%llx token=%llu revision=%llu status=%u candidates=%u mismatch=%u admission=%u rebase=%u rebase_admission=%u",writer,names[unsigned(s.stage)],s.instance,s.geometry,s.row,s.blas.address,s.blas.token,s.blas.revision,unsigned(s.status),s.exact.candidates,s.exact.mismatch,unsigned(s.exact.admission),unsigned(s.rebase.stage),unsigned(s.rebase.admission));
   const auto& b=s.binding;const auto& c=b.constants;const auto& w=s.witness;
   Log("GI_OWNED_GEOMETRY_BIND writer=%llu row=%llu tag=%u type=%u indices=%llx attributes=%llx constants=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",writer,s.row,b.tag,b.type,b.indices,b.attributes,c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],c[9]);
   Log("GI_OWNED_GEOMETRY_WITNESS writer=%llu row=%llu indices=%llx positions=%llx index_stride=%u position_stride=%llu index_count=%u vertex_count=%u index_token=%llu position_token=%llu found_index=%llu found_position=%llu found_attribute=%llu",writer,s.row,w.indices,w.positions,w.indexStride,w.positionStride,w.indexCount,w.vertexCount,w.indexToken,w.positionToken,s.exact.indexToken,s.exact.positionToken,s.exact.attributeToken);
   const auto& m=s.metadata;
   Log("GI_OWNED_GEOMETRY_METADATA writer=%llu row=%llu indices=%llx positions=%llx transform=%llx stride=%llu index_count=%u vertex_count=%u index_format=%u vertex_format=%u flags=%u metadata_error=%u convert_error=%u",writer,s.row,m.indices,m.vertices,m.transform,m.stride,m.indexCount,m.vertexCount,m.indexFormat,m.vertexFormat,m.flags,s.metadataError,s.convertError);
   const auto allocation=[&](const char* name,const PTOwnedScene::Allocation& a){Log("GI_OWNED_GEOMETRY_ALLOCATION writer=%llu row=%llu kind=%s token=%llu address=%llx bytes=%llu",writer,s.row,name,a.token,a.address,a.bytes);};
   allocation("indices",s.exact.indexAllocation);allocation("positions",s.exact.positionAllocation);allocation("attributes",s.exact.attributeAllocation);
   const auto range=[&](const char* name,const gi_reuse::GIFrozenRange& r){Log("GI_OWNED_GEOMETRY_FROZEN writer=%llu row=%llu kind=%s original=%llx bytes=%llu address=%llx capacity=%llu",writer,s.row,name,r.original,r.bytes,r.address,r.capacity);};
   range("indices",s.frozenIndices);range("positions",s.frozenPositions);
  }
 }
 bool Admit(const gi_reuse::GILiveSceneCatalog& catalog,UINT64 cut) noexcept override {
  GIOwnedMetrics::Scope timing(GIOwnedMetrics::Admission);
  try {
   const auto& writer=*native->writer;gi_reuse::GILiveSceneCatalog::Selection selected;gi_reuse::GILiveSceneCatalog::Diagnostic diagnostic;
   bool selectionReady=false;
   {GIOwnedMetrics::Scope detail(GIOwnedMetrics::Selection);
    selectionReady=catalog.Select(cut,native->generation,writer.scene.token,writer.scene.address,selected,&diagnostic);}
   if(!selectionReady){
    if(GIOwnedPipeline::rejected.load()<12||GIOwnedPipeline::rejected.load()%120==0)
     Log("GI_OWNED_SCENE stage=%s order=%s status=%u token=%llu address=%llx instance=%u missing_instances=%u",diagnostic.stage,
      diagnostic.orderReason,unsigned(diagnostic.status),diagnostic.token,diagnostic.address,diagnostic.instance,diagnostic.missingBottoms);
    GINativeSceneQueues::ExplainWait(cut,native->writer->ticket.serial);
    return Reject("ordered_scene");
   }
   // Every triangle must be tied to its BLAS-build-time copies, including hair.
   if(selected.geometry.empty())return Reject("empty_scene");
   for(const auto& geometry:selected.geometry)if(!geometry.frozen||!geometry.frozen->owner)return Reject("unpreserved_geometry");
   PTShaderTableAdapter::Exports exports{writer.opaque,writer.alpha,pipeline->pipeline.Producer().Opaque(),pipeline->pipeline.Producer().Alpha()};
   // Sample counters periodically so persistent reuse is visible after warmup.
   static std::atomic<unsigned> censusAttempts{0};
   gi_reuse::GISceneProducerDiagnostic census;
   const auto censusAttempt=censusAttempts.fetch_add(1);
   auto* censusOutput=(censusAttempt<3||censusAttempt%120==0)?&census:nullptr;
   gi_reuse::GISceneProducerError prepared;gi_reuse::D3D12LiveSceneOwners::PrepareTimings timings;
   {GIOwnedMetrics::Scope detail(GIOwnedMetrics::Mapping);
    prepared=GINativeSceneOwners::owners.Prepare(catalog.History(),cut,native->generation,selected.top,selected.instances,selected.perInstance,
      selected.geometry,writer.inputs.table,writer.inputs.bindings,reinterpret_cast<UINT_PTR>(pipeline->pipeline.Producer().State()),exports,scene,censusOutput,&timings);}
   GIOwnedMetrics::RecordMS(GIOwnedMetrics::OwnerLock,timings.lockMS);
   GIOwnedMetrics::RecordMS(GIOwnedMetrics::MappingWork,timings.mappingMS);
   GIOwnedMetrics::RecordMS(GIOwnedMetrics::OwnerLeases,timings.leasesMS);
   if(censusOutput)ReportGeometry(census,writer.ticket.serial,unsigned(prepared));
   if(prepared!=gi_reuse::GISceneProducerError::None)return Reject("private_scene_table",unsigned(prepared));
   std::vector<ID3D12Resource*> resources;resources.reserve(scene.buffers.size()+scene.frozen.size()*2);
   for(const auto& lease:scene.buffers)resources.push_back(lease.resource.Get());
   std::set<const void*> storages;
   for(const auto& geometry:scene.frozen)if(storages.insert(geometry->owner.get()).second){
    const auto storage=std::static_pointer_cast<GIGeometrySnapshot::Storage>(geometry->owner);
    for(const auto& copy:storage->copies)resources.push_back(copy.second.Get());
   }
   HRESULT committed;{GIOwnedMetrics::Scope detail(GIOwnedMetrics::TableCommit);committed=table.Commit(device.Get(),scene.plan.table,resources);}if(FAILED(committed))return Reject("table_commit",unsigned(committed));
   HRESULT armed;{GIOwnedMetrics::Scope detail(GIOwnedMetrics::Controls);armed=tile.CommitAdmission(table);}if(FAILED(armed))return Reject("controls",unsigned(armed));
   GIOwnedPipeline::attempts.Succeeded();const auto n=++GIOwnedPipeline::admitted;GIOwnedPipeline::active=true;
   if(n<=4||n%600==0)Log("GI_OWNED_ADMITTED writer=%llu triangles=%llu backing_buffers=%llu count=%llu gpu_completion=0",writer.ticket.serial,
    static_cast<UINT64>(selected.geometry.size()),static_cast<UINT64>(resources.size()),n);
   return true;
  }catch(...){Cancel();return Reject("exception");}
 }
 bool ProducerOrdered(UINT64 queue) noexcept override {
  return native->writer->submission.Precedes(queue)||Reject("producer_not_ordered");
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
 if(!GIOwnedPipeline::attempts.Allow(GetTickCount64())){GIOwnedPipeline::active=false;return;}
 GIOwnedMetrics::Scope timing(GIOwnedMetrics::Prepare);
 const auto& writer=*native->writer;
 if(!writer.nativeInputs||writer.pass||writer.passes!=1||!writer.ticket.width||!writer.ticket.height||z!=1||x!=(writer.ticket.width+7)/8||y!=(writer.ticket.height+15)/16||
    !native->shadow.Has(native->generation,gi_reuse::D3D12InitialGITile::NativeRequired)){Skip("native_pass_contract");return;}
 const auto count=UINT64(writer.ticket.width)*writer.ticket.height;
 if(count>UINT32_MAX||!UINT(writer.ticket.serial)){Skip("image_or_revision_range");return;}
 auto recording=GINativeSceneOwners::ReadRecording(list,false);
 if(!recording||!recording->complete||recording->submitted||recording->generation!=native->generation){Skip("recording_generation");return;}
 if(!GINativeSceneQueues::Preflight(native->context.queue,*recording.Get(),writer)){Skip("scene_preflight");return;}
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
  if(n<=4||n%600==0){const auto stats=gi_reuse::SharedGIScratchPool().Statistics();Log("GI_OWNED_SCRATCH revision=R325 idle_bytes=%llu reuses=%llu",stats.bytes,stats.reuses);}
  if(n<=4||n%600==0)Log("GI_OWNED_RECORDED frame=%llu writer=%llu pixels=%llu rays=%u scratch=%llu deferred_admission=1 count=%llu",
   writer.ticket.engineFrame,writer.ticket.serial,count,writer.rays,bytes,n);
 }catch(...){Skip("recording_exception");}
}
[[maybe_unused]] static const bool registered=[] {GIOwnedPipeline::dispatchObserver=&AfterCompute;return true;}();
}
