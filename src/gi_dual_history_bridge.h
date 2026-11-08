#pragma once
#include <atomic>
#include <d3d12.h>
// Only exact GI binds and command-list lifetime events enter this adapter.
namespace gi_dual_bridge {
using Dispatch=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
struct Callbacks {
 void (*begin)(UINT64,UINT64) noexcept;
 void (*end)() noexcept;
 void (*copy)(D3D12_CPU_DESCRIPTOR_HANDLE,UINT,D3D12_CPU_DESCRIPTOR_HANDLE) noexcept;
 void (*constant)(D3D12_CPU_DESCRIPTOR_HANDLE,const D3D12_CONSTANT_BUFFER_VIEW_DESC&) noexcept;
 bool (*dispatch)(ID3D12GraphicsCommandList*,ID3D12PipelineState*,ID3D12PipelineState*,UINT,UINT,UINT,Dispatch) noexcept;
 void (*reset)(ID3D12GraphicsCommandList*) noexcept;
 UINT64 (*before)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*) noexcept;
 void (*after)(ID3D12CommandQueue*,UINT64) noexcept;
};
inline std::atomic<const Callbacks*> callbacks{nullptr};
inline thread_local bool observing=false;
inline std::atomic<bool(*)(ID3D12GraphicsCommandList*) noexcept> resetReady{nullptr};
}
