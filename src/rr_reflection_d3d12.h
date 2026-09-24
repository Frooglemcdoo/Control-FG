#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "rr_reflection_backend.h"

namespace control_rr_reflection {
// Windows SEH leaves contain no C++ objects requiring unwinding.
// Caller owns native tracker/recording locks for the entire Record call.
struct D3D12Api {
 void* user=nullptr;
 bool (*current)(void*,Context&)=nullptr;
 // Native command context + 8; caller supplies this only after exact TLS checks.
 std::uint64_t* commandCount=nullptr;
 HRESULT lastCreateResult=S_OK;
 DWORD lastCreateException=0;
 // Resolved from the real system DXGI module by the owner, never our proxy.
 using CreateFactory=HRESULT (WINAPI*)(REFIID,void**);
 CreateFactory createFactory=nullptr;
 HRESULT lastBudgetResult=S_OK;
 DWORD lastBudgetException=0;
 bool QueryBudget(Address device,control_rr::MemoryBudget& result) noexcept {
  result={};lastBudgetResult=E_NOINTERFACE;lastBudgetException=0;
  IDXGIFactory4* factory=nullptr;IDXGIAdapter3* adapter=nullptr;
  bool good=false;
  // Only raw COM pointers/PODs here: MSVC SEH must not need C++ unwinding.
  __try {
   if(device&&createFactory){
    lastBudgetResult=createFactory(__uuidof(IDXGIFactory4),reinterpret_cast<void**>(&factory));
    if(SUCCEEDED(lastBudgetResult)&&factory){
     const auto luid=reinterpret_cast<ID3D12Device*>(device)->GetAdapterLuid();
     lastBudgetResult=factory->EnumAdapterByLuid(luid,IID_PPV_ARGS(&adapter));
     if(SUCCEEDED(lastBudgetResult)&&adapter){
      DXGI_QUERY_VIDEO_MEMORY_INFO info{};
      lastBudgetResult=adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info);
      if(SUCCEEDED(lastBudgetResult)){result={info.Budget,info.CurrentUsage};good=true;}
     }
    }
   }
  } __except(EXCEPTION_EXECUTE_HANDLER){lastBudgetException=GetExceptionCode();good=false;}
  __try {if(adapter)adapter->Release();}
  __except(EXCEPTION_EXECUTE_HANDLER){lastBudgetException=GetExceptionCode();good=false;}
  __try {if(factory)factory->Release();}
  __except(EXCEPTION_EXECUTE_HANDLER){lastBudgetException=GetExceptionCode();good=false;}
  if(!good)result={};
  return good;
 }
 static D3D12_RESOURCE_DESC Descriptor(const Shape& s) noexcept {
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  d.Width=s.width;d.Height=s.height;d.DepthOrArraySize=static_cast<UINT16>(s.layers);
  d.MipLevels=1;d.Format=static_cast<DXGI_FORMAT>(s.format);d.SampleDesc.Count=1;
  // Private copies need no RT/UAV flags; source flags must not leak into them.
  d.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;d.Flags=D3D12_RESOURCE_FLAG_NONE;return d;
 }
 bool Current(const Context& expected) noexcept {
  __try {Context actual{};return current&&commandCount&&current(user,actual)&&Same(expected,actual);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool AllocationSize(Address device,const Shape& shape,std::uint64_t& bytes) noexcept {
  bytes=0;
  __try {auto d=Descriptor(shape);auto info=reinterpret_cast<ID3D12Device*>(device)->GetResourceAllocationInfo(0,1,&d);
   if(!info.SizeInBytes||info.SizeInBytes==UINT64_MAX||!info.Alignment)return false;
   bytes=info.SizeInBytes;return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool Create(Address device,const Shape& shape,Address& result) noexcept {
  result=0;lastCreateResult=S_OK;lastCreateException=0;
  __try {auto d=Descriptor(shape);D3D12_HEAP_PROPERTIES hp{};
   hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
   ID3D12Resource* resource=nullptr;
   lastCreateResult=reinterpret_cast<ID3D12Device*>(device)->CreateCommittedResource(
    &hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&resource));
   result=reinterpret_cast<Address>(resource);return SUCCEEDED(lastCreateResult)&&resource;
  } __except(EXCEPTION_EXECUTE_HANDLER){lastCreateException=GetExceptionCode();return false;}
 }
 bool Hold(Address resource) noexcept {
  __try {reinterpret_cast<ID3D12Resource*>(resource)->AddRef();return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 void Release(Address resource) noexcept {
  __try {reinterpret_cast<ID3D12Resource*>(resource)->Release();}
  __except(EXCEPTION_EXECUTE_HANDLER){}
 }
 bool Barriers(const Context& context,const Transition* transitions,std::size_t count) noexcept {
  __try {
   if(!Current(context)||count!=4||*commandCount==UINT64_MAX)return false;
   D3D12_RESOURCE_BARRIER barriers[4]{};
   for(unsigned i=0;i<4;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource=reinterpret_cast<ID3D12Resource*>(transitions[i].resource);
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore=static_cast<D3D12_RESOURCE_STATES>(transitions[i].before);
    b.Transition.StateAfter=static_cast<D3D12_RESOURCE_STATES>(transitions[i].after);
   }
   ++*commandCount;reinterpret_cast<ID3D12GraphicsCommandList*>(context.list)->ResourceBarrier(4,barriers);return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool Copy(const Context& context,Address destination,Address source) noexcept {
  __try {
   if(!Current(context)||*commandCount==UINT64_MAX)return false;
   ++*commandCount;reinterpret_cast<ID3D12GraphicsCommandList*>(context.list)->CopyResource(
    reinterpret_cast<ID3D12Resource*>(destination),reinterpret_cast<ID3D12Resource*>(source));return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool Signal(Address queue,Address fence,std::uint64_t value) noexcept {
  __try {return SUCCEEDED(reinterpret_cast<ID3D12CommandQueue*>(queue)->Signal(reinterpret_cast<ID3D12Fence*>(fence),value));}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 std::uint64_t Completed(Address fence) noexcept {
  __try {return reinterpret_cast<ID3D12Fence*>(fence)->GetCompletedValue();}
  __except(EXCEPTION_EXECUTE_HANDLER){return UINT64_MAX;}
 }
};
using D3D12Backend=Backend<D3D12Api>;
}
