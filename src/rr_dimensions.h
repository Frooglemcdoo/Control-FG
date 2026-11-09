#pragma once
#include <cstdint>
namespace control_rr {
// Preserve the existing per-axis bound while admitting full 8K render inputs.
// Memory admission is separate from shape/overflow validation.
inline constexpr std::uint64_t MaxRenderDimension=8192;
inline constexpr std::uint64_t MaxRenderPixels=MaxRenderDimension*MaxRenderDimension;
inline constexpr std::uint64_t MaxDiagnosticPixels=8388608;
inline bool RenderExtent(std::uint64_t width,std::uint64_t height) noexcept {
 return width&&height&&width<=MaxRenderDimension&&height<=MaxRenderDimension;
}
// Published only after reading validated native inputs, before any diagnostic
// allocation. A zero value cannot bootstrap the recurring diffuse replay.
inline std::uint64_t HighResolutionExtent(std::uint64_t width,std::uint64_t height) noexcept {
 return width>=64&&height>=64&&RenderExtent(width,height)&&width*height>MaxDiagnosticPixels?
  (width<<32)|height:0;
}
inline bool IndependentDiffuseBootstrap(std::uint64_t extent,bool diagnosticWaiting,bool ownerPresent) noexcept {
 return extent&&diagnosticWaiting&&!ownerPresent&&HighResolutionExtent(extent>>32,static_cast<std::uint32_t>(extent))==extent;
}
}
