#pragma once
#include <atomic>
#include <d3d12.h>
// Optional live observer boundary. Native-only tools and builds have no observer.
// Publish a complete immutable callback set before any queue is intercepted.
namespace GINativeSubmission {
struct Callbacks {
 UINT64 (*before)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*) noexcept;
 void (*after)(ID3D12CommandQueue*,UINT64) noexcept;
 bool (*beforeFence)() noexcept;
 void (*afterFence)(ID3D12CommandQueue*,ID3D12Fence*,UINT64,bool,HRESULT,bool) noexcept;
 void (*cpuSignal)(ID3D12Fence*) noexcept;
};
inline std::atomic<const Callbacks*> observer{nullptr};
inline UINT64 Before(ID3D12CommandQueue* q,UINT n,ID3D12CommandList*const* lists) noexcept {
 const auto o=observer.load();return o?o->before(q,n,lists):0;
}
inline void After(ID3D12CommandQueue* q,UINT64 token) noexcept {if(const auto o=observer.load())o->after(q,token);}
inline bool BeforeFence() noexcept {const auto o=observer.load();return o&&o->beforeFence();}
inline void AfterFence(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 v,bool s,HRESULT hr,bool locked) noexcept {
 if(const auto o=observer.load())o->afterFence(q,f,v,s,hr,locked);
}
inline void CpuSignal(ID3D12Fence* f) noexcept {if(const auto o=observer.load())o->cpuSignal(f);}
}
