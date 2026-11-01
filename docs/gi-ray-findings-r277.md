# R277 complete-ray capture findings

Package manifest and all four file hashes verified. Trace SHA256:
`5d7992f6fa5ec78066314f35bc647b0565a5c42eb4e360083b5d39d8d1fd9efb`.
Process 57620, present 8443, GI6, dispatch groups 320x90x1. F7 was accepted
with right mouse held. GPU completion and successful export were logged;
25,200 records retained with no overflow. No repeat is needed for this capture.
Nearby render logs confirm GI RAW BYPASS and raw reflection copy were applied.

The checked-in `r277-ray-analysis.json` is reproduced by
`tools/analyze_gi_ray_trace.py`. All 3,600 sampled pixels have six ray records
and one summary, yielding 21,600 rays. All inspected values are finite.

Maximum absolute reconstruction differences:

| Check | Error |
| --- | ---: |
| Per-ray products versus native accumulated sum | 1.12021e-7 |
| Native sum times actual depth scale divided by ray count versus final GI | 4.96705e-9 |
| Depth-fade formula versus recorded scale | 0 |

These errors are consistent with floating-point rounding. No accumulation or
final-normalization defect is demonstrated in this dispatch. This does not
prove the producer PDF, lighting semantics, or final RR behavior correct.

## Miss fallback must survive a replacement

12,707 rays have the native miss sentinel 65535. They carry positive radiance,
contributing about 82.62% of the sampled grid's normalized luminance sum.
Reject these as finite-surface reuse candidates, but preserve their native
fallback contribution in the GI estimator. Removing them from final GI would
discard valid scene lighting. This percentage is not a full-image measurement.

## Bright contributions

The ten brightest normalized ray contributions account for about 6.09% of the
sampled grid's luminance. At 14 sampled pixels, one of six rays provides over
90% of that pixel's GI luminance. This demonstrates strong within-pixel sample
imbalance in the captured dispatch, not temporal variance or proof of the
specific plant/hair artifact. No image or object-ID mapping accompanies this
trace, so primary/secondary material IDs must not be named as scene objects.

The result supports investigating better sampling of bright indirect surfaces,
while preserving fallback lighting and mean energy. It does not justify
flattening spotlight attenuation, dropping high-energy samples, or claiming
that resampling is already integrated. A future no-reuse candidate round trip
must retain the native primary response, secondary directional response,
depth scaling and miss contribution before adding any sample reuse.
