# R260: bounded final-pass GI history

R259 user result: Both Test removes ghosting and speckles, but floor lighting
under the projector boils. The eight-second submitted video is consistent with
changing blotchy floor lighting. No R259 runtime log accompanied that video.
This build tests restoring bounded final-pass history; it is not a proven fix.

## Test

Install the complete package. Use Native Blend, RR ON, FG OFF. Cycle F10's GI
button to **GI: BOTH BOUNDED** (after BOTH TEST), close F10 and wait ten seconds.
Repeat movement around the projector. Compare with **GI: BOTH TEST**, the exact
R259 no-second-history variant, and optionally original **GI: BOTH**.

Report floor boiling, trails, speckles, brightness changes and performance.
Send `ControlTOH/Tools/Collect-ControlFG-Compact-Logs.cmd` output. No new material
or full RR image capture is requested. Clamp Only remains the startup default;
the diagnostic mode is not persisted. DirectStorage and foliage admission are
unchanged.

## Scoped change

Both Bounded retains the R212 initial temporal pass and all native spatial
passes. On the final spatial pass, history RGB is clipped to an interval around
the current spatially filtered RGB, immediately before the native blend:

`lower = max(0, 0.75 * current - 0.001)`

`upper = 1.25 * current + 0.001`

`history = min(upper, max(lower, history))`

The interval is experimental, in native shader radiance units. It permits
moderate variation to average across frames while restricting stale lighting.
It cannot guarantee zero trails or preservation of every valid lighting change.
Native CameraCut, relative depth rejection, motion confidence and maximum 70%
history weight remain unchanged. This is a clipping-only test, not another
change to the rejection thresholds tried in R210/R211.

Only five arithmetic instructions and two scratch registers are added to each
exact audited spatial shader. No additional texture samples, descriptors,
render passes, history resources, clears or history-copy timing changes.
Existing R259 and original shaders retain their previous bytes. The new mode
uses the validated spatial dispatch scope and restores PSO/scope on exceptions.
Unavailable bounded PSOs use native dispatch and report incomplete application;
they do not silently substitute the R259 variant.

Log `gi_spatial_history build=R260 bounded=1 ... complete=1` requires all expected
spatial replacements and the initial temporal replacement. A fallback interval
is not evidence that the bounded experiment ran.

## Validation

Bytecode tests verify both original SHA256 hashes, all original instructions
except scratch count, the exact five inserted operations, unchanged non-SHEX
chunks, descriptor ABI, and checksums. The existing R259 invariant remains.
Windows GPU fixtures run actual native/R259/bounded bytecode with eight-frame
feedback. Cases cover stale bright/dark history, stable history, nonzero motion,
alternating current lighting, depth rejection, camera cuts, non-final passes,
black input and missing bounded PSO fallback. Six-mode hook tests cover context
gates and exceptions in both temporal and spatial dispatch.

The fixtures test uniform interior regions and the final history operation;
they do not reproduce the complete six-pass scene, RR, silhouettes, or exposure
changes. In-game validation remains necessary.
