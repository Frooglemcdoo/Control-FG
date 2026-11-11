# Owned GI source trace (R424)

This closes a specific evidence gap, not the visual bug. The R423 moving pair
contained large diffuse-GI excursions but no selected-owned-evaluator per-ray
light decomposition. The older GI trace variant was attached to the original
native PSO; the owned selector returns a different PSO. R424 attaches its own
instrumented bytecode to that alternate, only after the native shader and root
signature match and the alternate reports mode 2.

The launcher explicitly preserves the current single producer. Setting the
legacy `CONTROLTOH_GI_EVALUATOR=1` would disable the default single producer;
R424 instead clears that flag and sets `CONTROLTOH_GI_SINGLE=1`.

## Capture limits

- One actual owned-evaluator dispatch is instrumented, in motion.
- A movable 256 by 256 region is dense: every receiver, all native rays, one
  summary. No source values are clipped, smoothed, or excluded by brightness.
- The extra UAV is 16 + 262144 * 256 bytes. Reservations are per receiver so a
  failed capacity reservation never writes a partial ray set. Overflow fails
  collection and retains the partial diagnostic files.
- Existing command-list generation checks, submission fence, and PSO/root
  restoration remain in use. Normal launches leave this feature disabled.
- The engine frame joins to the first of two consecutive full-resolution RR
  frames. This is **not** two consecutive source traces. Readback/recording costs
  make the diagnostic unsuitable for performance measurement.

## OGI1 binary contract

Header: four uint32 values: attempted record count, magic `0x4f474931`, stride
256, engine-frame token. Metadata includes the full frame number, dispatch
extent, ROI, record capacity and evaluator identity. Frame values beyond uint32
are rejected before dispatch rather than silently truncated.

Each record is sixteen 16-byte groups. Offsets below are bytes. RGB means three
float32 values; all padding is zero. Kind 1 is a ray, 2 a receiver summary, and
3 an invalid receiver with native zero color and 65504 hit-info output.

| Offset | Ray record | Summary record |
|---|---|---|
| 0 | uint x,y,ray index,kind | uint x,y,ray count,kind |
| 16 | Secondary RGB, cumulative native hit-info | Final RGB, averaged native hit-info |
| 32 | Primary throughput RGB, depth | Same |
| 48 | Weighted RGB, individual distance | Scaled miss-volume RGB, depth-fade scale |
| 64 | Volume/ambient RGB, uint branch | Scaled volume/ambient RGB |
| 80 | Point-light RGB | Scaled point-light RGB |
| 96 | Spotlight RGB | Scaled spotlight RGB |
| 112 | Secondary minus component sum | Output minus component sum |
| 128 | Secondary world position, uint hit material | Reserved |
| 144 | Secondary shading normal, uint ray count | Reserved |
| 160 | Raw native hit position/UV words | Reserved |
| 176 | Raw native hit normal/UV words | Reserved |
| 192 | Primary biased world origin, uint frame token | Same |
| 208 | Primary world shading normal | Same |
| 224 | Native running radiance sum | Final unscaled native sum |
| 240 | Reserved | Reserved |

Branches: 1 miss-volume, 2 surface lit, 4 surface dark. Miss positions/normals
may be unspecified; they are not treated as valid geometry. Native hit-info is
recorded with its native convention, not described as an RR diffuse distance
input. Volume/ambient is the evaluator's irradiance source; it does not by itself
identify screen-space volumetric fog as the culprit.

The decomposition taps material-adjusted volume immediately before native dword
4999, point accumulation at 3807, and independently accumulates spot terms at
4982. Final surface material multiplication is retained. The miss branch taps
2302. An explicit residual captures floating-point differences from splitting
native fused operations; decomposed terms never feed back into native lighting.

## Verification and remaining limits

The generator pins both original shader sources by SHA256 and verifies that
removing marked instrumentation restores the original source exactly. CI compiles
the actual instrumented shader. A separate WARP fixture exercises the same
recorder, dense ROI selection, complete reservations, overflow protection,
disabled stores, padding and parsing. Collector tests reject wrong frames,
incomplete coverage, wrong evaluator identity and inconsistent counts.

These checks do not replay full scene lighting or prove bit-identical in-game
rendering between optimized instrumented and original shaders. They do not prove
a particular light source causes Jesse's artifacts. That attribution awaits the
new capture; existing R423 inputs cannot reconstruct these missing per-ray terms.
