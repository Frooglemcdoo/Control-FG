#pragma once
#include <d3d12.h>
namespace gi_reuse {
inline D3D12_STATIC_SAMPLER_DESC GIDiagnosticSampler() noexcept {
 D3D12_STATIC_SAMPLER_DESC s{};s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
 s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
 s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
 s.MinLOD=0;s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderRegister=0;s.RegisterSpace=19;
 s.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;return s;
}
}
