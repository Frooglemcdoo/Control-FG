# R275 trace and sampling audit

The capture succeeded with right mouse held. Session 33016 accepted F7, counted
down for five seconds, recorded one native GI6 dispatch at present 6013, restored
root/PSO, submitted it, and exported after GPU fence completion. The binary hash
is `9f1810000154dcca73e196b470f1756f204b8cfb01a623e49687b5e6bda17c0a`.
All four uploaded package files passed size/hash checks. No repeat is needed to
recover this trace. R275 is a diagnostic build, not a lighting correction.

## Measured evidence

`tools/analyze_gi_spot_trace.py` produces the checked-in
`research/gi-source-r267/r275-trace-analysis.json`. It honors per-stage validity,
rejects nonfinite valid values and malformed lengths, and excludes vector padding.

- 27,953 records, 3,304 sampled screen coordinates, ray indices 0–5, no overflow.
- 27,655 records reached only the coefficient stage; they are not 27,655 invalid
  rays or zero-valued samples of later stages.
- 174 records reached visibility but not the later contribution stages.
- 124 reached every recorded stage; all have session-local light index 6.
  This is not enough to name that record as the desk lamp without scene correlation.
- All recorded valid values are finite. The spotlight denominator and attenuation
  agree with the native formula within floating-point rounding. Gated RGB also
  agrees with the sampled light RGB, cookie, fade and visibility within rounding.
- The ten largest **partial max-RGB terms** account for about 82.9% of their sum.
  Terms whose light-to-secondary distance squared is below one account for about
  99.2%. These are scene-unit distances, not established meters.

The last two statistics are over a sparse grid in one dispatch. They are NOT
temporal variance, the energy fraction of the rendered image, a plant/hair ROI
measurement, or proof of an invalid estimator. The terms exclude the final
per-pixel depth scaling and sample-count normalization. Selecting only fully
executed records also excludes paths outside the spotlight contribution branch.

## Static source audit

`tools/audit-gi-sampling.py` disassembles both hash-pinned ray libraries with
official DXC, checks their sampling operations and verifies the existing R264
phase-only edit is reversible. It also decodes the actual pinned GI6 DXBC bytes
to check accumulation and sample-count normalization.

Both ray generators map sample coordinates to
`(cos(2*pi*u)*sqrt(1-v), sin(2*pi*u)*sqrt(1-v), sqrt(v))` before their native
normal-basis transform. For uniform inputs and an orthonormal basis, that is
cosine-weighted hemisphere sampling, with PDF `cos(theta)/pi`.
The R264 phase change does not change this mapping.

GI6 accumulates `primary_throughput * secondary_shading`, applies the native
depth scale, then divides by the configured ray count. Its loop uses the same
constant-buffer field as that division. This does not prove that the separate
ray producer and consumer received equal runtime constants, or that every hit
buffer/guide corresponds correctly. The current trace does not capture all of
those values.

No missing inverse-PDF factor is established: cosine sampling cancels the
Lambertian cosine/pi weighting at the primary surface. Adding another inverse
PDF factor without changing the estimator would be unjustified. The trace's
primary-throughput field includes the game's native material response and must
not be interpreted as a bare PDF weight.

## Correction decision

Do not flatten attenuation, raise its floor, add another arbitrary sample weight,
or reintroduce native temporal filtering. These checks do not identify a code
defect that justifies those changes. Do not target session-local light 6 or a
specific plant/hair material.

The next architectural target is reuse of indirect-path samples with correct
weights, initially without temporal history. The candidate must retain secondary
position/normal and radiance plus its proposal density; reuse at another primary
surface needs connection geometry, target-density/Jacobian handling and a fresh
visibility test. Averaging neighboring screen-space colors is not that algorithm
and would repeat the earlier edge/ghosting problems.

Before enabling such a path, recover the producer/consumer contract for those
candidate fields, test constant-radiance energy preservation, a small bright
secondary patch, thin-geometry visibility and disocclusion. Compare mean and
variance against a reference. The current evidence supports investigating this
direction but does not prove it will eliminate every visible speckle.

No new game build or recapture is required for this offline audit. Rendering
behavior remains R275. No performance or in-game quality improvement is claimed.

## Primary references

- [PBRT path tracing](https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Path_Tracing): importance-sampling indirect contributors requires matching weights.
- [PBRT sampling reflection functions](https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Sampling_Reflection_Functions): cosine-weighted sampling and diffuse PDF.
- [ReSTIR GI paper](https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing): indirect-path resampling as an architectural option, not a drop-in denoiser.

Validation: five trace-parser/analysis tests; both pinned native ray libraries;
GI6 normalization instruction checks; existing phase edit/pass/ray/wrap tests.

## Follow-up: existing clamps

The R275 log around frame 6013 confirms two different paths:
`GI_BYPASS_R262 ... mode=raw_bypass copy_applied=1 ... complete=1` and
`RR_CLAMP_STRENGTH_CS3 ... requested=60 effective=60 applied=1`.
RAW BYPASS is not a global clamp-disable setting.

`rr_gi_pipeline.h` selects the bypass PSO for that GI invocation. Its generator
in `gi_motion_history.py` retains only the current t3 mip-zero load and output;
it removes the GI energy clamp, history and neighboring samples. Thus the GI
clamp cannot be the origin of the measured pre-filter spotlight terms, and was
also bypassed downstream in this run. The prior clamp-only/smooth path is a
different experiment: it retains native mip clamping and can add outlier limiting
or depth-tested current-frame smoothing.

The separate specular shader `rr_clamp_strength.hlsl` remains active at 60%.
For positive luminance its scale is `pow(min(L_mip2.5 / L_current, 1), 0.6)`.
This is an exponent, not a 60% linear blend. A current sample 100 times brighter
than the mip reference retains about 6.31% of its RGB. Equal center/reference
luminance retains 100%. The reference has no depth/normal rejection in this
current-frame shader. It can therefore suppress valid thin highlights and
respond to changing neighboring brightness. These are mathematical properties
and plausible visual risks, not measured attribution of this run's artifacts.
Temporal weight is zero in the active fixed shader, so it does not itself retain
previous-frame trails; RR can still react to its changing output.

Do not call the specular clamp cleared merely because GI is bypassed. Before
attributing the final image entirely to GI sampling or implementing a new reuse
architecture, isolate this remaining specular stage with an otherwise identical
raw-specular path. Keep the GI bypass, sampling, light arithmetic and RR settings
fixed. Existing upstream GI evidence remains valid regardless of that result.
