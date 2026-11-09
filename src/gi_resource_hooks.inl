#pragma once
// Included inside native_lighting; uses its vtable patch helper. Metadata only.
namespace gi_resource_hooks {
using Committed=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using Placed=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Heap*,UINT64,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using Reserved=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using Queue=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_COMMAND_QUEUE_DESC*,REFIID,void**);
using QueueObserver=void (*)(ID3D12CommandQueue*) noexcept;
inline std::atomic<QueueObserver> queueObserver{nullptr};
inline std::atomic<bool> queueInstalled{false};
struct Hooks {void** table=nullptr;Committed committed=nullptr;Placed placed=nullptr;Reserved reserved=nullptr;Queue queue=nullptr;};
inline Hooks hooks[8]{};inline SRWLOCK lock=SRWLOCK_INIT;
inline Hooks Lookup(ID3D12Device* device) noexcept {
 const auto table=*reinterpret_cast<void***>(device);Hooks found{};AcquireSRWLockShared(&lock);
 for(const auto& h:hooks)if(h.table==table){found=h;break;}ReleaseSRWLockShared(&lock);return found;
}
inline void Observe(HRESULT hr,void** out,D3D12_RESOURCE_STATES state,bool supportedASStorage=false,ID3D12Heap* heap=nullptr) noexcept {if(SUCCEEDED(hr)&&out&&*out)gi_reuse::runtime::ObserveCreated(static_cast<IUnknown*>(*out),state,supportedASStorage,heap);}
inline HRESULT STDMETHODCALLTYPE CommittedHook(ID3D12Device* d,const D3D12_HEAP_PROPERTIES* h,D3D12_HEAP_FLAGS f,const D3D12_RESOURCE_DESC* r,D3D12_RESOURCE_STATES s,const D3D12_CLEAR_VALUE* c,REFIID iid,void** out){
 const auto hr=Lookup(d).committed(d,h,f,r,s,c,iid,out);Observe(hr,out,s,true);return hr;
}
inline HRESULT STDMETHODCALLTYPE PlacedHook(ID3D12Device* d,ID3D12Heap* h,UINT64 offset,const D3D12_RESOURCE_DESC* r,D3D12_RESOURCE_STATES s,const D3D12_CLEAR_VALUE* c,REFIID iid,void** out){
 const auto hr=Lookup(d).placed(d,h,offset,r,s,c,iid,out);Observe(hr,out,s,true,h);return hr;
}
inline HRESULT STDMETHODCALLTYPE ReservedHook(ID3D12Device* d,const D3D12_RESOURCE_DESC* r,D3D12_RESOURCE_STATES s,const D3D12_CLEAR_VALUE* c,REFIID iid,void** out){
 const auto hr=Lookup(d).reserved(d,r,s,c,iid,out);Observe(hr,out,s);return hr;
}
inline HRESULT STDMETHODCALLTYPE QueueHook(ID3D12Device* d,const D3D12_COMMAND_QUEUE_DESC* desc,REFIID iid,void** out){
 const auto hr=Lookup(d).queue(d,desc,iid,out);
 if(SUCCEEDED(hr)&&out&&*out){Microsoft::WRL::ComPtr<ID3D12CommandQueue> q;
  if(SUCCEEDED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&q)))){if(const auto observe=queueObserver.load())observe(q.Get());}
  else gi_reuse::runtime::Fail("queue_interface_coverage");
 }return hr;
}
inline void Install(ID3D12Device* device) noexcept {
 if(!device||!gi_reuse::runtime::Enabled())return;auto table=*reinterpret_cast<void***>(device);AcquireSRWLockExclusive(&lock);
 Hooks* slot=nullptr;for(auto& h:hooks){if(h.table==table){ReleaseSRWLockExclusive(&lock);return;}if(!h.table&&!slot)slot=&h;}
 if(!slot){gi_reuse::runtime::Fail("resource_hook_capacity");ReleaseSRWLockExclusive(&lock);return;}
 *slot={table,reinterpret_cast<Committed>(table[27]),reinterpret_cast<Placed>(table[29]),reinterpret_cast<Reserved>(table[30]),reinterpret_cast<Queue>(table[8])};
 const bool a=PatchSlot(&table[27],reinterpret_cast<void*>(slot->committed),reinterpret_cast<void*>(&CommittedHook));
 const bool b=PatchSlot(&table[29],reinterpret_cast<void*>(slot->placed),reinterpret_cast<void*>(&PlacedHook));
 const bool c=PatchSlot(&table[30],reinterpret_cast<void*>(slot->reserved),reinterpret_cast<void*>(&ReservedHook));
 const bool q=PatchSlot(&table[8],reinterpret_cast<void*>(slot->queue),reinterpret_cast<void*>(&QueueHook));
 if(!a||!b||!c||!q)gi_reuse::runtime::Fail("resource_hook_coverage");
 if(q)queueInstalled=true;
 ReleaseSRWLockExclusive(&lock);
}
} // namespace gi_resource_hooks
