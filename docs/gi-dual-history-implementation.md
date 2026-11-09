# R383 dual-history GI integration

R382 remains the user's last tested build. It corrected the old sloped-history
artifact, but the desk/plant video still fluctuated. RAW+RR is the visual
reference; noisy intermediate GI alone is not proof of final-image instability.

## Live implementation

The Stable temporal PSO now selects `gi_dual_history.hlsl` through
`gi_dual_history_runtime.h` when exact native bindings, target dimensions,
queue ownership, Reset coverage and pool availability are proven. The native
b0/b1 CBVs, motion convention, slope-aware per-tap depth validation and
RGB/red-alpha output ABI are retained. Fast RGB/age and luminance moments/long
age live in independent RGBA32F sidecars at t5/t6 and u1/u2. Native t4 remains
the previous final spatial result. Native camera cuts invalidate history.

`PTGlass::LightingTableShadow` mirrors CPU descriptors only during the exact
GI temporal ConstantTable bind. A fresh 49-descriptor table in the same native
heap adds the sidecars; dispatch restores root table 0 and the original PSO.
No heap or root-signature switch is needed. No legacy AS descriptor index or
scene/shadow inventory is enabled. That legacy ownership machinery is disabled
by SingleProducerEnabled and must not become a hidden dependency.

The independent owner has four slots, one actual submission queue and its own
completion fence. It retains closed command-list references through successful
Reset, in addition to writer/reader fence retirement. A source must be from the
immediately previous frame and match view, dimensions and producer epoch.
Cancellation is allowed only after Reset proves the unsubmitted commands gone.
Ambiguous queue/replay/fence failures poison reuse and retain allocations.
Exhaustion never waits: it selects the R382 temporal fallback.

F10 reports DUAL after a successful CPU recording, WAIT before one, and FALLBACK
when R382 temporal is used. This does not claim GPU completion. Captures remain
on-demand. Initial activation/fatal errors have bounded logging.

## Spatial-pass correction

The expanded compact-light regression exposed excessive spreading with six
spatial passes. Stable now caps native spatial passes at two. RAW and other
filter modes keep their existing pass policy. Both the candidate and fallback
use that cap. This is an intentional change from R382's six-pass behavior.

Measured in Windows WARP before the final release checks:
- Alternating .2/.6 input: R382 delta RMS .2586, candidate .006794; mean .396823
  against ground truth .4 (R382 mean .365).
- Independent noise, two passes: R382 delta .013024, candidate .001391.
- Independent noise, six-pass diagnostic: R382 delta .004405, candidate .000494.
- Compact 5x5 light input total energy 148.7. Two-pass candidate 147.869,
  R382 134.681; centre candidate 3.687 vs R382 3.068.
- Six-pass diagnostic spreads the centre to .754 and total energy to 158.323.
  This configuration is deliberately NOT selected by R383 Stable.
- Light-off/on, depth disocclusion and camera-cut fixtures retain their checks.

Tests retain strict temporal and two-pass production energy assertions. Six-pass
metrics are diagnostic, not a claim that six-pass quality is acceptable. The
native adapter GPU fixture executes four histories behind a deliberately held
queue fence, rejects a fifth allocation, checks output energy and retirement
only after GPU completion plus Reset. A post-dispatch probe also verifies native
PSO and descriptor-table restoration (passed in the final release run).

## Validation status / limits

The temporal-only candidate had passed earlier WARP tests. During expansion,
a fixture released a descriptor heap while a command list still referenced it;
that test bug is fixed by retaining the heap. CMD checks now reject every
nonzero exit code, including negative process-crash status, rather than allowing
some GPU test crashes to appear successful.

Final full build run 42437895258 / job 128600607056 passed on commit
`eefa6a47b84cd52518c99174d3cf94398ddbb816` (source tree
`6ef9af13390f677d5f981de772ec05e33d16d612`). The native scope fixture now
uses the live context ABI and expects two passes for Stable capture phases.
Shader, delayed-fence/Reset, binding restoration, legacy regression, DLL compile
and complete package checks passed. Foundation run 42437895340 also passed.

The downloaded full ZIP contains 81 entries and the exact 22,848-byte tested
DXBC is embedded in dxgi.dll; R383/DUAL/FALLBACK labels and adapter code verified.
- ZIP SHA256: `b7cab7b1eedccde1e1ebd30aa3e62cd3a205f9119c17f56e562dd0a77cbc379f`
- DXBC SHA256: `69bcda7d64a9934c7194919851fc5b87fb5d0963789f7b5cfe2b048ecbb3facd`
- Delivered full build: ControlTOH-R383.zip, Library
  `libfile_21bc48f2e01481918dd76b5656754f67`.

Synthetic tests do not prove the in-game desk/plant flicker is fixed, nor measure
RTX 5090 performance. The new temporal shader adds two sidecar reads/writes,
while Stable removes four spatial dispatches relative to the native six-pass
configuration. Storage is bounded to four pairs (up to 1 GiB nominal, with a
transient extra pair during replacement allocation). Camera motion, skinned
characters, real lighting changes and actual frame times still need game tests.

Research basis: NVIDIA NRD documentation on fast history, temporal confidence,
input signal quality and integration; SVGF temporal luminance variance.
https://github.com/NVIDIA-RTX/NRD
https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced
