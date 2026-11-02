#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <new>
#include "d3d12_geometry_metadata.h"
#include "submission_ledger.h"
namespace gi_reuse::runtime {
using Microsoft::WRL::ComPtr;
inline bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_METADATA",b,4)==1&&b[0]=='1';}();return value;}
inline const GUID journalKey={0x28611345,0x28ab,0x4127,{0x93,0x44,0x12,0x10,0x16,0x85,0x42,0x31}};
inline std::atomic<UINT64> generation{1};
inline std::atomic<bool> fault{false};
// The COM private-data owner follows command-list destruction and successful
// resets. No raw command-list pointers or native descriptor pointers are kept.
class Journal final:public IUnknown {
 std::atomic<ULONG> references_{1};
public:
 ASJournal<512> data;
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
 auto* next=new(std::nothrow) Journal;if(!next){fault=true;return;}
 if(FAILED(list->SetPrivateDataInterface(journalKey,next)))fault=true;
 next->Release();
}
inline void Initialize(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled()||fault.load())return;if(!Read(list))Reset(list);
}
inline void RecordBuild(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
 if(!Enabled()||fault.load())return;
 try {
  auto journal=Read(list);if(!journal||!desc||!desc->DestAccelerationStructureData){fault=true;return;}
  ASRecord r{};r.address=desc->DestAccelerationStructureData;r.count=desc->Inputs.NumDescs;
  // Updates require resolved prior descriptors and same-generation resources.
  // They are not equivalent to a fresh fully resolved Build event yet.
  if(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE){fault=true;return;}
  if(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
   if(desc->Inputs.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY||!desc->Inputs.InstanceDescs){fault=true;return;}
   r.topLevel=true;r.instances=desc->Inputs.InstanceDescs;
  }else{
   r.geometry=CopyBLASGeometry(desc->Inputs);if(!r.geometry){fault=true;return;}
  }
  if(!journal->data.Append({ASOperation::Build,r,0}))fault=true;
 }catch(...){fault=true;}
}
inline void RecordCopy(ID3D12GraphicsCommandList4* list,UINT64 destination,UINT64 source,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode) noexcept {
 if(!Enabled()||fault.load())return;auto journal=Read(list);
 if(!journal||!destination||!source||(mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE&&mode!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)){fault=true;return;}
 if(!journal->data.Append({ASOperation::Copy,{destination},source}))fault=true;
}
// No CPU wait, queue splitting, or GPU buffer access. A second queue, concurrent
// Execute, or multiple AS-bearing lists in one batch invalidates this restricted
// observer rather than claiming a dependency not established by the game.
inline SRWLOCK stateLock=SRWLOCK_INIT;
inline bool submitting=false;
inline UINT64 nextFence=0,activeToken=0;
inline ComPtr<ID3D12CommandQueue> queue;
inline ComPtr<ID3D12Fence> fence;
inline ASJournal<512> pending;
inline ASLedger<8192> ledger;
inline UINT64 BeforeExecute(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled()||fault.load())return 0;
 AcquireSRWLockExclusive(&stateLock);
 UINT64 token=0;
 if(submitting||!q||(count&&!lists)){fault=true;}
 else if(queue&&queue.Get()!=q){fault=true;}
 else {
  unsigned relevant=0;pending={};
  for(UINT i=0;i<count&&!fault.load();++i){auto j=Read(lists[i]);
   // Uninstrumented lists can contain unseen builds. No partial coverage claim.
   if(!j||!j->data.complete){fault=true;break;}
   if(j->data.size){if(++relevant>1){fault=true;break;}pending=j->data;}
  }
  if(!fault.load()&&relevant){
   if(!fence){ComPtr<ID3D12Device> d;if(FAILED(q->GetDevice(IID_PPV_ARGS(&d)))||FAILED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))fault=true;else queue=q;}
   if(!fault.load()){submitting=true;token=activeToken=++nextFence;}
  }
 }
 ReleaseSRWLockExclusive(&stateLock);return token;
}
inline void AfterExecute(ID3D12CommandQueue* q,UINT64 token) noexcept {
 if(!Enabled()||!token)return;
 AcquireSRWLockExclusive(&stateLock);
 if(!submitting||token!=activeToken||q!=queue.Get())fault=true;
 if(!fault.load()){
  if(FAILED(q->Signal(fence.Get(),token))||!ledger.Submit(reinterpret_cast<UINT64>(q),token,pending,pending.generation))fault=true;
 }
 pending={};submitting=false;activeToken=0;
 ReleaseSRWLockExclusive(&stateLock);
}
inline bool Snapshot(ASLedger<8192>& result) noexcept {
 if(!Enabled()||fault.load())return false;AcquireSRWLockExclusive(&stateLock);
 const bool valid=!fault.load()&&!submitting&&fence&&ledger.Snapshot(reinterpret_cast<UINT64>(queue.Get()),fence->GetCompletedValue(),result);
 ReleaseSRWLockExclusive(&stateLock);return valid;
}
// Snapshot proves only observed descriptor submission completion, never lifetime
// of vertex/index/instance resources or same-generation bytes. No rendering user.
} // namespace gi_reuse::runtime
