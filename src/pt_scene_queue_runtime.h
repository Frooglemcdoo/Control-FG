#pragma once
#include "pt_scene_capture_runtime.h"
#include <atomic>
// Observes the exact interfaces supplied by Control. Install on the submitted
// queue (including a proxy), never a different QueryInterface/native substitute.
namespace PTSceneQueues {
using Execute=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*);
using FenceOperation=HRESULT(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,ID3D12Fence*,UINT64);
using Reset=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
struct QueueMethods {Execute execute=nullptr;FenceOperation signal=nullptr,wait=nullptr;};
inline std::mutex hooksMutex;
inline std::recursive_mutex queueMutex;
inline std::map<void**,QueueMethods> queues;
inline std::map<void**,Reset> lists;
inline std::atomic<PTSceneLive::Capture*> observer{nullptr};
inline bool hooksValid=true;
inline thread_local unsigned nesting=0;
struct Scope {bool outer;Scope():outer(nesting++==0){}~Scope(){--nesting;}};
inline void STDMETHODCALLTYPE ExecuteHook(ID3D12CommandQueue* q,UINT n,ID3D12CommandList*const* input){
    Execute original=nullptr;{std::lock_guard<std::mutex> guard(hooksMutex);original=queues.at(*reinterpret_cast<void***>(q)).execute;}
    Scope scope;const auto capture=observer.load(std::memory_order_acquire);
    if(!scope.outer||!capture){original(q,n,input);return;}
    // Keep the observation order identical to calls reaching the native API.
    std::lock_guard<std::recursive_mutex> guard(queueMutex);
    const DWORD before=GetLastError();
    capture->Submit(q,n,input);SetLastError(before);original(q,n,input);
    const DWORD after=GetLastError();capture->Submitted(q);SetLastError(after);
}
inline HRESULT FenceHook(ID3D12CommandQueue* q,ID3D12Fence* fence,UINT64 value,bool signal){
    FenceOperation original=nullptr;{std::lock_guard<std::mutex> guard(hooksMutex);const auto& methods=queues.at(*reinterpret_cast<void***>(q));original=signal?methods.signal:methods.wait;}
    Scope scope;const auto capture=observer.load(std::memory_order_acquire);
    if(!scope.outer||!capture)return original(q,fence,value);
    std::lock_guard<std::recursive_mutex> guard(queueMutex);
    const HRESULT hr=original(q,fence,value);const DWORD error=GetLastError();
    if(FAILED(hr))capture->Invalidate();
    else if(signal)capture->Signal(q,fence,value);else capture->Wait(q,fence,value);
    SetLastError(error);return hr;
}
inline HRESULT STDMETHODCALLTYPE SignalHook(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 n){return FenceHook(q,f,n,true);}
inline HRESULT STDMETHODCALLTYPE WaitHook(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 n){return FenceHook(q,f,n,false);}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso){
    Reset original=nullptr;{std::lock_guard<std::mutex> guard(hooksMutex);original=lists.at(*reinterpret_cast<void***>(list));}
    Scope scope;const auto capture=observer.load(std::memory_order_acquire);
    const HRESULT hr=original(list,allocator,pso);const DWORD error=GetLastError();
    if(scope.outer&&capture&&SUCCEEDED(hr))capture->Reset(list);
    SetLastError(error);return hr;
}
inline bool Exchange(void** entry,void* expected,void* replacement){
    if(*entry!=expected)return false;
    DWORD previous=0;if(!VirtualProtect(entry,sizeof(void*),PAGE_READWRITE,&previous))return false;
    const bool exchanged=InterlockedCompareExchangePointer(reinterpret_cast<void*volatile*>(entry),replacement,expected)==expected;
    DWORD unused=0;const bool restored=VirtualProtect(entry,sizeof(void*),previous,&unused)!=FALSE;
    return exchanged&&restored;
}
inline bool WatchQueue(ID3D12CommandQueue* queue){
    if(!queue)return false;std::lock_guard<std::mutex> guard(hooksMutex);
    auto** table=*reinterpret_cast<void***>(queue);if(!hooksValid)return false;if(queues.count(table))return true;
    if(queues.size()>=16)return false;
    const QueueMethods original{reinterpret_cast<Execute>(table[10]),reinterpret_cast<FenceOperation>(table[14]),reinterpret_cast<FenceOperation>(table[15])};
    queues.emplace(table,original);
    const bool a=Exchange(table+10,reinterpret_cast<void*>(original.execute),reinterpret_cast<void*>(&ExecuteHook));
    const bool b=a&&Exchange(table+14,reinterpret_cast<void*>(original.signal),reinterpret_cast<void*>(&SignalHook));
    const bool c=b&&Exchange(table+15,reinterpret_cast<void*>(original.wait),reinterpret_cast<void*>(&WaitHook));
    // Keep forwarding records even on partial failure: another thread can
    // already be inside a newly installed callback. Disable scene admission.
    if(!c){hooksValid=false;if(auto* capture=observer.load())capture->Invalidate();return false;}return true;
}
inline bool WatchListInterface(ID3D12GraphicsCommandList* list){
    if(!list)return false;std::lock_guard<std::mutex> guard(hooksMutex);
    auto** table=*reinterpret_cast<void***>(list);if(!hooksValid)return false;if(lists.count(table))return true;
    if(lists.size()>=32)return false;
    const auto original=reinterpret_cast<Reset>(table[10]);lists.emplace(table,original);
    if(!Exchange(table+10,reinterpret_cast<void*>(original),reinterpret_cast<void*>(&ResetHook))){hooksValid=false;if(auto* capture=observer.load())capture->Invalidate();return false;}return true;
}
inline bool WatchList(ID3D12GraphicsCommandList* list){
    if(!WatchListInterface(list))return false;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> base;
    if(FAILED(list->QueryInterface(IID_PPV_ARGS(&base))))return false;
    return WatchListInterface(base.Get());
}
inline bool Start(PTSceneLive::Capture* capture){
    PTSceneLive::Capture* empty=nullptr;return capture&&(observer.compare_exchange_strong(empty,capture,std::memory_order_release)||empty==capture);
}
}
