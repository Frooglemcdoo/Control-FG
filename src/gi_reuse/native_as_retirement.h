#pragma once
#include <windows.h>
#include <excpt.h>
#include "d3d12_as_owners.h"
namespace gi_reuse {
enum class NativeASRetirement { Untracked, Retired, Stale, InvalidWrapper };
inline bool ReadNativeReleaseResource(void* wrapper,ID3D12Resource*& resource) noexcept {
 resource=nullptr;
 __try {if(!wrapper)return false;resource=*static_cast<ID3D12Resource**>(wrapper);return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// The hash-verified native function first AddRefs/enqueues its resource, then
// Releases the wrapper reference and clears the wrapper. Forward it exactly once.
// Never infer GPU completion, suppress native release, or read the wrapper after
// forwarding. The registry drops only its own diagnostic reference; native queue
// ownership and existing journal/consumer leases remain independent.
template<class Registry,class Forward>
NativeASRetirement ForwardNativeASRelease(Registry& registry,void* wrapper,Forward forward) {
 const DWORD before=GetLastError();ID3D12Resource* resource=nullptr;
 const bool readable=ReadNativeReleaseResource(wrapper,resource);
 typename Registry::Lease held,retired;
 if(readable)registry.AcquirePointer(resource,held);
 SetLastError(before);forward(wrapper);const DWORD after=GetLastError();
 NativeASRetirement result=readable?NativeASRetirement::Untracked:NativeASRetirement::InvalidWrapper;
 if(held.resource)result=registry.Retire(held.resource.Get(),held.token,retired)?NativeASRetirement::Retired:NativeASRetirement::Stale;
 held={};retired={};SetLastError(after);return result;
}
}
