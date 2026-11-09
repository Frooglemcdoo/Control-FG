# R185: bounded reflection sample phases

R184 helped slightly inside the elevator according to the user, but did not
improve the painting. Validated captures still show grain before RR. R185 keeps
the R184 hit-view correction and addresses a separate sampling precision defect.

## Change

The pinned native ray-generation shader computes three phases as
`frac(float(temporalFrame * sampleCount + sampleIndex) * alpha + blueNoise)`.
Its float32 products lose fractional precision as that counter increases,
before the blue-noise shift is applied. For synthetic offsets at counter 50093
and SPEC4, the first two dimensions retain only 64 and 128 distinct phase values
out of 65536 tested offsets. Fused multiply-add does not fix the final rounding
at that magnitude. The third dimension retains 32 values.

Each native float32 alpha is exactly an integer divided by 2^24. R185 computes
the fractional product with unsigned integer multiplication modulo 2^24, then
converts the bounded result to float before adding the native blue-noise shift.
This preserves all 65536 tested offset levels at those synthetic counter values and
at counter one million. The exact constants, index wrapping, GGX angular mapping,
native 0.4 radial truncation, rejection threshold and fallback remain. The third
phase feeds the existing ray-length modulation, so its precision is corrected
as well; the length formula and distribution are not redesigned.

The shader reads `g_uTemporalFrame` (CB register 69.y, byte offset 1108),
not `g_uCurrentFrame` or the CPU engine frame in the capture manifest. The
provided captures do not record that GPU counter. Its actual range/reset
behavior is unverified; the numerical examples above must not be described
as measured phase quantization in these captures. If the game keeps this
counter small, this correction may have negligible visual impact.

This is a correction to the intended sequence, not a new sequence, frozen noise,
extra samples or a spatial/temporal filter. It does not fix every possible source
of variance. The synthetic precision results do not prove better in-game RR
quality or SPEC4 equivalence to SPEC16. The exact float constants have a finite
sequence period (2^23 sample indices); this change does not claim infinite
non-repeating coverage.

The three primary float products become three integer products, masks, bounded
conversions and scalings. No texture reads or rays are added. R172 scheduling and
R176 fallback reuse remain. GPU performance still requires a hardware test.
Other native shader variants pass through unchanged.

## Test

Close Control and install the inner ZIP beside Control_DX12.exe, preserving the
existing NVIDIA runtime files. Launch normally. Use NATIVE BLEND, RR on, FG off,
and SPEC4 with other settings unchanged. The bottom F10 label reads **SPEC: PHASE**.
Inspect the stationary painting and elevator both after a fresh launch and
during normal play. Report splatter and FPS; use SAVE LOG. No exact camera
alignment, SPEC8/16 rerun, or full capture upload is required initially.

`SPEC_PHASE_PRECISION build=R185 applied=1 native_fallback=0` confirms the matching
ray library was substituted. The F10 label alone identifies the build, not
successful shader creation. `SPEC_HIT_VIEW` separately confirms the retained
R184 correction. Unknown variants and creation failures retain native behavior.

## Validation scope

The production transform is hash-pinned and composes with the existing R176
transform. Reversing only its three phase edits recovers that baseline exactly.
An interpreter executes the actual inserted arithmetic against independent
exact rational products for 10223 inputs, including counters matching the capture frame numbers for stress testing,
long sessions and 32-bit overflow. Malformed source is rejected.

Windows CI validates the full transformed DXIL library. A matching compute
fixture executes the phase arithmetic on WARP against a separate 64-bit CPU
oracle, including offset boundaries and randomized indices. Runtime tests
exercise replacement selection, association rebasing, native fallback and
unknown layouts. WARP compute tests do not execute the full DXR scene or RR.

Sampling literature motivates preserving low-discrepancy sample quality, but
the defect and fix above are derived directly from the native shader. This
build does not implement the optimized screen-space samplers from
[Belcour and Heitz (2021)](https://arxiv.org/abs/2105.12620).

Experimental branch only; no production branch or release changes.
