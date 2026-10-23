#pragma once
namespace rt_spot_capture {
// R128 crashes after loading on the live RTX 5090. Keep its GPU diagnostics
// out of gameplay until driver-side pipeline and hook validation is available.
// This is a build policy, not an INI option or a switch hidden behind BOTH.
// Only the standalone executable may opt in; shipping DLL builds stay disabled.
#ifdef CONTROL_CAPTURE_PREFLIGHT
inline constexpr bool GpuCaptureEnabled = true;
#else
inline constexpr bool GpuCaptureEnabled = false;
#endif
}
