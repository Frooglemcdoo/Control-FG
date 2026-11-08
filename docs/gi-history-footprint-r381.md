# R381: validate history samples before interpolation

The R380 desk video still shows shading fluctuation. Its UI confirms Owned GI,
Stable enabled, and the separate Owned GI filter disabled. RAW bypass is the
visual reference. Intermediate raw GI variance is not proof of final-image noise.

## Reproduced shader defect

The inherited temporal path bilinearly sampled previous depth, compared that
average with current depth, then accepted/rejected a bilinearly sampled history
color. At a boundary this can admit a foreign surface or reject a valid sample
as fractional pixel position changes. Current-frame history color bounds do not
prevent it when the two radiances are inside those bounds.

`test-gi-history-footprint.cpp` executes production DXBC against the exact R380
control generated with `validated_history=False`. It keeps the receiver and
history radiances fixed and changes fractional reprojection to .25/.5/.75.
Windows WARP run 42429395780 produced:

| Phase | R380 | Per-sample validation |
|---|---:|---:|
| .25 | .445 | .305 |
| .50 | .585 | .305 |
| .75 | .550 | .305 |

Additional checks: no valid contributing depths returns current .55; a valid
continuous surface retains the ordinary .585 result; valid bright-only history
retains .865. Tests check finite output and the red-in-alpha ABI.

## Change

Stable temporal filtering reconstructs the 2x2 history footprint, checks each
previous linear-depth X sample (including native previous-view depth offset),
rejects out-of-screen taps, and renormalizes valid bilinear weights. Zero valid
weight disables history. Native camera-cut weight, current-frame gather, color
bounds, blend coefficient, resource bindings and output contract are retained.
No changes to RAW, ray generation, the spatial shader or pass count.

This follows the per-sample rejection/custom bilinear weighting described by
NVIDIA, *Fast Denoising with Self-Stabilizing Recurrent Blurs*, slides 27–30:
https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf

The implementation retains the existing relative-depth criterion; it does not
claim full NRD integration, normal/material history validation, or plane-distance
reprojection. It adds six texture sample instructions and two scratch registers
to the Stable temporal shader. No new dispatches, resources, or idle diagnostics.
GPU timing on the user's hardware remains unmeasured.

## Six-pass validation gap corrected

The old full-chain test dispatched two spatial passes, while the capture showed
six. It now runs both counts, sets the actual pass index/count (strides 1/1/2/2/4/4),
and feeds the last result into the next frame for 32 frames. Six-pass static
surfaces did not reproduce the reported flicker; this rules out that simple
fixture, not the in-game spatial chain. The original two-pass assertions remain.
Six-pass metrics are reported, with no invented thresholds or visible-quality claim.
The 32x16 fixture is small and its image boundaries affect wide filters; it is
not a full-resolution game replay or an RR test.

The targeted failure is reproduced and corrected. Whether it accounts for the
books/plant fluctuation still requires the user's in-game comparison. No general
flicker-free, brightness-parity, or performance-parity claim is made.

## User validation: rejected

The subsequent 9.1-second video `20261108-0036-01.6108770.mp4` reports a worse
result. The visible panel confirms R381, GI Owned, GI Stable ON, GI Filter OFF,
clamp 60%, and four rays. Extracted desk/plant frames still show changing shading.
Treat R381 as a visual regression, not a successful flicker fix.

A concrete untested assumption is present in the new footprint reconstruction:
`size_prev`/`inv_prev` come from sys_constants.g_vOutputRes/g_vInvOutputRes, rather
than the bound history SRV extent. All footprint fixtures currently set current,
previous-depth, history and output sizes equal. Native code using those constants
for normalized reprojection does not prove they describe the history texel grid.
The existing R378 exports have different RR input/output sizes, but do not capture
the native temporal history SRV extent and constants together; that is not proof
of an actual mismatch in this dispatch. Do not label the mismatch the root cause
without evidence. Validate unequal dimensions and the actual binding contract
before another per-tap change. Also test sloped receiver depth and temporal jitter;
flat synthetic depth boundaries did not predict the observed game result.
