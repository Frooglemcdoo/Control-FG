#pragma once
#include "gi_native_scene_descriptors.h"
// Mirrors CPU descriptor sources only during the exact GI6 ConstantTable bind.
// The command list owns the completed snapshot, so another recording thread or
// TLS pass cannot silently substitute a different writer/table.
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
static Snapshot* Active() noexcept {
 if(!capturing||rejected)return nullptr;if(current)return current.Get();
 try {
  PTGlassRuntime::Context context;if(!PTGlassRuntime::ReadContext(&context)){rejected=true;return nullptr;}
  auto recording=GINativeSceneOwners::ReadRecording(context.rays,false);
  if(!recording||!recording->complete||recording->submitted){rejected=true;return nullptr;}
  std::shared_ptr<const GINativeSceneOwners::Writer> writer;
  for(auto event=recording->events.rbegin();event!=recording->events.rend();++event)if(event->writer){writer=event->writer;break;}
  if(!writer||!writer->nativeInputs||writer->ticket.list!=reinterpret_cast<UINT_PTR>(context.rays)){rejected=true;return nullptr;}
  ComPtr<Snapshot> snapshot;snapshot.Attach(new(std::nothrow) Snapshot);if(!snapshot){rejected=true;return nullptr;}
  snapshot->generation=recording->generation;snapshot->context=context;snapshot->heap=context.table.heap;snapshot->writer=std::move(writer);
  GINativeSceneDescriptors::Mute mute;
  if(FAILED(snapshot->shadow.Initialize(PTGlassRuntime::device.Get()))||!snapshot->shadow.Begin(snapshot->generation,context.table)){rejected=true;return nullptr;}
  current=std::move(snapshot);return current.Get();
 }catch(...){rejected=true;return nullptr;}
}
static void Copy(D3D12_CPU_DESCRIPTOR_HANDLE destination,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE source) noexcept {
 if(auto* s=Active()){GINativeSceneDescriptors::Mute mute;s->shadow.ObserveNativeCopy(s->generation,destination,count,source);}
}
static void Constant(D3D12_CPU_DESCRIPTOR_HANDLE destination,const D3D12_CONSTANT_BUFFER_VIEW_DESC& cb) noexcept {
 if(auto* s=Active()){GINativeSceneDescriptors::Mute mute;s->shadow.ObserveNativeConstant(s->generation,destination,cb);}
}
static void Hook(void* shader){
 current.Reset();rejected=false;capturing=true;
 PTGlassRuntime::Context previous;if(PTGlassRuntime::ReadContext(&previous))previous.rays->SetPrivateDataInterface(key,nullptr);
 native(shader);
 const DWORD error=GetLastError();capturing=false;
 if(current){if(rejected)current->context.rays->SetPrivateDataInterface(key,nullptr);
  else current->context.rays->SetPrivateDataInterface(key,current.Get());}
 current.Reset();SetLastError(error);
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
