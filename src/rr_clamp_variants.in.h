#pragma once
#include <cstddef>
#include "rr_clamp_60.h"
inline constexpr std::size_t kRRClampNativeSize=6160;
struct RRClampShaderBytes {const void* data;std::size_t size;};
inline const RRClampShaderBytes kRRClampShaders[]={
{kRRClampShader60,sizeof(kRRClampShader60)},
};
