#pragma once
#include "fg_native_source_policy.h"
static bool FGNativeSourceRead(std::uintptr_t address,void* output,std::size_t size) noexcept {
    SIZE_T copied=0;
    return address&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,size,&copied)&&copied==size;
}
static control_fg_native_source::Result FGNativeSourceSelect(const void* wrapper,
    ID3D12Resource* const* shadows,UINT count,UINT fallback) noexcept {
    // verifiedD3d is assigned only after the existing exact module hash checks.
    // Read while inside native Present, after its flush and before it advances
    // the private render-target index. Never write game state or use an unmatched pointer.
    const DWORD savedError=GetLastError();
    std::uintptr_t identities[2]{};
    if(count==2&&shadows){identities[0]=reinterpret_cast<std::uintptr_t>(shadows[0]);identities[1]=reinterpret_cast<std::uintptr_t>(shadows[1]);}
    const auto address=verifiedD3d?reinterpret_cast<std::uintptr_t>(verifiedD3d)+kNativeDevicePointerRva:0;
    const auto result=control_fg_native_source::Select(address,reinterpret_cast<std::uintptr_t>(wrapper),identities,count,fallback,FGNativeSourceRead);
    SetLastError(savedError);return result;
}
