# R261: steady spatial weighting

R260 user result: ghosting returned with Both Bounded. R259 Both Test remains
the comparison: no reported ghosting or speckles, with floor boiling. R261
keeps the final history blend disabled and tests one current-frame weight.

## Test

Install the full package. Use Native Blend, RR ON, FG OFF. In F10 select
**GI: BOTH STEADY**, close F10 and wait ten seconds. Repeat movement around
the projector and inspect the floor. Compare with **GI: BOTH TEST** (R259).

Check boiling, ghosting, speckles, floor detail, light/shadow edges and FPS.
Send `ControlTOH/Tools/Collect-ControlFG-Compact-Logs.cmd` output. A short video
is useful if the difference is subtle. No material or full RR capture required.
Clamp Only remains the startup default. Diagnostic modes are not persisted.
The rejected Both Bounded is retained only as a comparison, not selected by
this test. DirectStorage and foliage admission are unchanged.

## Source finding and hypothesis

In both audited spatial shaders, DWORDs 1503–1807 derive a radiance-difference
variance scale from the current center and four neighboring samples. The scale
is `min(10 * sqrt(abs(variance)), 1)`. At 1814 it multiplies all four neighboring
weights; the center retains weight 0.375. A small scale makes the result approach
the center sample even though the pass still executes. The scale is recalculated
from current lighting each pass and frame. It may therefore leave low-amplitude
noise unfiltered or vary filter strength as lighting changes. This is a source
mechanism to test, not proof that it causes the reported floor boiling.

Both Steady replaces that scale with 1. The native normal, depth, material-ID,
hit-distance and stencil weights remain unchanged, as do pass count, radii and
normalization. This removes one radiance-based adaptive weight and can smooth
same-surface lighting detail more strongly. Geometry tests do not protect every
illumination boundary, so visual validation must check softness/light leakage.

The first temporal shader is the unchanged R212 adaptive pass used by R259;
the final temporal coefficient stays zero. This mode is not globally history
free: it preserves the initial stabilization and RR's own history.

Relative to native bytecode, only two instructions change: the scale's `min`
becomes `mov 1`, and the final history coefficient becomes zero. No new texture
reads, resources, passes, descriptors, or history-copy changes. Existing R259 and
R260 shader variants retain their prior semantics. Selection is restricted to
the validated GI spatial dispatch scope. Missing optional PSOs use native work
and report incomplete application. Look for `gi_spatial_history build=R261
steady=1 bounded=0 ... complete=1` in the log.

## Validation

Source tests compare every decoded instruction in both variants and require
only those two changes. Other DXBC chunks, bindings and output ABI match.
Windows GPU fixtures execute actual shaders for low-contrast checker noise,
changing current input, bright/dark history independence, material-ID boundaries,
native depth weighting, black input, and optional-PSO fallback. A numerical
reference checks interior pixels against the native stencil and variance math.
Previous temporal and bounded-history fixtures remain. Hook tests cover seven
modes and restoration through temporal/spatial exceptions.

Fixtures isolate the spatial operation on small synthetic textures. They do
not establish full-scene stability, normal-edge behavior, six-pass image quality,
or performance. This is an experimental build, not a confirmed boiling fix.
