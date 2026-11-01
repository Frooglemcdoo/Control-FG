# R262: verified GI denoiser bypass experiment

R261 user result: reduced boiling, ghosting and speckles remained gone. The
uploaded compact bundle confirms both steady PSOs were created, but lacked
lighting per-pass records. R261 remains available as GI: BOTH STEADY.

## Test

Install the complete package. Use Native Blend, RR ON, FG OFF. Select F10
**GI: RAW BYPASS**, close F10, and wait ten seconds. Repeat the same projector
floor movement. Compare with **GI: BOTH STEADY**, also allowing ten seconds.
Report boiling, speckles, ghosting, brightness and lighting detail. Collect
`ControlTOH/Tools/Collect-ControlFG-Compact-Logs.cmd` after both modes. No full
material capture required for this first bypass comparison.

## Exact scope

This is a GI-only diagnostic, not a claim that every native denoiser in the
entire RR path has been removed. The existing frame-latched Native Blend/RR
eligibility gates remain. RR-off and other ineligible native calls are unchanged.
Clamp Only remains the startup default; diagnostic mode selection is not saved.

The exact native temporal shader is replaced by an integer mip-0 load of t3
at the native dispatch pixel. It writes RGB and red in alpha, matching the
native output ABI. There is no luminance cap, mip-based energy clamp, neighbor
filter, history sample, temporal weighting, depth or motion-vector sample.
The existing output allocation, binding, copies and history bookkeeping remain.
Zero spatial passes are requested, including removal of the final spatial
history blend. Native composition outside this function remains untouched.
The initial shader's native operation was same-resolution texel loading, so this
copy introduces no resolution conversion or sampler filtering.

This copy preserves finite negative and high-intensity values; it does not hide
fireflies. It is intentionally distinct from Clamp Only/Smooth Off, which still
performs the native energy clamp. RR still performs its own reconstruction.
Other upstream/downstream filtering and RR guide validity require further audit.

## Verification and limitations

The generator SHA-locks the source. Instruction tests require exactly coordinate
calculation, one current-pixel load, RGB/red swizzle, one UAV store and return.
Bindings and non-code DXBC chunks remain identical. WARP pixel tests cover bright
peaks, checker noise, black/negative input, conflicting mip/history values, depth
edges and camera cut. Production hook tests cover eight modes, eligibility,
fallback, exceptions and flag restoration. Missing bypass PSO uses native work
and reports incomplete application; it never silently selects Clamp Only.

Probe logs now include GI mode transitions and steady per-pass application
counts, avoiding dependence on the separate lighting log. For the bypass require
`GI_BYPASS_R262 ... copy_applied=1 spatial_requested=0 spatial_observed=0 complete=1`.
Creation `ready=1` alone is not proof of application. Counters are sampled, not a
complete frame capture. This first build verifies the bypass; synchronized
multi-stage image sequences and reprojection/guide audits remain subsequent
work if the result is unstable. No claim of improved quality or performance.

DirectStorage, foliage admission, reflection sidecars and startup defaults are
unchanged. NVIDIA's RR checklist calls for disabling other denoisers:
https://developer.nvidia.com/blog/how-to-integrate-nvidia-dlss-4-into-your-game-with-nvidia-streamline/
