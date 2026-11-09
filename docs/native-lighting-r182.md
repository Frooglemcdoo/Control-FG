# R182: isolate bright reflection contributions

This is a diagnostic build, not a quality or performance fix. R180 placed the
grain in accumulated specular lighting before RR, but did not separate emission
from reflected shading. R181's offline spatial averaging softened legitimate
reflection detail; simple primary-sample stratification did not consistently
improve the native sequence. Neither experiment is enabled here.

## Test

Close Control and install the package beside `Control_DX12.exe`, keeping the
existing NVIDIA runtimes and `ControlFGStreamline` contents. Run
`Start-Spec-Component-Test.cmd`. The launcher refuses to start another copy of
Control and sets a process-local diagnostic variable, not a saved game setting.

1. Select **2: Emission removed**. In game select NATIVE BLEND, RR on, FG off,
   SPEC 4, and keep your other settings unchanged. Capture the painting, then
   the elevator using CAPTURE RR, holding still for each capture. PACKAGE RR
   collects both pairs; keep every ZIP and save a Lighting-R182 log.
2. Close Control. Repeat with **3: Emission only**. Keep its package separate,
   labeled with the launcher option. Report whether the bright splatters remain
   in each mode. No SPEC8/16 comparison is requested.

Normal launch, or launcher option 1, restores the accepted R176 rendering path.
The two diagnostic modes intentionally remove parts of reflected lighting.
They do not remove the scene's primary lights or primary emissive surfaces.
Do not assess them as finished lighting, image-quality or FPS improvements.

## Exact intervention

The hash-locked native reflection resolve still shades every hit and traces
the same rays. One temporary retains either material emission or the remaining
material/environment shading. The native per-hit clamp's luminance denominator
and cap are calculated from the ORIGINAL total radiance in both modes. Only
the numerator and unclamped selection use the isolated component. Environment
misses remain in the shading mode and contribute zero to emission mode.
The accepted R172 immutable-load scheduling and R176 fallback reuse remain.
Other native variants pass through unchanged.

This preserves additive attribution at the per-hit stage, subject to native
floating-point/zero-luminance behavior. Subsequent native/RR filtering and
exposure are nonlinear: do not add the final captures together and expect the
baseline image. Captures contain exposure metadata. They also contain other
lighting channels; a remaining bright spot is not automatically proof of a
particular secondary BRDF. We must check the actual component selection and
resource contents before drawing conclusions.

## Validation and fallbacks

The transformer accepts only the pinned 42,408-byte native shader hash. Tests
undo every edit and recover the exact accepted instruction sequence, verify
component swizzles and unchanged clamp inputs, and check the attribution rule.
Windows CI validates each complete shader through D3D reflection, a compatible
root signature and WARP PSO creation. These checks do not establish in-game
quality or complete scene execution coverage.

The log records `SPEC_COMPONENT requested=... applied=... native_fallback=...`.
A failed PSO creation falls back to the native path and is not a valid isolated
capture. Startup selection avoids adding per-frame hooks, descriptor rebinding,
GPU readback or waits. Readback occurs only when CAPTURE RR is requested.
No changes are made to production branches or releases.
