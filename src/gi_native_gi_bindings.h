#pragma once
#include "gi_native_scene_descriptors.h"
// Mirrors CPU descriptor sources only during the exact GI6 ConstantTable bind.
// The command list owns the completed snapshot. The exact ray invocation hands
// its writer across native list rotation; submission proves producer ordering.
namespace GINativeGIBindings {
using Microsoft::WRL::ComPtr;
using Bind=void(*)(void*);
static Bind native=nullptr;static RRAlbedoCallPatch patch{};
static std::atomic<bool> installed{false};
static const GUID key={0x310e0005,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
class Snapshot final:public IUnknown {
 std::atomic<ULONG> refs{1};
public:
 UINT64 generation=0;
 PTGlassRuntime::Context context;
 ComPtr<ID3D12DescriptorHeap> heap;
 PTGlass::LightingTableShadow shadow;
 std::shared_ptr<const GINativeSceneOwners::Writer> writer;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override {if(!out)return E_POINTER;*out=nullptr;
  if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
};
static thread_local bool capturing=false,rejected=false;
static thread_local ComPtr<Snapshot> current;
static thread_local std::shared_ptr<const GINativeSceneOwners::Writer> bindingWriter;
static thread_local const char* failure="no_descriptor_callback";
static thread_local UINT copies=0,constants=0;
static std::atomic<UINT64> binds{0};
static Snapshot* Reject(const char* reason) noexcept {failure=reason;rejected=true;return nullptr;}
static Snapshot* Active() noexcept {
 if(!capturing||rejected)return nullptr;if(current)return current.Get();
 try {
  PTGlassRuntime::Context context;if(!PTGlassRuntime::ReadContext(&context)){return Reject("context");}
  auto recording=GINativeSceneOwners::ReadRecording(context.rays,false);
  if(!recording)return Reject("recording_missing");
  if(!recording->complete)return Reject("recording_incomplete");
  if(recording->submitted)return Reject("recording_submitted");
  auto writer=bindingWriter;
  if(!writer)return Reject("writer_handoff_missing");
  if(!writer->nativeInputs)return Reject(writer->inputReason);
  ComPtr<Snapshot> snapshot;snapshot.Attach(new(std::nothrow) Snapshot);if(!snapshot)return Reject("snapshot_allocation");
  snapshot->generation=recording->generation;snapshot->context=context;snapshot->heap=context.table.heap;snapshot->writer=std::move(writer);
  GINativeSceneDescriptors::Mute mute;
  if(FAILED(snapshot->shadow.Initialize(PTGlassRuntime::device.Get()))||!snapshot->shadow.Begin(snapshot->generation,context.table)){return Reject("shadow_initialize");}
  failure="none";current=std::move(snapshot);return current.Get();
 }catch(...){return Reject("exception");}
}
static void Copy(D3D12_CPU_DESCRIPTOR_HANDLE destination,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE source) noexcept {
 if(capturing)++copies;
 if(auto* s=Active()){GINativeSceneDescriptors::Mute mute;s->shadow.ObserveNativeCopy(s->generation,destination,count,source);}
}
static void Constant(D3D12_CPU_DESCRIPTOR_HANDLE destination,const D3D12_CONSTANT_BUFFER_VIEW_DESC& cb) noexcept {
 if(capturing)++constants;
 if(auto* s=Active()){GINativeSceneDescriptors::Mute mute;s->shadow.ObserveNativeConstant(s->generation,destination,cb);}
}
static void Hook(void* shader){
 GIOwnedMetrics::Scope timing(GIOwnedMetrics::Bindings);
 current.Reset();rejected=false;failure="no_descriptor_callback";copies=constants=0;capturing=true;
 unsigned long long frame=0;DWORD fault=0;
 if(!ReadEngineFrameSafe(&frame,&fault))frame=0;
 bindingWriter=GINativeSceneOwners::writerHandoff.Take(rrNativeGIInvocation,frame);
 PTGlassRuntime::Context previous;if(PTGlassRuntime::ReadContext(&previous))previous.rays->SetPrivateDataInterface(key,nullptr);
 native(shader);
 const DWORD error=GetLastError();capturing=false;
 if(current){if(rejected)current->context.rays->SetPrivateDataInterface(key,nullptr);
  else current->context.rays->SetPrivateDataInterface(key,current.Get());}
 const auto n=++binds;
 if(n<=8||n%600==0){
  const auto stats=GINativeSceneDescriptors::index->Statistics();
  Log("GI_OWNED_BIND_RESULT call=%llu snapshot=%u reason=%s copies=%u constants=%u observed=%llx descriptor_failed=%u index_healthy=%u index_reason=%s as_writes=%llu entries=%llu producer_generation=%llu consumer_generation=%llu writer=%llu",
   n,unsigned(current.Get()!=nullptr&&!rejected),failure,copies,constants,current?current->shadow.Observed():0ull,
   unsigned(GINativeSceneDescriptors::failed.load()),unsigned(stats.healthy),stats.failure,stats.asWrites,stats.entries,
   bindingWriter?bindingWriter->generation:0ull,current?current->generation:0ull,bindingWriter?bindingWriter->ticket.serial:0ull);
 }
 current.Reset();bindingWriter.reset();SetLastError(error);
}
static ComPtr<Snapshot> Take(ID3D12GraphicsCommandList4* list) noexcept {
 ComPtr<Snapshot> result;if(!list)return result;
 IUnknown* value=nullptr;UINT bytes=sizeof(value);
 if(SUCCEEDED(list->GetPrivateData(key,&bytes,&value))&&value){
  list->SetPrivateDataInterface(key,nullptr);
  if(bytes==sizeof(value))result.Attach(static_cast<Snapshot*>(value));else value->Release();
 }
 if(result){const auto recording=GINativeSceneOwners::ReadRecording(list,false);
  if(!recording||!recording->complete||recording->submitted||recording->generation!=result->generation)result.Reset();}
 return result;
}
static bool Install() noexcept {
 if(!GINativeSceneOwners::Enabled()||!GINativeSceneOwners::ready.load()||!GINativeSceneDescriptors::installed.load())return false;
 if(installed.load())return true;
 auto* renderer=reinterpret_cast<unsigned char*>(verifiedRenderer);native=reinterpret_cast<Bind>(renderer+0x1db750);
 if(!RRAlbedoPrepareCallPatch(renderer+0x12cbd2,reinterpret_cast<void*>(native),reinterpret_cast<void*>(&Hook),&patch)||
    !RRAlbedoExchangeCall(&patch,true))return false;
 GINativeSceneDescriptors::copyObserver=&Copy;GINativeSceneDescriptors::constantObserver=&Constant;installed=true;
 Log("GI_OWNED_BINDINGS installed=1 native_bind=0x12cbd2 cpu_sources_only=1");return true;
}
}
