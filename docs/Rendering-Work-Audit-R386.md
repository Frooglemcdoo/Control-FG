# Rendering work audit — R386

Historical audit. The per-recording allocation issue below was addressed by R388
reuse; it is not a current recurring-cost claim. See
[the performance pivot audit](../research/performance-pivot/audit.md) for verified
R391 timings, current-source checks, and remaining evidence gaps.

Reported regression: approximately 15–30 FPS, user observation. No matched GPU
capture has yet established which pass accounts for that loss. FPS deltas alone
cannot be converted to cost without both frame rates and matching settings.

## Replacement boundaries checked in source

| Feature | Ownership | Source evidence |
|---|---|---|
| Owned GI rays | Replaces the original ray dispatch | `gi_live_toggle::Dispatch` returns true after its custom dispatch; `rt_shadow_capture::DispatchHook` calls the original only when replacement returns false. |
| GI lighting evaluator | Substitutes its PSO at the original compute dispatch | `native_lighting::SetHook` selects either alternate or native PSO; the ordinary dispatch runs once. The legacy additive Owned path is disabled by `SingleProducerEnabled`. |
| RELAX diffuse denoiser | Replaces native GI temporal/spatial filtering on the admitted path | `control_rr_gi::Dispatch` returns immediately after successful RELAX recording. `RRNativeRunGIFilter` requests zero native spatial passes. Failed admission chooses RAW copy, not the old filter. Unintercepted fallback paths must be observed separately. |
| Enhanced spotlight | Replaces the selected native ray dispatch | The same replacement-dispatch return contract applies to `rt_spot_runtime::Dispatch`. |
| Character / near-volume shaders | PSO replacement at native bindings | `Binding` selects alternate or native; it does not issue both draws. |
| Reflection clamp | Replacement at original scoped dispatch | `control_rr_clamp::Dispatch` dispatches the replacement once, then restores state without another dispatch. |
| Native reflection/broad diffuse filters | Default Native Blend does not request preservation | `PreservesSpecularFiltering` and `PreservesBroadDiffuseFiltering` select specific diagnostic modes, not default Native Blend. Check runtime counters for fallback or unexpected modes. |

## Additional work that is real, not duplicate native rendering

- Owned GI issues a separate temporal/visibility ray dispatch after fresh rays.
  It validates reused hit history. It is not the old native ray generator again.
- RELAX exports receiver guides, converts its inputs, runs multiple NRD compute
  passes, and converts the result back. RR reconstruction still runs afterward.
  Thus there are two reconstruction/denoising stages in the overall pipeline;
  RELAX was added to stabilize the noisy input that RR alone did not resolve.
  This is a cost to measure, not something to remove blindly after the quality fix.
- Native shadow filtering can remain for the separate shadow signal. Diffuse GI
  RELAX does not replace that shadow filter.
- `DiffusePipeline::Record` and `D3D12Backend::AllocateRecording` each allocate a
  committed upload constant buffer per recording. This is a concrete CPU/driver
  allocation cost to investigate for fence-safe reuse, not proof of GPU duplication.

## R386 observation

CHECK SETTINGS adds counters only during its ten-second observation window:
Owned/native GI rays, GI evaluator dispatches, temporal visibility, receiver
exports, RELAX internal dispatches and GPU completion, native/custom GI filter
paths, and native specular/broad diffuse filter calls with RR. Native filter
activity in a RELAX window is flagged for review. Window totals cannot prove
same-frame duplication, especially during fallback or transitions.

No rendering algorithm, sample count, quality setting, shadow visibility or
history behavior was changed for this audit. No FPS improvement is claimed.
The next optimization should be selected from measured pass durations and CPU
allocation cost, preserving the accepted Owned GI + RELAX image.
