# R307 — First live GI sampling stage

This opt-in build replaces the random-coordinate generator in both supported GI
ray-generation libraries. It traces the existing number of rays using jittered
equal-area strata in the two-dimensional cosine-hemisphere sampling domain. Seeds
include pixel coordinates, the full current-frame counter, batch and ray index.
The third randomized coordinate drives the existing 0.8–1.2 base-distance law.
There is no 256-frame seed mask or floating-point growing sequence in this stage.

Native hit/miss shaders, foliage alpha tests, material evaluation, ray origin,
cosine mapping, range formula, SBT layout, root signatures, output resources and
GI normalization are retained. No extra ray dispatch or diagnostic scene copy is
added. The normal native command stream owns the resources as before.

This is initial sampling modernization, NOT complete replacement lighting or
ReSTIR GI. There is no reservoir resampling, neighbor reuse, new temporal history,
new visibility pass, higher resolution, or new lighting model. No speckle fix,
variance reduction in this particular scene, or performance gain is yet proven.
Small bright sources can still be noisy at the existing ray count. The benefit
of enabling this stage now is to test an actual rendering change rather than
another observer-only build.

## Run

Install the full ZIP beside Control_DX12.exe, preserving folders. Launch
`ControlTOH/Diagnostics/Start-GI-Rendering-Test.cmd` and choose **1**. Keep RT
indirect lighting on and retain your usual GI ray count, RR and Native Blend
settings. Look at the desk-lamp-lit plant and Jesse's hair, let the camera settle,
then move. Exit normally and send the logs and what changed visually.

Option **2** uses the existing R264 baseline in a separate launch. Normal launch
also retains the existing baseline unless CONTROLTOH_GI_LIVE is set externally.
No F7/F9, RR capture, fixed camera or geometry capture is required. The launcher
explicitly disables the large diagnostic inventories and readbacks. Selection is
restart-scoped; it does not modify persistent graphics settings.

`GI_LIVE_R307 state_created=1` confirms creation of a matched experimental GI
pipeline. That is not a GPU completion or quality claim. An unsupported library
or failed creation falls back to the original state-object creation path.

## R306 evidence and implementation choice

R306 observed 12,800 wrapper releases but retired no tracked AS resources; its
inventory still saturated. Its four GI tables nevertheless validated 35,644
bindings, including 5,105 alpha-tested bindings. That capture does not need
repeating. This in-place producer change does not depend on the diagnostic AS
inventory or its release bridge. Future extra dispatches still need their own
complete lifetime/binding contract; this shortcut does not authorize them.

## Basis and checks

PBRT explains jittered stratification and its variance properties:
https://www.pbr-book.org/4ed/Sampling_and_Reconstruction/Stratified_Sampler
NVIDIA distinguishes initial tracing/shading from ReSTIR GI resampling:
https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md

The transform is hash-pinned and reversibly replaces only three coordinate
definitions in each library. Existing closest-hit/miss code and downstream
instructions remain identical. Both transformed libraries assembled and passed
DXIL validation locally. Emitted arithmetic is checked with float32 rounding for
1, 3, 4, 6, 8, 16 and 64 rays: coordinate bounds, stratum bounds, cosine moments,
range moments, angular-patch energy and frame variation. These statistical checks
are sanity tests, not proof of independent pseudorandom dimensions or low noise
in Control. Windows state-object tests exercise both baseline/experimental
selection, association rebasing, unsupported input and failed creation fallback.

## Delivered validation

Windows foundation run 41678850050 and full build run 41678849824 passed on
2026-11-03. The full build validated both generated DXIL libraries and both
state-object selection modes, then compiled and packaged the proxy. This is
build/fixture evidence, not an in-game quality or performance result.

Artifact 13252711944 contains the verified 75-entry install archive (4,276,241
bytes). Outer and inner ZIP CRC checks passed, the inner SHA256 matched the
packaged checksum, and the DLL contains the GI_LIVE_R307 marker. The rendering
launcher is included.

Install ZIP SHA256:
`87a5485eaa0b3859b37221bdb48bd571864787edabed14b393c638e407e80fcc`
