#pragma once
#include <cstddef>
#include "rr_clamp_fixed.h"
#include "../src/rr_clamp_strength_policy.h"
static_assert(kRRClampCompiledPercent==control_rr_clamp::Requested(),"Compiled clamp does not match runtime policy");
inline constexpr std::size_t kRRClampNativeSize=6160;
struct RRClampShaderBytes {const void* data;std::size_t size;};
inline const RRClampShaderBytes kRRClampShaders[]={
{kRRClampShaderFixed,sizeof(kRRClampShaderFixed)},
};
