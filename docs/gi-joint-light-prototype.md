# Same-dispatch spotlight and GI prototype

`tools/gi_joint_light_trace.py` now generates a combined instrumented GI6 shader
from the hash-pinned native bytes. It is NOT selected by the runtime or shipped.
Existing R278 generation remains byte-identical with and without the newly
exposed instrumentation plan. Native instruction recovery is exact for the new
prototype too. A successful bytecode decode is not GPU validation.

The stream keeps the complete ray and summary records, and adds kind 3 records
at native spotlight accumulation. The record shares pixel and ray indices with
the complete-ray record. Fields include gated light RGB, primary throughput,
secondary albedo, native light position, attenuation coefficients, light color
and range, distance squared, denominator, attenuation, secondary diffuse factor,
session-local light ID, ray count, and primary/secondary materials. Position
fields retain native coordinate conventions; do not call them world-space
without verifying their transforms. The spotlight marker is 0x281 at byte 172.

The per-light contribution is:
`primary_throughput * secondary_albedo * diffuse_factor * gated_light_RGB`.
This is normalized offline with the SAME pixel's recorded depth scale and total
ray count. The shader's native spotlight MAD at SHEX dword 4982 and secondary
albedo multiply at 5011 are pinned and checked. There is no extra inverse-PDF
factor, clamp, shadow bypass, light-energy change or image filter.

`tools/analyze_gi_joint_trace.py` validates the original ray stream, joins executed
spot records to their pixel/ray/material context, checks the product and primary
throughput, and normalizes using the paired summary. Duplicate pixel/ray/light
keys, unknown formats, missing context, nonfinite values and overflow fail.
Skipped light branches have no kind 3 record: absence is NOT a measured zero.
A light index alone does not name the desk lamp. Separately rounded products may
differ from contributions implicit in native fused accumulation.

Three tests cover byte-identical legacy generation, native-byte recovery and
identity rejection, legacy parsing, and synthetic mixed-stream normalization,
material-context mismatch and duplicate rejection. Existing eight ray-parser
tests also pass. Run `python tools/test-gi-joint-light.py`.

Before shipping: add focused ROI selection instead of the inherited sparse
32-pixel grid; execute exact injected blocks with a GPU oracle; extend the
runtime/collector for the mixed format; capture same-frame boundary evidence.
No in-game improvement or root cause is claimed by this prototype.
