#pragma once
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <cstdint>
#include <new>
#include "rr_clamp_strength_policy.h"
#include "../build/rr_clamp_variants.h"
// Diagnostic CS3: immutable PSO clones; original creation result is never replaced.
// Only the validated RR temporal-dispatch scope may select a clone. Descriptor
// heaps, roots and resources stay native. Restore the exact observed native PSO.
namespace control_rr_clamp {
static_assert(sizeof(kRRClampShaders)/sizeof(kRRClampShaders[0])==VariantCount,"shader table count");
inline void Select(unsigned v) noexcept {if(Valid(v))requested.store(v,std::memory_order_release);}
using CreateFn=HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC*,REFIID,void**);
using SetFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using DispatchFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
inline CreateFn createOriginal=nullptr;
inline SetFn setOriginal=nullptr;
inline DispatchFn dispatchOriginal=nullptr;
inline void** deviceTable=nullptr;inline void** listTable=nullptr;
inline SRWLOCK installLock=SRWLOCK_INIT;
inline ID3D12Device* hostDevice=nullptr; // held for the bounded process-lifetime cache
struct Bundle {ID3D12PipelineState* native=nullptr;ID3D12PipelineState* variants[VariantCount]{};};
inline std::atomic<Bundle*> bundles[8]{};
inline std::atomic<unsigned> bundleCount{0};
struct Request {ID3D12GraphicsCommandList* list=nullptr;ID3D12PipelineState* observed=nullptr;unsigned strength=100,count=0;bool used=false;};
inline thread_local Request* active=nullptr;
inline bool Exchange(void** slot,void* expected,void* replacement) noexcept {
 DWORD protection=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection))return false;
 const bool okay=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
 DWORD ignored=0;const bool restored=VirtualProtect(slot,sizeof(void*),protection,&ignored)!=FALSE;
 if(!restored)Log("RR_CLAMP_CS3_PROTECTION_RESTORE_FAILED slot=%p",slot);
 return okay;
}
inline std::uint32_t CRC(const void* data,std::size_t size) noexcept {
 std::uint32_t crc=0xffffffffu;auto* p=static_cast<const unsigned char*>(data);
 for(std::size_t i=0;i<size;++i){crc^=p[i];for(unsigned j=0;j<8;++j)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
 return crc^0xffffffffu;
}
inline bool Target(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) noexcept {
 __try {return desc&&desc->pRootSignature&&desc->CS.pShaderBytecode&&desc->CS.BytecodeLength==kRRClampNativeSize&&CRC(desc->CS.pShaderBytecode,desc->CS.BytecodeLength)==0x600347e7u;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void Capture(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,void* result) noexcept {
 if(device!=hostDevice||!result||!Target(desc))return;
 const unsigned index=bundleCount.fetch_add(1,std::memory_order_acq_rel);
 if(index>=8){Log("RR_CLAMP_CS3_CACHE_FULL native_fallback=1");return;}
 auto* b=new(std::nothrow) Bundle{};if(!b)return;
 auto* unknown=static_cast<IUnknown*>(result);
 if(FAILED(unknown->QueryInterface(IID_PPV_ARGS(&b->native)))||!b->native){delete b;return;}
 for(unsigned i=0;i<VariantCount;++i){
  auto copy=*desc;copy.CachedPSO={};copy.CS={kRRClampShaders[i].data,kRRClampShaders[i].size};
  const HRESULT hr=createOriginal(device,&copy,IID_PPV_ARGS(&b->variants[i]));
  if(FAILED(hr))Log("RR_CLAMP_CS3_VARIANT_FAILED index=%u hr=0x%08lX",i,static_cast<unsigned long>(hr));
 }
 bundles[index].store(b,std::memory_order_release);
 unsigned ready=0;for(auto* pipeline:b->variants)if(pipeline)++ready;
 Log("RR_CLAMP_CS3_CAPTURE native=%p variants_ready=%u total=%u root_preserved=1 cached_blob_cleared=1",b->native,ready,VariantCount);
}
inline HRESULT STDMETHODCALLTYPE Create(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,REFIID iid,void** result) {
 const HRESULT hr=createOriginal(device,desc,iid,result);const DWORD error=GetLastError();
 if(SUCCEEDED(hr)&&result&&*result)Capture(device,desc,*result);
 SetLastError(error);return hr;
}
inline bool InstallDevice(IUnknown* unknown) noexcept {
 ID3D12Device* device=nullptr;if(!unknown||FAILED(unknown->QueryInterface(IID_PPV_ARGS(&device)))||!device)return false;
 AcquireSRWLockExclusive(&installLock);bool okay=false;auto** table=*reinterpret_cast<void***>(device);
 if(deviceTable){okay=deviceTable==table&&hostDevice==device;}
 else {
  hostDevice=device;createOriginal=reinterpret_cast<CreateFn>(table[11]);
  okay=Exchange(&table[11],reinterpret_cast<void*>(createOriginal),reinterpret_cast<void*>(&Create));
  if(okay){deviceTable=table;device->AddRef();}else {hostDevice=nullptr;createOriginal=nullptr;}
 }
 ReleaseSRWLockExclusive(&installLock);device->Release();
 Log("RR_CLAMP_CS3_DEVICE_HOOK ready=%u",unsigned(okay));return okay;
}
inline Bundle* Find(ID3D12PipelineState* native) noexcept {
 if(!native)return nullptr;for(auto& slot:bundles){auto* b=slot.load(std::memory_order_acquire);if(b&&b->native==native)return b;}return nullptr;
}
inline void STDMETHODCALLTYPE Set(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pipeline) {
 setOriginal(list,pipeline);if(active&&active->list==list)active->observed=pipeline;
}
inline void STDMETHODCALLTYPE Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) {
 auto* request=active;
 if(!request||request->list!=list){dispatchOriginal(list,x,y,z);return;}
 ++request->count;auto* b=Find(request->observed);const int variant=Variant(request->strength);
 auto* replacement=(b&&variant>=0)?b->variants[variant]:nullptr;
 if(!Admit(true,true,b!=nullptr,replacement!=nullptr,request->count)){dispatchOriginal(list,x,y,z);return;}
 // This runs after the native DeviceUtil has committed its root/descriptors/PSO.
 // Never swap before DeviceUtil, which could overwrite the chosen PSO.
 __try {
  setOriginal(list,replacement);dispatchOriginal(list,x,y,z);request->used=true;
 } __finally {setOriginal(list,request->observed);}
}
inline bool InstallList(ID3D12GraphicsCommandList* list) noexcept {
 if(!list)return false;AcquireSRWLockExclusive(&installLock);auto** table=*reinterpret_cast<void***>(list);bool okay=false;
 if(listTable)okay=listTable==table&&table[14]==reinterpret_cast<void*>(&Dispatch)&&table[25]==reinterpret_cast<void*>(&Set);
 else {
  setOriginal=reinterpret_cast<SetFn>(table[25]);dispatchOriginal=reinterpret_cast<DispatchFn>(table[14]);
  if(Exchange(&table[25],reinterpret_cast<void*>(setOriginal),reinterpret_cast<void*>(&Set))){
   if(Exchange(&table[14],reinterpret_cast<void*>(dispatchOriginal),reinterpret_cast<void*>(&Dispatch))){listTable=table;okay=true;}
   else {(void)Exchange(&table[25],reinterpret_cast<void*>(&Set),reinterpret_cast<void*>(setOriginal));listTable=table;}
  }
 }
 ReleaseSRWLockExclusive(&installLock);return okay;
}
// Internal fallback=100 and RR OFF never substitute shaders. A missed creation path or
// unobserved native PSO gives an explicit native fallback, not a guessed binding.
using NativeDispatchFn=void (*)(void*,const void*);
inline bool Invoke(NativeDispatchFn fn,void* state,const void* groups,ID3D12GraphicsCommandList* list,unsigned strength) {
 Request request{};request.list=list;request.strength=strength;auto* previous=active;
 const DWORD incoming=GetLastError();DWORD nativeError=incoming;
 const bool armed=Variant(strength)>=0&&InstallList(list);
 __try {if(armed)active=&request;SetLastError(incoming);fn(state,groups);nativeError=GetLastError();}
 __finally {active=previous;}
 const bool changed=armed&&request.used&&request.count==1;
 effective.store(changed?strength:100,std::memory_order_release);applied.store(strength==100||changed,std::memory_order_release);
 SetLastError(nativeError);return !armed||request.count<=1;
}
}
