#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <new>
#include "d3d12_geometry_metadata.h"
#include "submission_ledger.h"
#include "scene_read_plan.h"
#include "local_buffer_states.h"
#include "d3d12_buffer_owners.h"
#include "d3d12_as_owners.h"
#include "d3d12_storage_lease.h"
#include "d3d12_compute_bindings.h"
#include "raw_snapshot_plan.h"
#include "index_snapshot_plan.h"
namespace gi_reuse::runtime {
using Microsoft::WRL::ComPtr;
inline bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_METADATA",b,4)==1&&b[0]=='1';}();return value;}
inline const GUID journalKey={0x28611345,0x28ab,0x4127,{0x93,0x44,0x12,0x10,0x16,0x85,0x42,0x31}};
inline std::atomic<UINT64> generation{1};
inline std::atomic<bool> fault{false};
inline std::atomic<const char*> failureReason{nullptr};
inline void Fail(const char* reason) noexcept {const char* empty=nullptr;failureReason.compare_exchange_strong(empty,reason);fault=true;}
// Independent live-scene observers must still see writes after a diagnostic
// inventory reaches its cap. They own their own identities and admission.
inline std::atomic<void(*)(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*) noexcept> liveBuild{nullptr};
inline std::atomic<void(*)(ID3D12GraphicsCommandList4*,UINT64,UINT64,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE) noexcept> liveCopy{nullptr};
inline std::atomic<void(*)(ID3D12GraphicsCommandList*) noexcept> liveReset{nullptr};
inline std::atomic<void(*)(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*) noexcept> liveBarriers{nullptr};
inline std::atomic<void(*)(ID3D12GraphicsCommandList*) noexcept> liveInitialize{nullptr};
inline BufferOwners owners;
inline ASResourceOwners<> asOwners;
inline std::atomic<UINT64> asIdentityResolved{0},asIdentityMissing{0},asIdentityAmbiguous{0};
inline std::atomic<UINT64> journalPeak{0}, creationObservations{0}, barrierObservations{0};
inline void JournalPeak(std::size_t size) noexcept {auto old=journalPeak.load();while(old<size&&!journalPeak.compare_exchange_weak(old,size)){};}
inline std::atomic<UINT64> buildCount{0},copyCount{0},resolvedRanges{0},missingRanges{0};
inline void ObserveBuffer(ID3D12Resource* resource) noexcept {if(Enabled())owners.Observe(resource);}
inline void ObserveCreated(IUnknown* object,D3D12_RESOURCE_STATES initialState=D3D12_RESOURCE_STATE_COMMON,
                           bool supportedASStorage=false,ID3D12Heap* placedHeap=nullptr) noexcept {
 if(!Enabled()||!object)return;ComPtr<ID3D12Resource> resource;
 if(SUCCEEDED(object->QueryInterface(IID_PPV_ARGS(&resource)))&&resource->GetDesc().Dimension==D3D12_RESOURCE_DIMENSION_BUFFER){
  ++creationObservations;
  // A creation hook supplies backing-storage proof. Barrier-only discoveries
  // and reserved/tiled resources deliberately remain ineligible for snapshots.
  if(supportedASStorage)RetainCreatedStorage(resource.Get(),placedHeap);
  owners.Observe(resource.Get(),initialState);
  if(supportedASStorage&&(initialState&D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))asOwners.Created(resource.Get(),placedHeap);
 }
}
inline void ObserveBarriers(UINT count,const D3D12_RESOURCE_BARRIER* barriers) noexcept {
 if(!Enabled()||!barriers)return;
 for(UINT i=0;i<count;++i){const auto& b=barriers[i];++barrierObservations;
  if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)owners.Observe(b.Transition.pResource,b.Transition.StateAfter);
  else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV)owners.Observe(b.UAV.pResource);
  else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){owners.Observe(b.Aliasing.pResourceBefore);owners.Observe(b.Aliasing.pResourceAfter);}
 }
}
enum class OwnerKind:unsigned {Vertex,Index,Transform,Instances};
struct MissingRange {UINT64 address=0,bytes=0,hits=0,lastBuild=0;OwnerKind kind{};RangeMatch reason{};};
inline SRWLOCK missingLock=SRWLOCK_INIT;
inline std::array<MissingRange,128> missingSamples{};inline unsigned missingCursor=0;
inline std::array<std::atomic<UINT64>,4> missingByKind{};
inline std::atomic<UINT64> absentRanges{0},ambiguousRanges{0},invalidRanges{0};
inline std::array<std::array<std::atomic<UINT64>,4>,4> localStateCounts{};
inline std::atomic<UINT64> localStateLimits{0},localAliasInvalidations{0};
// R295 input preflight uses the already-resolved owner. These counters do not
// establish alias/residency/queue correctness and never authorize a GPU read.
inline bool InputAuditEnabled() noexcept {static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_BINDINGS",v,4)==1&&v[0]=='1';}();return enabled;}
inline std::array<std::array<std::atomic<UINT64>,6>,4> inputAudit{};
inline std::array<std::atomic<UINT64>,5> indexRawReasons{};
inline std::atomic<UINT64> exactIndexEligible{0};
inline void AuditInput(ID3D12Resource* resource,UINT64 offset,UINT64 bytes,OwnerKind kind) noexcept {
 if(!InputAuditEnabled())return;auto& counts=inputAudit[static_cast<unsigned>(kind)];
 const auto desc=resource->GetDesc();BufferStorageLease lease;RawSnapshotPlan plan;
 const bool storage=AcquireStorageLease(resource,lease);
 ++counts[storage?(lease.kind==BufferStorageKind::Placed?1u:0u):2u];
 if(desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)++counts[3];
 RawSnapshotReject reason;const bool rawOK=RawSnapshotPlan::Make(desc.Width,offset,bytes,plan,&reason);
 if(!rawOK)++counts[4];
 if(storage&&!(desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)&&rawOK)++counts[5];
 if(kind==OwnerKind::Index){++indexRawReasons[static_cast<unsigned>(reason)];IndexSnapshotPlan exact;
  if(storage&&!(desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)&&IndexSnapshotPlan::Make(desc.Width,offset,bytes,exact))++exactIndexEligible;
 }
}

inline void CheckOwner(UINT64 address,UINT64 bytes,OwnerKind kind,LocalBufferStates<>* states=nullptr) noexcept {
 ComPtr<ID3D12Resource> owner;UINT64 offset=0;RangeMatch reason{};
 if(owners.Resolve(address,bytes,owner,offset,&reason)){
  ++resolvedRanges;AuditInput(owner.Get(),offset,bytes,kind);
  if(states){const auto state=states->Find(owners.Identity(owner.Get()));unsigned category=0;
   if(state.kind==LocalStateKind::Pending)category=2;
   else if(state.kind==LocalStateKind::Explicit)category=(state.after&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)?1u:3u;
   ++localStateCounts[static_cast<unsigned>(kind)][category];
  }
  return;
 }
 ++missingRanges;++missingByKind[static_cast<unsigned>(kind)];if(reason==RangeMatch::Ambiguous)++ambiguousRanges;else if(reason==RangeMatch::Invalid)++invalidRanges;else ++absentRanges;
 AcquireSRWLockExclusive(&missingLock);MissingRange* found=nullptr;
 for(auto& sample:missingSamples)if(sample.hits&&sample.address==address&&sample.bytes==bytes&&sample.kind==kind&&sample.reason==reason){found=&sample;break;}
 if(!found){found=&missingSamples[missingCursor++%missingSamples.size()];*found={address,bytes,0,0,kind,reason};}
 ++found->hits;found->lastBuild=buildCount.load();ReleaseSRWLockExclusive(&missingLock);
}
inline void ReadMissing(std::array<MissingRange,128>& output) noexcept {
 AcquireSRWLockShared(&missingLock);output=missingSamples;ReleaseSRWLockShared(&missingLock);
}
// The COM private-data owner follows command-list destruction and successful
// resets. No raw command-list pointers or native descriptor pointers are kept.
class Journal final:public IUnknown {
 std::atomic<ULONG> references_{1};
public:
 GrowingASJournal<65536> data;
 std::vector<std::pair<UINT64,UINT64>> asTokens;
 std::unordered_map<UINT64,ASResourceOwners<>::Lease> asLeases;
 UINT64 missingASLeases=0;
 LocalBufferStates<> states;
 Journal(){data.generation=generation.fetch_add(1);}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
  if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
  *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override {return ++references_;}
 ULONG STDMETHODCALLTYPE Release() override {const auto n=--references_;if(!n)delete this;return n;}
};
inline std::pair<UINT64,UINT64> ASIdentities(UINT64 destination,UINT64 source,Journal* journal=nullptr) {
 const auto lookup=[journal](UINT64 address){ASResourceOwners<>::Lease lease;RangeMatch reason{};
  if(asOwners.Resolve(address,lease,&reason)){++asIdentityResolved;const auto token=lease.token;
   if(journal)journal->asLeases.emplace(token,std::move(lease));return token;}
  if(journal)++journal->missingASLeases;
  ++asIdentityMissing;if(reason==RangeMatch::Ambiguous)++asIdentityAmbiguous;return UINT64(0);
 };
 return {lookup(destination),source?lookup(source):0};
}
inline ComPtr<Journal> Read(ID3D12Object* list) noexcept {
 ComPtr<Journal> result;if(!list)return result;UINT size=sizeof(IUnknown*);IUnknown* value=nullptr;
 if(SUCCEEDED(list->GetPrivateData(journalKey,&size,&value))&&value){
  if(size==sizeof(value))result.Attach(static_cast<Journal*>(value));else value->Release();
 }
 return result;
}
inline void RecordBarriers(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RESOURCE_BARRIER* barriers) noexcept {
 if(const auto callback=liveBarriers.load())callback(list,count,barriers);
 if(!Enabled()||fault.load()||!list||!barriers)return;auto j=Read(list);if(!j)return;
 const bool wasComplete=j->states.Complete();
 for(UINT i=0;i<count;++i){const auto& b=barriers[i];
  if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){j->states.Clear();++localAliasInvalidations;continue;}
  if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION||!b.Transition.pResource)continue;
  const auto id=owners.Identity(b.Transition.pResource);if(!id)continue;
  if(b.Transition.Subresource!=0&&b.Transition.Subresource!=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES){j->states.Clear();continue;}
  j->states.Transition(id,unsigned(b.Transition.StateBefore),unsigned(b.Transition.StateAfter),unsigned(b.Flags));
 }
 if(wasComplete&&!j->states.Complete())++localStateLimits;
}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(const auto callback=liveReset.load())callback(list);
 if(!Enabled()||fault.load()||!list)return;
 auto* next=new(std::nothrow) Journal;if(!next){Fail("journal_allocation");return;}
 if(FAILED(list->SetPrivateDataInterface(journalKey,next)))Fail("journal_attachment");
 next->Release();
}
inline void Initialize(ID3D12GraphicsCommandList* list) noexcept {
 if(const auto callback=liveInitialize.load())callback(list);
 if(!Enabled()||fault.load())return;if(!Read(list))Reset(list);
}
struct RejectedGeometry {GeometryDiagnostic diagnostic;UINT64 destination=0,source=0;UINT count=0,layout=0,flags=0;};
inline SRWLOCK rejectionLock=SRWLOCK_INIT;
inline RejectedGeometry rejectedGeometry{};
inline bool haveRejectedGeometry=false;
inline void RememberGeometryReject(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc,const GeometryDiagnostic& diagnostic) noexcept {
 AcquireSRWLockExclusive(&rejectionLock);
 if(!haveRejectedGeometry){rejectedGeometry={diagnostic,desc.DestAccelerationStructureData,desc.SourceAccelerationStructureData,desc.Inputs.NumDescs,unsigned(desc.Inputs.DescsLayout),unsigned(desc.Inputs.Flags)};haveRejectedGeometry=true;}
 ReleaseSRWLockExclusive(&rejectionLock);
}
inline bool ReadGeometryReject(RejectedGeometry& out) noexcept {
 AcquireSRWLockShared(&rejectionLock);const bool found=haveRejectedGeometry;if(found)out=rejectedGeometry;ReleaseSRWLockShared(&rejectionLock);return found;
}
inline void RecordBuild(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
 if(const auto callback=liveBuild.load())callback(list,desc);
 if(!Enabled())return;++buildCount;if(fault.load())return;
 try {
  auto journal=Read(list);if(!journal){Fail("build_missing_journal");return;}
  if(!desc||!desc->DestAccelerationStructureData){Fail("build_invalid_descriptor");return;}
  ASRecord r{};r.address=desc->DestAccelerationStructureData;r.count=desc->Inputs.NumDescs;
  const bool update=(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE)!=0;
  r.allowUpdate=(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)!=0;
  if(update&&!desc->SourceAccelerationStructureData){Fail("update_missing_source");return;}
  if(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
   if(desc->Inputs.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY||!desc->Inputs.InstanceDescs){Fail("tlas_layout_or_instances");return;}
   r.topLevel=true;r.instances=desc->Inputs.InstanceDescs;
  }else{
   GeometryDiagnostic diagnostic;r.geometry=CopyBLASGeometry(desc->Inputs,&diagnostic);if(!r.geometry){RememberGeometryReject(*desc,diagnostic);Fail("blas_unsupported_descriptor");return;}
  }
  if(r.topLevel)CheckOwner(r.instances,UINT64(r.count)*64,OwnerKind::Instances,&journal->states);
  else for(UINT i=0;i<r.count;++i){const auto* t=r.geometry->At(i);CheckOwner(t->vertices,GeometryMetadata::VertexBytes(*t),OwnerKind::Vertex,&journal->states);CheckOwner(t->indices,GeometryMetadata::IndexBytes(*t),OwnerKind::Index,&journal->states);if(t->transform)CheckOwner(t->transform,48,OwnerKind::Transform,&journal->states);}
  const auto source=update?desc->SourceAccelerationStructureData:0;
  journal->asTokens.push_back(ASIdentities(r.address,source,journal.Get()));
  if(!journal->data.Append({update?ASOperation::Update:ASOperation::Build,r,source}))Fail("build_journal_capacity");
  JournalPeak(journal->data.size);
 }catch(...){Fail("build_allocation_exception");}
}
inline void RecordCopy(ID3D12GraphicsCommandList4* list,UINT64 destination,UINT64 source,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode) noexcept {
 if(const auto callback=liveCopy.load())callback(list,destination,source,mode);
 if(!Enabled())return;++copyCount;if(fault.load())return;auto journal=Read(list);
 if(!journal){Fail("copy_missing_journal");return;}
 if(!destination||!source||(mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE&&mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)){Fail("copy_address_or_mode");return;}
 try {journal->asTokens.push_back(ASIdentities(destination,source,journal.Get()));if(!journal->data.Append({ASOperation::Copy,{destination},source}))Fail("copy_journal_capacity");JournalPeak(journal->data.size);}
 catch(...){Fail("copy_allocation_exception");}
}
// No CPU wait, queue splitting, or GPU buffer access. A second queue, concurrent
// Execute, or multiple AS-bearing lists in one batch invalidates this restricted
// observer rather than claiming a dependency not established by the game.
inline SRWLOCK stateLock=SRWLOCK_INIT;
inline bool submitting=false;
inline UINT64 nextFence=0,activeToken=0;
inline ComPtr<ID3D12CommandQueue> queue;
inline ComPtr<ID3D12Fence> fence;
inline GrowingASJournal<65536> pending;
using MetadataLedger=ASLedger<65536>;
inline MetadataLedger ledger;
inline std::atomic<UINT64> ignoredCopySubmissions{0},ignoredEmptySubmissions{0},asSubmissions{0};
// Keep recorded AS references through actual completion even if the native
// command list resets. A failed signal or uncertain submission pins the entry.
// Covers recorded build/copy roots, not every BLAS referenced by traversal.
// R306 drops obsolete diagnostic inventory references after native deferred
// release; native traversal lifetime remains native-owned. Extra GI dispatches
// still require a complete independently retained traversal closure.
struct ASFlight {ComPtr<Journal> journal;UINT64 fenceValue=0;bool signaled=false;};
inline std::array<ASFlight,256> asFlights;
inline std::size_t activeFlight=asFlights.size();
inline std::atomic<UINT64> asLeaseSubmitted{0},asLeaseRetired{0},asLeaseMissing{0};
inline void PollASLeases() noexcept {
 std::array<ComPtr<Journal>,256> released;
 AcquireSRWLockExclusive(&stateLock);
 const auto completed=fence?fence->GetCompletedValue():0;
 if(completed!=UINT64_MAX)for(std::size_t i=0;i<asFlights.size();++i){auto& f=asFlights[i];
  if(f.journal&&f.signaled&&f.fenceValue<=completed){released[i]=std::move(f.journal);f={};++asLeaseRetired;}
 }
 ReleaseSRWLockExclusive(&stateLock); // Destroy references outside observer locks.
}
inline UINT64 BeforeExecute(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled())return 0;PollASLeases();if(fault.load())return 0;
 if(!q||(count&&!lists)){Fail("invalid_submission");return 0;}
 // Copy queues cannot contain AS build/copy commands. This says nothing about
 // geometry byte dependencies; those remain unverified by this metadata observer.
 if(q->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_COPY){++ignoredCopySubmissions;return 0;}
 AcquireSRWLockExclusive(&stateLock);
 UINT64 token=0;unsigned relevant=0;ComPtr<Journal> selected;
 for(UINT i=0;i<count&&!fault.load();++i){auto j=Read(lists[i]);
  // Unknown direct/compute lists remain unknown, never assumed empty.
  if(!j||!j->data.complete){Fail("unobserved_or_incomplete_list");break;}
  if(j->data.size){if(++relevant>1){Fail("multiple_as_lists_in_batch");break;}selected=j;}
 }
 if(!fault.load()&&!relevant)++ignoredEmptySubmissions;
 if(!fault.load()&&relevant){
  if(submitting)Fail("concurrent_as_submission");
  else if(queue&&queue.Get()!=q)Fail("multiple_as_queues");
  else {
   try{pending=selected->data;}catch(...){Fail("submission_snapshot_allocation");}
   if(!fault.load()&&!fence){ComPtr<ID3D12Device> d;if(FAILED(q->GetDevice(IID_PPV_ARGS(&d))))Fail("queue_device");else if(FAILED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))Fail("fence_creation");else queue=q;}
   if(!fault.load()){
    std::size_t slot=0;while(slot<asFlights.size()&&asFlights[slot].journal)++slot;
    if(slot==asFlights.size())Fail("as_lease_submission_capacity");
    else if(nextFence>=UINT64_MAX-1)Fail("as_lease_fence_overflow");
    else {submitting=true;token=activeToken=++nextFence;++asSubmissions;activeFlight=slot;
     asFlights[slot]={selected,token,false};++asLeaseSubmitted;asLeaseMissing+=selected->missingASLeases;}
   }
  }
 }
 ReleaseSRWLockExclusive(&stateLock);return token;
}
inline void AfterExecute(ID3D12CommandQueue* q,UINT64 token) noexcept {
 if(!Enabled()||!token)return;
 AcquireSRWLockExclusive(&stateLock);
 if(!submitting||token!=activeToken||q!=queue.Get())Fail("submission_token_mismatch");
 if(!fault.load()){
  if(FAILED(q->Signal(fence.Get(),token)))Fail("queue_signal");
  else {if(activeFlight<asFlights.size())asFlights[activeFlight].signaled=true;
   if(!ledger.Submit(reinterpret_cast<UINT64>(q),token,pending,pending.generation))Fail(ledger.FailureReason());}
 }
 pending={};submitting=false;activeToken=0;activeFlight=asFlights.size();
 ReleaseSRWLockExclusive(&stateLock);
}
inline bool Snapshot(MetadataLedger& result) noexcept {
 if(!Enabled()||fault.load())return false;AcquireSRWLockExclusive(&stateLock);
 const bool valid=!fault.load()&&!submitting&&fence&&ledger.Snapshot(reinterpret_cast<UINT64>(queue.Get()),fence->GetCompletedValue(),result);
 ReleaseSRWLockExclusive(&stateLock);return valid;
}
inline bool CutPublicationReady() noexcept {
 AcquireSRWLockShared(&stateLock);const bool ready=!fault.load()&&!submitting;
 ReleaseSRWLockShared(&stateLock);return ready;
}
inline void CompletionStats(UINT64& issued,UINT64& completed,bool& ready) noexcept {
 AcquireSRWLockShared(&stateLock);issued=nextFence;completed=fence?fence->GetCompletedValue():0;
 ready=!fault.load()&&!submitting&&fence&&completed!=UINT64_MAX&&completed>=issued&&ledger.Complete();ReleaseSRWLockShared(&stateLock);
}
inline UINT64 LedgerSize() noexcept {
 AcquireSRWLockShared(&stateLock);const auto size=ledger.Size();ReleaseSRWLockShared(&stateLock);return static_cast<UINT64>(size);
}
struct ScenePlanStats {
 bool snapshot=false,planned=false;
 UINT64 fenceValue=0,blas=0,tlas=0,ranges=0,owners=0,resolved=0,missing=0;
 std::array<UINT64,4> missingByKind{};
 UINT64 defaultRanges=0,uploadRanges=0,otherRanges=0,requestedBytes=0;
};
inline ScenePlanStats InspectScenePlan() noexcept {
 ScenePlanStats s{};
 try {
  auto snapshot=std::make_unique<MetadataLedger>();
  if(!Snapshot(*snapshot))return s;
  s.snapshot=true;s.fenceValue=snapshot->SubmissionFence();
  SceneReadPlan<> plan;if(!plan.Build(*snapshot))return s;
  s.planned=true;s.blas=plan.BLASCount();s.tlas=plan.TLASCount();s.ranges=plan.Ranges().size();
  std::unordered_map<ID3D12Resource*,ComPtr<ID3D12Resource>> unique;
  for(const auto& r:plan.Ranges()){
   if(r.bytes>UINT64_MAX-s.requestedBytes){s.planned=false;return s;}
   s.requestedBytes+=r.bytes;
   ComPtr<ID3D12Resource> owner;UINT64 offset=0;
   if(!owners.Resolve(r.address,r.bytes,owner,offset,nullptr,false)){++s.missing;++s.missingByKind[static_cast<unsigned>(r.kind)];continue;}
   ++s.resolved;unique.emplace(owner.Get(),owner);
   D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
   if(FAILED(owner->GetHeapProperties(&heap,&flags)))++s.otherRanges;
   else if(heap.Type==D3D12_HEAP_TYPE_DEFAULT)++s.defaultRanges;
   else if(heap.Type==D3D12_HEAP_TYPE_UPLOAD)++s.uploadRanges;
   else ++s.otherRanges;
  }
  s.owners=unique.size();
 }catch(...){s.planned=false;}
 return s;
}
// Snapshot proves only observed descriptor submission completion, never lifetime
// of vertex/index/instance resources or same-generation bytes. No rendering user.
} // namespace gi_reuse::runtime
