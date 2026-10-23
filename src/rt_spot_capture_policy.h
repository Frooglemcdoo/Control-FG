#pragma once
namespace rt_spot_capture {
// R128 crashes after loading on the live RTX 5090. Keep its GPU diagnostics
// out of gameplay until driver-side pipeline and hook validation is available.
// This is a build policy, not an INI option or a switch hidden behind BOTH.
inline constexpr bool GpuCaptureEnabled = false;
}
