#pragma once
namespace rt_spot_capture {
// R131 passed all five startup stages on the RTX 5090. Only the standalone
// preflight and explicitly opted-in diagnostic builds enable GPU capture.
// Ordinary builds retain the R129 isolation policy.
#if defined(CONTROL_CAPTURE_PREFLIGHT) || defined(CONTROL_CAPTURE_DIAGNOSTIC)
inline constexpr bool GpuCaptureEnabled = true;
#else
inline constexpr bool GpuCaptureEnabled = false;
#endif
}
