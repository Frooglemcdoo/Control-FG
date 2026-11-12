#pragma once
#include "gi_reuse/d3d12_live_scene_owners.h"
#include "gi_reuse/live_scene_catalog.h"
#include "gi_reuse/native_gi_identity.h"
#include "gi_reuse/d3d12_geometry_snapshot.h"
#include "gi_native_lifetime_runtime.h"
#include "gi_reuse/writer_handoff.h"
#include "gi_owned_metrics.h"
#include "gi_reuse/owned_request.h"
// Recovered R103 creation boundaries. These observe native ownership only;
// installing them does not enable the replacement lighting dispatch.
namespace GINativeSceneOwners {
static gi_reuse::D3D12LiveSceneOwners owners;
static std::atomic<bool> ready{false};
// Stop new source inventory only; retirement remains live for retained owners.
static std::atomic<bool> frozenSourceTrackingStopped{false};
static std::atomic<UINT64> created{0},rejected{0};
static bool Enabled() noexcept {if(gi_reuse::SingleProducerEnabled())return false;static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_OWNED",b,4)==1&&b[0]=='1';}();return value;}
struct Wrapper {IUnknown* resource=nullptr;UINT64 bytes=0,address=0;UINT state=0;};
static bool Read(void* pointer,Wrapper& output) noexcept {
 output={};__try{if(!pointer)return false;auto* p=static_cast<unsigned char*>(pointer);
  output.resource=*reinterpret_cast<IUnknown**>(p);output.state=*reinterpret_cast<UINT*>(p+0x20);output.bytes=*reinterpret_cast<UINT64*>(p+0x30);output.address=*reinterpret_cast<UINT64*>(p+0x48);
  return output.resource&&output.bytes&&output.address;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void Created(void* wrapper) noexcept {
 if(!ready.load()||frozenSourceTrackingStopped.load())return;const DWORD error=GetLastError();Wrapper input;
 if(Read(wrapper,input)&&owners.Created(reinterpret_cast<UINT_PTR>(wrapper),input.resource,input.address,input.bytes))++created;
 else {owners.Retire(reinterpret_cast<UINT_PTR>(wrapper));++rejected;}
 SetLastError(error);
}
static void Retire(void* wrapper) noexcept {if(ready.load())owners.Retire(reinterpret_cast<UINT_PTR>(wrapper));}
using General=void(*)(UINT64,UINT64,unsigned,unsigned,UINT64,UINT64,void*);
using Ray=void(*)(int,int,void*,int);
static General general=nullptr;static Ray ray=nullptr;
static std::atomic<void(*)(void*) noexcept> liveASDescriptor{nullptr};
static RRAlbedoCallPatch creates[11]{};
static void GeneralHook(UINT64 bytes,UINT64 stride,unsigned flags,unsigned format,UINT64 a,UINT64 b,void* wrapper){general(bytes,stride,flags,format,a,b,wrapper);Created(wrapper);}
static void RayHook(int bytes,int type,void* wrapper,int flags){ray(bytes,type,wrapper,flags);Created(wrapper);if(type==1)if(const auto observe=liveASDescriptor.load())observe(wrapper);}
// CPU source bytes are copied before the native memcpy, never read back from
// upload heaps. Build observers must retain the returned immutable snapshot.
struct InstanceUpload {UINT64 address=0,token=0;std::vector<unsigned char> bytes;};
static std::mutex uploadsLock;
static std::map<UINT64,std::shared_ptr<const InstanceUpload>> uploads;
using LockInstances=void(*)(void*,UINT64,UINT64,unsigned,void**);
using CopyInstances=void*(*)(void*,const void*,std::size_t);
static LockInstances lockInstances=nullptr;static CopyInstances copyInstances=nullptr;
static RRIndirectCallPatch lockPatch;static RRAlbedoCallPatch copyPatch{};
struct Locked {void* pointer=nullptr;UINT64 address=0,bytes=0;};static thread_local Locked locked;
static bool ReadPointer(void** source,void*& value) noexcept {__try{if(!source)return false;value=*source;return value!=nullptr;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
static bool CopyBytes(void* to,const void* from,std::size_t bytes) noexcept {__try{if(!from)return false;std::memcpy(to,from,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
static void LockHook(void* wrapper,UINT64 offset,UINT64 bytes,unsigned mode,void** pointer){
 lockInstances(wrapper,offset,bytes,mode,pointer);const DWORD error=GetLastError();locked={};Wrapper input;void* mapped=nullptr;
 if(ready.load()&&!offset&&bytes&&bytes<=2ull*1024*1024&&bytes%64==0&&Read(wrapper,input)&&bytes<=input.bytes&&ReadPointer(pointer,mapped))locked={mapped,input.address,bytes};
 SetLastError(error);
}
static void* CopyHook(void* to,const void* from,std::size_t bytes){
 const DWORD error=GetLastError();const auto current=locked;locked={};
 if(ready.load()&&to==current.pointer&&bytes==current.bytes&&current.address){
  try {std::lock_guard<std::mutex> guard(uploadsLock);uploads.erase(current.address);
   gi_reuse::D3D12LiveSceneOwners::Identity allocation;
   if(uploads.size()<64&&owners.Identify(current.address,bytes,allocation)){
    auto snapshot=std::make_shared<InstanceUpload>();snapshot->address=current.address;snapshot->token=allocation.token;snapshot->bytes.resize(bytes);
    if(CopyBytes(snapshot->bytes.data(),from,bytes))uploads.emplace(current.address,std::move(snapshot));
   }
  }catch(...){ready=false;}
 }
 SetLastError(error);return copyInstances(to,from,bytes);
}
[[maybe_unused]] static std::shared_ptr<const InstanceUpload> Instances(UINT64 address,UINT count) noexcept {
 try {std::lock_guard<std::mutex> guard(uploadsLock);gi_reuse::D3D12LiveSceneOwners::Identity allocation;
  const auto found=uploads.find(address);if(!ready.load()||!count||found==uploads.end()||found->second->bytes.size()!=UINT64(count)*64||
      !owners.Identify(address,UINT64(count)*64,allocation)||allocation.token!=found->second->token)return {};
  return found->second;
 }catch(...){return {};}
}
struct Writer {
 explicit Writer(UINT64 producerGeneration):submission(producerGeneration),generation(producerGeneration){}
 mutable gi_reuse::GIWriterSubmission submission;
 const UINT64 generation;
 GINativeJobRoute::Ticket ticket;
 gi_reuse::NativeGIWriterInputs inputs;
 Microsoft::WRL::ComPtr<ID3D12StateObject> pipeline;
 PTShaderTableAdapter::Identifier opaque{},alpha{};
 gi_reuse::D3D12LiveSceneOwners::Identity scene;
 float baseRange=0;UINT rays=0,pass=0,passes=0;bool nativeInputs=false;
 const char* inputReason="not_observed";
};
static std::atomic<bool(*)(Writer&) noexcept> writerInputs{nullptr};
static thread_local gi_reuse::GIWriterHandoff<Writer> writerHandoff;
static std::atomic<UINT64> writerPublications{0};
struct Use {
 virtual ~Use()=default;
 virtual bool ProducerOrdered(UINT64 queue) noexcept=0;
 virtual bool Admit(const gi_reuse::GILiveSceneCatalog&,UINT64 cut) noexcept=0;
 virtual bool RecordGate(ID3D12GraphicsCommandList*) noexcept=0;
 virtual void Cancel() noexcept=0;
};
struct Event {
 gi_reuse::OrderedASEvent build;
 std::shared_ptr<const InstanceUpload> instances;
 std::vector<gi_reuse::D3D12LiveSceneOwners::Identity> bottoms;
 std::vector<PTOwnedScene::Triangle> geometry;
 std::vector<std::shared_ptr<const gi_reuse::GIFrozenGeometry>> frozen;
 std::shared_ptr<GIGeometrySnapshot::Transfer> transfer;
 std::shared_ptr<const Writer> writer;
 std::shared_ptr<Use> use;
};
static const GUID recordingKey={0x310e0001,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
static std::atomic<UINT64> generations{1};
class Recording final:public IUnknown {
 std::atomic<ULONG> refs{1};
public:
 const UINT64 generation=generations.fetch_add(1);
 bool complete=true,submitted=false;
 const char* incompleteReason="none";
 void Incomplete(const char* reason) noexcept {if(complete)incompleteReason=reason;complete=false;}
 std::vector<Event> events;
 gi_reuse::LocalBufferStates<> states;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override {if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
};
static Microsoft::WRL::ComPtr<Recording> ReadRecording(ID3D12Object* object,bool create=true) noexcept {
 Microsoft::WRL::ComPtr<Recording> result;if(!object||!ready.load())return result;
 UINT bytes=sizeof(IUnknown*);IUnknown* value=nullptr;
 if(SUCCEEDED(object->GetPrivateData(recordingKey,&bytes,&value))&&value){if(bytes==sizeof(value))result.Attach(static_cast<Recording*>(value));else value->Release();return result;}
 if(!create)return result;result.Attach(new(std::nothrow) Recording);
 if(result&&FAILED(object->SetPrivateDataInterface(recordingKey,result.Get())))result.Reset();return result;
}
static void Initialize(ID3D12GraphicsCommandList* list) noexcept {ReadRecording(list);}
static void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!ready.load()||!list)return;
 // A successful native Reset cancels unsubmitted metadata, while submitted
 // owners must be retained separately by the queue retirement path.
 list->SetPrivateDataInterface(recordingKey,nullptr);
 ReadRecording(list);
}
static thread_local bool copyingGeometry=false;
static const auto geometryBudget=std::make_shared<PTOwnedScene::LeaseBudget>(512ull*1024*1024);
static const auto geometryPool=std::make_shared<GIGeometrySnapshot::Pool>();
static void Barriers(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RESOURCE_BARRIER* barriers) noexcept {
 if(copyingGeometry||!ready.load())return;auto recording=ReadRecording(list);if(!recording||!recording->complete)return;
 if(count&&!barriers){recording->Incomplete("null_barriers");return;}
 for(UINT i=0;i<count;++i){const auto& b=barriers[i];
  if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){recording->Incomplete("aliasing_barrier");return;}
  if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION||!b.Transition.pResource)continue;
  const auto desc=b.Transition.pResource->GetDesc();if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)continue;
  gi_reuse::D3D12LiveSceneOwners::Identity id;
  if(!owners.Identify(b.Transition.pResource->GetGPUVirtualAddress(),desc.Width,id))continue;
  if(b.Transition.Subresource!=0&&b.Transition.Subresource!=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES){recording->Incomplete("buffer_subresource");return;}
  recording->states.Transition(id.token,unsigned(b.Transition.StateBefore),unsigned(b.Transition.StateAfter),unsigned(b.Flags));
 }
 if(!recording->states.Complete())recording->Incomplete("buffer_state_tracking");
}
static bool FreezeGeometry(ID3D12GraphicsCommandList4* list,Recording& recording,Event& event,const char*& reason){
 reason="geometry_empty";
 if(event.geometry.empty())return false;
 std::map<UINT64,GIGeometrySnapshot::Source> sources;
 for(const auto& g:event.geometry)for(const auto token:{g.indexToken,g.positionToken}){
  if(sources.count(token))continue;
  reason="geometry_storage_owner";
  gi_reuse::BufferStorageLease lease;UINT64 wrapper=0;Wrapper native;
  if(!owners.Inspect(token,lease,wrapper)||!Read(reinterpret_cast<void*>(wrapper),native))return false;
  // Never call through a raw wrapper pointer after releasing the owner lock.
  // The retained resource is callable; a changed wrapper is simply ineligible.
  reason="geometry_wrapper_identity";
  if(native.resource!=static_cast<IUnknown*>(lease.resource.Get()))return false;
  const auto address=lease.address,bytes=lease.bytes;
  if(address!=native.address||native.bytes>bytes)return false;
  reason="geometry_resource_state";
  const auto local=recording.states.Find(token);D3D12_RESOURCE_STATES state{};
  if(local.kind==gi_reuse::LocalStateKind::Pending||local.kind==gi_reuse::LocalStateKind::Invalid)return false;
  if(local.kind==gi_reuse::LocalStateKind::Explicit)state=static_cast<D3D12_RESOURCE_STATES>(local.after);
  else {
   // Recovered R103 policy: static native wrappers retain creation read flags
   // across buffer decay. Only these exact read defaults normalize to COMMON;
   // never infer completion of UAV/COPY_DEST or a pending local transition.
   constexpr unsigned reads=D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
   state=native.state&&!(native.state&~reads)?D3D12_RESOURCE_STATE_COMMON:static_cast<D3D12_RESOURCE_STATES>(native.state);
  }
  sources.emplace(token,GIGeometrySnapshot::Source{{token,address,bytes},lease.resource,state,0,0,lease});
 }
 reason="geometry_copy_span";
 std::map<UINT64,std::pair<UINT64,UINT64>> spans;
 const auto include=[&](UINT64 token,UINT64 address,UINT64 bytes){
  const auto& allocation=sources.at(token).allocation;
  if(!bytes||address<allocation.address||address-allocation.address>allocation.bytes||bytes>allocation.bytes-(address-allocation.address))return false;
  const auto start=(address-allocation.address)&~UINT64(3),end=address-allocation.address+bytes;
  if(end>UINT64_MAX-3)return false;const auto last=(std::min)((end+3)&~UINT64(3),allocation.bytes);
  const auto inserted=spans.emplace(token,std::make_pair(start,last));
  if(!inserted.second){auto& span=inserted.first->second;span.first=(std::min)(span.first,start);span.second=(std::max)(span.second,last);}return true;
 };
 for(const auto& g:event.geometry){
  if(!g.vertexCount||g.positionStride<12||UINT64(g.vertexCount-1)>(UINT64_MAX-12)/g.positionStride||
    !include(g.indexToken,g.indices,UINT64(g.indexCount)*g.indexStride)||
    !include(g.positionToken,g.positions,UINT64(g.vertexCount-1)*g.positionStride+12))return false;
 }
 for(auto& item:sources){const auto& span=spans.at(item.first);item.second.offset=span.first;item.second.bytes=span.second-span.first;}
 std::vector<std::shared_ptr<gi_reuse::GIFrozenGeometry>> frozen;frozen.reserve(event.geometry.size());event.frozen.reserve(event.geometry.size());
 for(const auto& g:event.geometry){
  auto f=std::make_shared<gi_reuse::GIFrozenGeometry>();const auto& i=sources.at(g.indexToken);const auto& p=sources.at(g.positionToken);
  f->indices.original=i.allocation.address+i.offset;f->indices.bytes=i.bytes;
  f->positions.original=p.allocation.address+p.offset;f->positions.bytes=p.bytes;
  event.frozen.push_back(f);frozen.push_back(std::move(f));
 }
 std::vector<GIGeometrySnapshot::Source> ordered;ordered.reserve(sources.size());for(const auto& item:sources)ordered.push_back(item.second);
 struct Mute {Mute(){copyingGeometry=true;}~Mute(){copyingGeometry=false;}} mute;
 reason="geometry_copy_record";
 if(!GIGeometrySnapshot::Record(list,ordered,geometryBudget,event.transfer,geometryPool,&reason)){event.frozen.clear();return false;}
 // No allocations beyond this point: the command list now refers to the copies.
 for(std::size_t n=0;n<frozen.size();++n){auto& f=*frozen[n];const auto& g=event.geometry[n];
  const auto& index=event.transfer->storage->copies.at(g.indexToken);const auto& position=event.transfer->storage->copies.at(g.positionToken);
  f.owner=event.transfer->storage;f.indices.address=index->GetGPUVirtualAddress();f.indices.capacity=index->GetDesc().Width;
  f.positions.address=position->GetGPUVirtualAddress();f.positions.capacity=position->GetDesc().Width;
 }
 return true;
}
static void Build(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
 GIOwnedMetrics::Scope timing(GIOwnedMetrics::Geometry);
 auto recording=ReadRecording(list);if(!recording||!recording->complete)return;
 try {
  if(!desc||recording->events.size()>=65536){recording->Incomplete("build_descriptor_or_capacity");return;}
  gi_reuse::D3D12LiveSceneOwners::Identity destination,source;
  gi_reuse::D3D12LiveSceneOwners::LookupDiagnostic lookup;
  if(!owners.Identify(desc->DestAccelerationStructureData,256,destination,&lookup)){
   static std::atomic<UINT64> lookupFailures{0};const auto failures=++lookupFailures;
   if(failures<=8||failures%600==0)Log("GI_OWNED_OWNER_FAILURE revision=R325 reason=%s address=%llx owners=%llu retained=%llu budget=%llu covering=%llu top=%u descriptors=%u",lookup.reason,desc->DestAccelerationStructureData,lookup.owners,lookup.bytes,lookup.budget,lookup.covering,unsigned(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL),desc->Inputs.NumDescs);
   recording->Incomplete("build_destination_owner");return;}

  // Reserve before recording copy commands, while growing geometrically across BLAS builds.
  if(recording->events.size()==recording->events.capacity()){
   const auto capacity=recording->events.capacity();
   recording->events.reserve((std::min)(std::size_t(65536),(std::max)(std::size_t(8),capacity*2)));
  }
  Event e;auto& event=e.build.event;auto& r=event.record;
  r.address=desc->DestAccelerationStructureData;r.count=desc->Inputs.NumDescs;
  r.allowUpdate=(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)!=0;
  e.build.destinationToken=destination.token;event.operation=gi_reuse::ASOperation::Build;
  if(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE){
   event.source=desc->SourceAccelerationStructureData;event.operation=gi_reuse::ASOperation::Update;
   if(!owners.Identify(event.source,256,source)){recording->Incomplete("update_source_owner");return;}e.build.sourceToken=source.token;
  }
  bool valid=r.count!=0;const char* reason="empty_build";unsigned failureClass=0;
  gi_reuse::GeometryDiagnostic diagnostic;
  if(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
   r.topLevel=true;r.instances=desc->Inputs.InstanceDescs;e.instances=Instances(r.instances,r.count);
   reason=desc->Inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY?"tlas_instance_snapshot":"tlas_layout";failureClass=1;
   valid=valid&&desc->Inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY&&bool(e.instances);
   if(valid){e.bottoms.resize(r.count);
    for(UINT i=0;i<r.count;++i){gi_reuse::InstanceRecord row;
     if(!gi_reuse::DecodeInstance(e.instances->bytes.data()+SIZE_T(i)*64,row)){valid=false;reason="instance_decode";failureClass=2;break;}
     if(row.blas&&!owners.Identify(row.blas,256,e.bottoms[i])){valid=false;reason="instance_blas_owner";failureClass=3;break;}
    }
   }
  }else{
   r.geometry=gi_reuse::CopyBLASGeometry(desc->Inputs,&diagnostic);valid=valid&&bool(r.geometry);
   reason=gi_reuse::GeometryRejectName(diagnostic.reason);failureClass=4;
   if(valid){reason="geometry_owner_or_transform";failureClass=5;e.geometry.resize(r.count);for(UINT i=0;i<r.count;++i)if(!owners.Geometry(*r.geometry->At(i),e.geometry[i])){valid=false;diagnostic.index=i;diagnostic.triangle=*r.geometry->At(i);break;}}
   if(valid){failureClass=6;valid=FreezeGeometry(list,*recording.Get(),e,reason);}
  }
  // Unsupported writes hide the prior version; they never preserve an old
  // valid geometry mapping at the same address.
  if(!valid){
   static std::array<std::atomic<UINT64>,7> failures{};const auto n=++failures[failureClass];
   if(n<=8||n%256==0)Log("GI_OWNED_BUILD_REJECT reason=%s class=%u count=%llu top=%u address=%llx token=%llu source=%llx descriptors=%u flags=%x instance_address=%llx geometry=%u vertices=%u indices=%u stride=%llu transform=%llx frozen_bytes=%llu",
    reason,failureClass,n,unsigned(r.topLevel),r.address,destination.token,event.source,r.count,unsigned(desc->Inputs.Flags),r.instances,
    diagnostic.index,diagnostic.triangle.vertexCount,diagnostic.triangle.indexCount,diagnostic.triangle.stride,diagnostic.triangle.transform,geometryBudget->Held());
   event.operation=gi_reuse::ASOperation::Invalidate;e.geometry.clear();e.bottoms.clear();e.instances.reset();}
  recording->events.push_back(std::move(e));
 }catch(...){recording->Incomplete("observer_exception");}
}
static void Copy(ID3D12GraphicsCommandList4* list,UINT64 destination,UINT64 source,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode) noexcept {
 auto recording=ReadRecording(list);if(!recording||!recording->complete)return;
 try {gi_reuse::D3D12LiveSceneOwners::Identity dst,src;
  if(recording->events.size()>=65536||!owners.Identify(destination,256,dst)){recording->Incomplete("copy_destination_or_capacity");return;}
  Event e;e.build.destinationToken=dst.token;e.build.event.record.address=destination;e.build.event.operation=gi_reuse::ASOperation::Invalidate;
  if((mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE||mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)&&owners.Identify(source,256,src)){
   e.build.sourceToken=src.token;e.build.event.source=source;e.build.event.operation=gi_reuse::ASOperation::Copy;
  }
  recording->events.push_back(std::move(e));
 }catch(...){recording->Incomplete("observer_exception");}
}
static void WriterReady(const GINativeJobRoute::Ticket& ticket) noexcept {
 if(!ready.load())return;writerHandoff.Clear();if(!gi_reuse::ownedRequested.load())return;
 GIOwnedMetrics::Scope timing(GIOwnedMetrics::Writer);
 auto recording=ReadRecording(reinterpret_cast<ID3D12GraphicsCommandList4*>(ticket.list));
 const auto n=++writerPublications;
 const auto report=[&](const char* reason,const Writer* writer=nullptr){
  if(n<=8||n%600==0)Log("GI_OWNED_WRITER serial=%llu invocation=%llu generation=%llu published=%u reason=%s native_inputs=%u input_reason=%s",
   ticket.serial,rrNativeGIInvocation,recording?recording->generation:0ull,unsigned(writer!=nullptr),reason,
   unsigned(writer&&writer->nativeInputs),writer?writer->inputReason:"not_observed");
 };
 if(!rrNativeGIDispatchScope||!rrNativeGIInvocation){report("native_invocation");return;}
 if(!recording||!recording->complete||recording->submitted){report("producer_recording");return;}
 try {if(recording->events.size()>=65536){recording->Incomplete("writer_capacity");report("event_capacity");return;}
  auto writer=std::make_shared<Writer>(recording->generation);writer->ticket=ticket;
  gi_reuse::native_gi_detail::Header header;
  if(!gi_reuse::SnapshotNativeGIWriter(ticket,writer->inputs)||!gi_reuse::native_gi_detail::ReadHeader(ticket.writer,header)||header.pipeline!=ticket.pipeline){recording->Incomplete("writer_snapshot");report("writer_snapshot");return;}
  writer->opaque=header.opaque;writer->alpha=header.alpha;
  writer->pipeline=reinterpret_cast<ID3D12StateObject*>(ticket.pipeline);
  if(const auto fill=writerInputs.load())writer->nativeInputs=fill(*writer);
  Event e;e.writer=writer;recording->events.push_back(std::move(e));
  writerHandoff.Publish(rrNativeGIInvocation,ticket.engineFrame,writer);report("none",writer.get());
 }catch(...){recording->Incomplete("observer_exception");report("exception");}
}
static bool Install(HMODULE d3d) noexcept {
 if((!Enabled()&&!gi_reuse::runtime::FrozenSourcesEnabled())||!gi_reuse::runtime::Enabled()||!HashMatches(d3d,kD3dHash)||!HashMatches(verifiedRenderer,kRendererHash))return false;
 if(ready.load())return true;
 const bool sourcesOnly=gi_reuse::runtime::FrozenSourcesEnabled();
 if(!GINativeLifetimeRuntime::Install(d3d))return false;
 auto* b=reinterpret_cast<unsigned char*>(d3d);auto* r=reinterpret_cast<unsigned char*>(verifiedRenderer);
 general=reinterpret_cast<General>(b+0x26030);ray=reinterpret_cast<Ray>(b+0x26360);
 lockInstances=reinterpret_cast<LockInstances>(*reinterpret_cast<void**>(r+0x5dfe70));copyInstances=reinterpret_cast<CopyInstances>(r+0x588e0e);
 unsigned n=0;bool ok=true;
 for(unsigned offset:{0x25217u,0x2527fu,0x252f1u,0x25367u,0x253d1u,0x2544bu,0x254bbu,0x2551fu}){
  if(!RRAlbedoPrepareCallPatch(b+offset,reinterpret_cast<void*>(general),reinterpret_cast<void*>(&GeneralHook),&creates[n++])){ok=false;break;}}
 if(ok)for(unsigned offset:{0x2824fu,0x2ae34u,0x35763u}){
  if(!RRAlbedoPrepareCallPatch(b+offset,reinterpret_cast<void*>(ray),reinterpret_cast<void*>(&RayHook),&creates[n++])){ok=false;break;}}
 ok=ok&&n==11;
 if(ok&&!sourcesOnly)ok=lockPatch.Prepare(r+0xa8972,reinterpret_cast<void**>(r+0x5dfe70),reinterpret_cast<void*>(lockInstances),reinterpret_cast<void*>(&LockHook))&&
  RRAlbedoPrepareCallPatch(r+0xa8987,reinterpret_cast<void*>(copyInstances),reinterpret_cast<void*>(&CopyHook),&copyPatch);
 if(ok)for(auto& p:creates)if(!RRAlbedoExchangeCall(&p,true)){ok=false;break;}
 if(ok&&!sourcesOnly)ok=lockPatch.Exchange(true)&&RRAlbedoExchangeCall(&copyPatch,true);
 if(!ok){if(copyPatch.site)RRAlbedoExchangeCall(&copyPatch,false);if(lockPatch.changed)lockPatch.Exchange(false);for(auto& p:creates)if(p.site)RRAlbedoExchangeCall(&p,false);}
 if(ok){GINativeLifetimeRuntime::liveRetire.store(&Retire);if(!sourcesOnly){gi_reuse::runtime::liveBuild.store(&Build);gi_reuse::runtime::liveCopy.store(&Copy);gi_reuse::runtime::liveReset.store(&Reset);gi_reuse::runtime::liveBarriers.store(&Barriers);gi_reuse::runtime::liveInitialize.store(&Initialize);}ready=true;}
 Log("GI_LIVE_OWNERS ready=%u native_creates=11 instance_source_copy=%u replacement_dispatch=0",unsigned(ok),unsigned(!sourcesOnly));return ok;
}
}
