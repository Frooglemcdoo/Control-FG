#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <new>
#include "d3d12_geometry_metadata.h"
#include "submission_ledger.h"
#include "d3d12_buffer_owners.h"
namespace gi_reuse::runtime {
using Microsoft::WRL::ComPtr;
inline bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_METADATA",b,4)==1&&b[0]=='1';}();return value;}
inline const GUID journalKey={0x28611345,0x28ab,0x4127,{0x93,0x44,0x12,0x10,0x16,0x85,0x42,0x31}};
inline std::atomic<UINT64> generation{1};
inline std::atomic<bool> fault{false};
inline std::atomic<const char*> failureReason{nullptr};
inline void Fail(const char* reason) noexcept {const char* empty=nullptr;failureReason.compare_exchange_strong(empty,reason);fault=true;}
inline BufferOwners owners;
inline std::atomic<UINT64> journalPeak{0}, creationObservations{0}, barrierObservations{0};
inline void JournalPeak(std::size_t size) noexcept {auto old=journalPeak.load();while(old<size&&!journalPeak.compare_exchange_weak(old,size)){};}
inline std::atomic<UINT64> buildCount{0},copyCount{0},resolvedRanges{0},missingRanges{0};
inline void ObserveBuffer(ID3D12Resource* resource) noexcept {if(Enabled())owners.Observe(resource);}
inline void ObserveCreated(IUnknown* object,D3D12_RESOURCE_STATES initialState=D3D12_RESOURCE_STATE_COMMON) noexcept {
 if(!Enabled()||!object)return;ComPtr<ID3D12Resource> resource;
 if(SUCCEEDED(object->QueryInterface(IID_PPV_ARGS(&resource)))&&resource->GetDesc().Dimension==D3D12_RESOURCE_DIMENSION_BUFFER){++creationObservations;owners.Observe(resource.Get(),initialState);}
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
inline std::atomic<UINT64> absentRanges{0},ambiguousRanges{0},invalidRanges{0};
inline void CheckOwner(UINT64 address,UINT64 bytes,OwnerKind kind) noexcept {
 ComPtr<ID3D12Resource> owner;UINT64 offset=0;RangeMatch reason{};
 if(owners.Resolve(address,bytes,owner,offset,&reason)){++resolvedRanges;return;}
 ++missingRanges;if(reason==RangeMatch::Ambiguous)++ambiguousRanges;else if(reason==RangeMatch::Invalid)++invalidRanges;else ++absentRanges;
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
 Journal(){data.generation=generation.fetch_add(1);}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
  if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
  *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override {return ++references_;}
 ULONG STDMETHODCALLTYPE Release() override {const auto n=--references_;if(!n)delete this;return n;}
};
inline ComPtr<Journal> Read(ID3D12Object* list) noexcept {
 ComPtr<Journal> result;if(!list)return result;UINT size=sizeof(IUnknown*);IUnknown* value=nullptr;
 if(SUCCEEDED(list->GetPrivateData(journalKey,&size,&value))&&value){
  if(size==sizeof(value))result.Attach(static_cast<Journal*>(value));else value->Release();
 }
 return result;
}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled()||fault.load()||!list)return;
 auto* next=new(std::nothrow) Journal;if(!next){Fail("journal_allocation");return;}
 if(FAILED(list->SetPrivateDataInterface(journalKey,next)))Fail("journal_attachment");
 next->Release();
}
inline void Initialize(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled()||fault.load())return;if(!Read(list))Reset(list);
}
inline void RecordBuild(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
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
   r.geometry=CopyBLASGeometry(desc->Inputs);if(!r.geometry){Fail("blas_unsupported_descriptor");return;}
  }
  if(r.topLevel)CheckOwner(r.instances,UINT64(r.count)*64,OwnerKind::Instances);
  else for(UINT i=0;i<r.count;++i){const auto* t=r.geometry->At(i);CheckOwner(t->vertices,UINT64(t->vertexCount)*t->stride,OwnerKind::Vertex);CheckOwner(t->indices,UINT64(t->indexCount)*2,OwnerKind::Index);if(t->transform)CheckOwner(t->transform,48,OwnerKind::Transform);}
  if(!journal->data.Append({update?ASOperation::Update:ASOperation::Build,r,update?desc->SourceAccelerationStructureData:0}))Fail("build_journal_capacity");
  JournalPeak(journal->data.size);
 }catch(...){Fail("build_allocation_exception");}
}
inline void RecordCopy(ID3D12GraphicsCommandList4* list,UINT64 destination,UINT64 source,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode) noexcept {
 if(!Enabled())return;++copyCount;if(fault.load())return;auto journal=Read(list);
 if(!journal){Fail("copy_missing_journal");return;}
 if(!destination||!source||(mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE&&mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)){Fail("copy_address_or_mode");return;}
 try {if(!journal->data.Append({ASOperation::Copy,{destination},source}))Fail("copy_journal_capacity");JournalPeak(journal->data.size);}
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
inline ASLedger<8192> ledger;
inline std::atomic<UINT64> ignoredCopySubmissions{0},ignoredEmptySubmissions{0},asSubmissions{0};
inline UINT64 BeforeExecute(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled()||fault.load())return 0;
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
   if(!fault.load()){submitting=true;token=activeToken=++nextFence;++asSubmissions;}
  }
 }
 ReleaseSRWLockExclusive(&stateLock);return token;
}
inline void AfterExecute(ID3D12CommandQueue* q,UINT64 token) noexcept {
 if(!Enabled()||!token)return;
 AcquireSRWLockExclusive(&stateLock);
 if(!submitting||token!=activeToken||q!=queue.Get())Fail("submission_token_mismatch");
 if(!fault.load()){
  if(FAILED(q->Signal(fence.Get(),token)))Fail("queue_signal");else if(!ledger.Submit(reinterpret_cast<UINT64>(q),token,pending,pending.generation))Fail(ledger.FailureReason());
 }
 pending={};submitting=false;activeToken=0;
 ReleaseSRWLockExclusive(&stateLock);
}
inline bool Snapshot(ASLedger<8192>& result) noexcept {
 if(!Enabled()||fault.load())return false;AcquireSRWLockExclusive(&stateLock);
 const bool valid=!fault.load()&&!submitting&&fence&&ledger.Snapshot(reinterpret_cast<UINT64>(queue.Get()),fence->GetCompletedValue(),result);
 ReleaseSRWLockExclusive(&stateLock);return valid;
}
inline void CompletionStats(UINT64& issued,UINT64& completed,bool& ready) noexcept {
 AcquireSRWLockShared(&stateLock);issued=nextFence;completed=fence?fence->GetCompletedValue():0;
 ready=!fault.load()&&!submitting&&fence&&completed!=UINT64_MAX&&completed>=issued&&ledger.Complete();ReleaseSRWLockShared(&stateLock);
}
// Snapshot proves only observed descriptor submission completion, never lifetime
// of vertex/index/instance resources or same-generation bytes. No rendering user.
} // namespace gi_reuse::runtime
