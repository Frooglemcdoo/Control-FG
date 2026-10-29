#pragma once
#include <cstdint>
namespace control_rr_gi_spatial {
enum class Result { Failed, NativeInitialization, Cleared };
// A missing history is allocated and cleared by the native filter itself.
// An existing history is cleared in place; ownership and dimensions stay native.
template<class Api> Result Prepare(Api& api,std::uintptr_t slot,std::uintptr_t expected) {
 if(!slot||slot!=expected)return Result::Failed;
 std::uintptr_t container=0;
 if(!api.Read(slot,container))return Result::Failed;
 if(!container)return Result::NativeInitialization;
 const auto texture=api.Texture(container);
 if(!texture||!api.Clear(texture))return Result::Failed;
 return Result::Cleared;
}
}
