#pragma once
#include "gi_native_scene_owners.h"
#include "gi_reuse/as_descriptor_index.h"
#include "rr_specular_native_clamp.h"
namespace GINativeSceneDescriptors {
using Microsoft::WRL::ComPtr;
using CreateSRV=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateUAV=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateCBV=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateHeap=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CopySimple=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
using CopyRanges=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
static CreateSRV srv=nullptr;static CreateUAV uav=nullptr;static CreateCBV cbv=nullptr;static CreateHeap heap=nullptr;
static CopySimple simple=nullptr;static CopyRanges ranges=nullptr;
static ComPtr<ID3D12Device> device;
static control_rr_native_clamp::Api providerApi;
static std::unique_ptr<GIASDescriptors::Index> index;
static std::atomic<bool> installed{false},failed{false};
static thread_local unsigned muted=0;
struct Mute {Mute(){++muted;}~Mute(){--muted;}Mute(const Mute&)=delete;};
using CopyObserver=void(*)(D3D12_CPU_DESCRIPTOR_HANDLE,UINT,D3D12_CPU_DESCRIPTOR_HANDLE) noexcept;
using ConstantObserver=void(*)(D3D12_CPU_DESCRIPTOR_HANDLE,const D3D12_CONSTANT_BUFFER_VIEW_DESC&) noexcept;
static std::atomic<CopyObserver> copyObserver{nullptr};static std::atomic<ConstantObserver> constantObserver{nullptr};
static bool Active(ID3D12Device* d)noexcept{return installed.load()&&!failed.load()&&!muted&&d==device.Get();}
static void Write(D3D12_CPU_DESCRIPTOR_HANDLE dst,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc=nullptr){
 GIASDescriptors::Acceleration as;gi_reuse::D3D12LiveSceneOwners::Identity allocation;
 if(desc&&desc->ViewDimension==D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE&&
    GINativeSceneOwners::owners.Identify(desc->RaytracingAccelerationStructure.Location,256,allocation))as={allocation.address,allocation.token,allocation.bytes};
 if(!index->Write(dst.ptr,as))failed=true;
}
static void STDMETHODCALLTYPE SRVHook(ID3D12Device* d,ID3D12Resource* r,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst){
 srv(d,r,desc,dst);const DWORD error=GetLastError();if(Active(d))try{Write(dst,desc);}catch(...){failed=true;}SetLastError(error);
}
static void STDMETHODCALLTYPE UAVHook(ID3D12Device* d,ID3D12Resource* r,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst){
 uav(d,r,counter,desc,dst);const DWORD error=GetLastError();if(Active(d))try{Write(dst);}catch(...){failed=true;}SetLastError(error);
}
static void STDMETHODCALLTYPE CBVHook(ID3D12Device* d,const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst){
 cbv(d,desc,dst);const DWORD error=GetLastError();if(Active(d))try{Write(dst);if(desc)if(const auto observe=constantObserver.load())observe(dst,*desc);}catch(...){failed=true;}SetLastError(error);
}
static void STDMETHODCALLTYPE SimpleHook(ID3D12Device* d,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE dst,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type){
 simple(d,count,dst,src,type);const DWORD error=GetLastError();
 if(Active(d)&&type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)try{if(!index->CopySimple(dst.ptr,src.ptr,count))failed=true;
  if(const auto observe=copyObserver.load())observe(dst,count,src);
 }catch(...){failed=true;}SetLastError(error);
}
static void STDMETHODCALLTYPE RangesHook(ID3D12Device* d,UINT dstCount,const D3D12_CPU_DESCRIPTOR_HANDLE* dst,const UINT* dstSizes,
 UINT srcCount,const D3D12_CPU_DESCRIPTOR_HANDLE* src,const UINT* srcSizes,D3D12_DESCRIPTOR_HEAP_TYPE type){
 ranges(d,dstCount,dst,dstSizes,srcCount,src,srcSizes,type);const DWORD error=GetLastError();
 if(Active(d)&&type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)try{
  std::vector<GIASDescriptors::Range> to,from;
  if(!GIASDescriptors::CopyRanges(dstCount,dst,dstSizes,to)||!GIASDescriptors::CopyRanges(srcCount,src,srcSizes,from)||!index->Copy(to,from))failed=true;
  // Mirror the same source spans without ever reading a shader-visible table.
  if(!failed.load())if(const auto observe=copyObserver.load()){
   const UINT step=d->GetDescriptorHandleIncrementSize(type);std::size_t a=0,b=0;UINT64 da=0,sb=0;
   while(a<to.size()&&b<from.size()){const auto n=(std::min)(to[a].count-da,from[b].count-sb);
    observe({SIZE_T(to[a].start+da*step)},static_cast<UINT>(n),{SIZE_T(from[b].start+sb*step)});
    da+=n;sb+=n;if(da==to[a].count){++a;da=0;}if(sb==from[b].count){++b;sb=0;}
   }
  }
 }catch(...){failed=true;}SetLastError(error);
}
static HRESULT STDMETHODCALLTYPE HeapHook(ID3D12Device* d,const D3D12_DESCRIPTOR_HEAP_DESC* desc,REFIID iid,void** output){
 const auto hr=heap(d,desc,iid,output);const DWORD error=GetLastError();
 if(Active(d)&&SUCCEEDED(hr)&&output&&*output&&desc&&desc->Type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)try{
  ComPtr<ID3D12DescriptorHeap> value;if(FAILED(static_cast<IUnknown*>(*output)->QueryInterface(IID_PPV_ARGS(&value)))||
   !index->Invalidate({value->GetCPUDescriptorHandleForHeapStart().ptr,desc->NumDescriptors}))failed=true;
 }catch(...){failed=true;}SetLastError(error);return hr;
}
static bool NativeHandle(void* wrapper,UINT64& out) noexcept {
 __try {if(!wrapper)return false;out=*reinterpret_cast<UINT64*>(static_cast<unsigned char*>(wrapper)+0x10);return out!=0;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void CreatedAS(void* wrapper) noexcept {
 if(!installed.load()||failed.load())return;
 try{UINT64 handle=0;GINativeSceneOwners::Wrapper resource;
  if(!NativeHandle(wrapper,handle)||!GINativeSceneOwners::Read(wrapper,resource))return;
  D3D12_SHADER_RESOURCE_VIEW_DESC desc{};desc.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;desc.RaytracingAccelerationStructure.Location=resource.address;
  Write({SIZE_T(handle)},&desc);
 }catch(...){failed=true;}
}
static bool Select(const PTGlassRuntime::Context& context,UINT srvSlot,gi_reuse::D3D12LiveSceneOwners::Identity& output) noexcept {
 output={};if(!installed.load()||failed.load()||!context.table.heap||srvSlot>=36)return false;
 try {const auto d=context.table.heap->GetDesc();const UINT64 at=UINT64(context.table.first)+5+srvSlot;
  if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||at>=d.NumDescriptors)return false;
  const auto step=device->GetDescriptorHandleIncrementSize(d.Type);const auto base=context.table.heap->GetCPUDescriptorHandleForHeapStart().ptr;
  if(at>(UINT64_MAX-base)/step)return false;
  const auto selected=index->Read(base+at*step);gi_reuse::D3D12LiveSceneOwners::Identity current;
  if(!selected.token||!GINativeSceneOwners::owners.Identify(selected.address,256,current)||current.token!=selected.token)return false;
  output=current;return true;
 }catch(...){return false;}
}
// Read native thread-local provider values through the already verified API.
// Range comes from the ray producer's provider, not GI6's different cb4 layout.
static bool Provider(const char* name,UINT& output) noexcept {
 output=0;__try {
  if(!providerApi.ready||!providerApi.resolve)return false;
  struct Slot {int id,tag;} slot{-1,0};providerApi.resolve(&slot,name);
  return providerApi.ReadCurrent(slot.id,output);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool Inputs(GINativeSceneOwners::Writer& writer) noexcept {
 writer.scene={};writer.baseRange=0;writer.rays=writer.pass=writer.passes=0;
 UINT tag=0,bytes=sizeof(tag),range=0;PTGlassRuntime::Context context;
 const auto fail=[&](const char* reason){writer.inputReason=reason;return false;};
 if(!writer.pipeline)return fail("writer_pipeline");
 if(FAILED(writer.pipeline->GetPrivateData(gi_reuse::nativeGIVariantKey,&bytes,&tag))||bytes!=sizeof(tag)||tag<1||tag>2)return fail("variant_tag");
 if(!PTGlassRuntime::ReadContext(&context))return fail("ray_context");
 if(reinterpret_cast<UINT_PTR>(context.rays)!=writer.ticket.list)return fail("ray_list_identity");
 if(!Select(context,tag==1?4u:5u,writer.scene))return fail("tlas_descriptor");
 if(!Provider("g_fDiffuseGIRadius",range))return fail("radius_provider");
 if(!Provider("g_uRTDiffuseRayCount",writer.rays))return fail("ray_count_provider");
 if(!Provider("g_uDGIPass",writer.pass))return fail("pass_provider");
 if(!Provider("g_uDGIPassCount",writer.passes))return fail("pass_count_provider");
 std::memcpy(&writer.baseRange,&range,4);
 if(!(std::isfinite(writer.baseRange)&&writer.baseRange>0&&writer.rays>0&&writer.rays<=64&&
  writer.passes>0&&writer.pass<writer.passes&&((writer.pass==0&&tag==1)||(writer.pass>0&&tag==2))))return fail("provider_values");
 writer.inputReason="none";return true;
}
static bool Install(ID3D12Device* d) noexcept {
 if(!GINativeSceneOwners::Enabled()||!d)return false;if(installed.load())return d==device.Get()&&!failed.load();
 try{if(!providerApi.Initialize(verifiedRenderer,verifiedD3d))return false;
  device=d;index=std::make_unique<GIASDescriptors::Index>(d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  const bool ok=PTGlassRuntime::Patch(d,18,reinterpret_cast<void*>(&SRVHook),reinterpret_cast<void**>(&srv))&&
   PTGlassRuntime::Patch(d,19,reinterpret_cast<void*>(&UAVHook),reinterpret_cast<void**>(&uav))&&
   PTGlassRuntime::Patch(d,17,reinterpret_cast<void*>(&CBVHook),reinterpret_cast<void**>(&cbv))&&
   PTGlassRuntime::Patch(d,14,reinterpret_cast<void*>(&HeapHook),reinterpret_cast<void**>(&heap))&&
   PTGlassRuntime::Patch(d,23,reinterpret_cast<void*>(&RangesHook),reinterpret_cast<void**>(&ranges))&&
   PTGlassRuntime::Patch(d,24,reinterpret_cast<void*>(&SimpleHook),reinterpret_cast<void**>(&simple));
  if(!ok){failed=true;return false;}
  GINativeSceneOwners::liveASDescriptor=&CreatedAS;GINativeSceneOwners::writerInputs=&Inputs;installed=true;return true;
 }catch(...){failed=true;return false;}
}
}
