# R141 — current-pixel spotlight history agreement

The user reports that R140 No History appears to remove the stair/railing flicker
while moving. Evidence: `20261024-1645-24.5201649.mp4`. This isolates temporal
reuse; it does not establish previous-light identity mismatch as the root cause.

## Candidate correction

**History Guard** retains the R139 normal correction, authored-clip rays, centered
spatial softness and existing 3x3 history clamp. Immediately before native temporal
blending it compares the clamped spotlight history with current-pixel visibility.
If their normalized difference exceeds **0.125**, or history is nonfinite, it uses
the current spotlight value. Otherwise it preserves temporal accumulation.
The threshold lives once in `MAX_HISTORY_DIFFERENCE` in the generator.

The old neighborhood clamp can admit both lit and shadowed values at a railing
edge. This additional test restricts reuse to the current pixel's signal. The
maximum inherited visibility error under the native 0.7 blend weight is 0.0875.
This numerical bound is not an image-quality guarantee. Genuine stochastic
variation may reject history too often, increase noise, or cause threshold pops.

All three native spotlight lanes (x/z/w) receive the same guarded value. Point
history (y), native depth/motion rejection, transition reset, spatial filters,
resource layout and dispatch dimensions remain unchanged. No new texture reads,
extra rays, persistent GPU resources or render-thread waits are introduced.

This is **not light-identity validation**. Small discrepancies can still pass;
correct identity-aware reprojection remains a follow-up if this test is inadequate.
NVIDIA's [NRD documentation](https://github.com/NVIDIA-RTX/NRD) describes applying
SIGMA per light for multi-light shadow denoising. That supports keeping light
identity as a separate correctness concern, not treating this threshold as proof.
Control's shader is not NRD and this change does not integrate NRD.

## Test

Install with Control closed; retain existing NVIDIA runtimes. Keep FG off for the
comparison and the same RR/RT/DLSS settings. R139 **Default (Fixed)** remains the
automatic baseline. In F10 select **History Guard**, close the panel and repeat
the stair/railing motion. Compare against Default and **No History**, then stand
still and check noise, softness and thin shadows. Repeat in other affected rooms.

Use Issue Remains / Issue Gone and Save Log; send the log and a short video.
No restart is needed between modes. F9 includes the new mode (25 comparisons,
up to 75 PNGs), but stationary images do not establish motion stability.

## Validation and release status

The emitted instruction interpreter covers 10,000 random signals, exact threshold
boundaries, stale mixed-neighborhood history, nonfinite history and point-lane
preservation. Independent decoding checks the guard operands and proves all
original instructions remain in order. Input SHA checks reject unknown shaders.
Windows CI also creates the real shader PSO on WARP and tests separate history,
spatial and composition routes. Check the associated commit's CI result for its
build status. This candidate needs target-GPU visual and performance validation
before promotion. Optional shader failure leaves Default and No History available.

Tracked in [issue #14](https://github.com/Frooglemcdoo/Control-FG/issues/14).
