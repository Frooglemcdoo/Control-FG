# R283: joint spotlight and GI-volume source accounting

R282 completed with 1,024 pixels, 6,144 rays, 3,436 visited spotlight candidates
and no overflow. Of those candidates, 3,434 were outside the native projection
bounds; two executed, both with visibility 1 and strongly attenuating cookies.
The miss fallback supplied 83.12% of sampled ROI GI luminance. This is not proof
that the fallback is defective or that the spotlight cannot cause other speckles.
The two sessions are not a controlled lighting toggle or a semantic material match.
See research/gi-source-r267/r282-stage-findings.json for verified capture data.

R283 retains spotlight stage capture and adds native GI-volume sampling records
for every sampled hit/miss ray. It changes no native lighting instruction, filter,
clamp, material exclusion or light value. It is a diagnostic, not a visual fix.

## Run

1. Install the package beside Control_DX12.exe, preserving folders.
2. With the game closed, run ControlTOH/Diagnostics/Start-GI-Joint-Trace.cmd.
3. Use RR and RT indirect ON, Native Blend, FG OFF, GI RAW BYPASS,
   REF CLAMP: BYPASS. Leave the desk lamp intact.
4. Center the crosshair on a visibly speckled plant leaf. Keep the view still;
   mouse zoom may stay held. Press F7 once and wait for ARMED then SAVED.
5. Exit normally and send every generated ZIP. Do not use CAPTURE RR/PACKAGE RR.

Output: GI-Joint-R283-PID.bin. R282 does not need repeating. The capture still
covers a dense central 32x32 dispatch ROI and one dispatch. The instrumented
frame can take longer; ordinary frames use the existing lighting path. The paused
geometry probe stays disabled. Overflow or incomplete fence completion is failure.

## Captured contract

Header marker is 0x283. Stride/capacity remain 256 bytes / 65,536 records.
Kinds 1–4 retain R282 definitions and their internal markers. Kind 5 records one
volume summary per ray; kind 6 records each volume whose bounds branch executes.
Only matching same-dispatch pixel/ray keys may be joined.

| Kind 5 offset | Value |
| --- | --- |
| 0 | Pixel XY, ray, kind |
| 16 | GI lookup world position |
| 32 | GI lookup normal |
| 48 | Reconstructed ray direction |
| 64 | Native weighted volume RGB sum and total weight |
| 80 | Native normalized volume RGB before ambient |
| 96 | Ambient RGB from illuminationvolume CB |
| 112 | Final volume RGB after ambient |
| 128 | Primary throughput RGB and native miss conversion scale |
| 144 | Primary material, total ray count, secondary material, 0x283 |
| 160 | Native active-volume count |

| Kind 6 offset | Value |
| --- | --- |
| 0 | Pixel XY, ray, kind |
| 16 | Volume-local lookup position before tree traversal |
| 32 | Brick texture UVW before directional samples |
| 48 | Native volume RGB and effective weight |
| 64 | Native running weighted RGB sum and weight after this volume |
| 80 | Volume index, brick-index offset/sentinel, weight multiplier |
| 96 | Primary material, ray count, secondary material, 0x283 |

The analyzer checks complete volume-summary coverage, unique/bounded volume
indices, material joins, weighted RGB/weight sums, normalization plus ambient,
and miss radiance against the complete-ray record. A zero-weight blend is valid
and uses the native unnormalized sum before ambient. Per-volume records do not
claim to capture all six individual directional texture samples. The snapshot
observes consumer values; it does not establish authored volume quality or temporal
variance. Native color is retained even if a volume's brick sentinel selects the
engine's diagnostic fallback color.

## Source audit and research

Pinned GI6 SHEX offsets: 1093 ray direction; 1122 lookup position/normal;
1192 volume-local coordinate; 1866 brick UVW; 2218 completed per-volume blend;
2234 completed weighted sum; 2250 normalized RGB; 2259 ambient-added result;
2302 miss conversion. Each added block preserves native registers and resources,
and removing instrumentation must reconstruct the hash-pinned native bytes.

Remedy/NVIDIA's *Ray Tracing in Control*, sections 46.1.2 and 46.4, describes
precomputed voxel GI and its use for near-field diffuse ray misses. This supports
inspecting that path, not disabling it or treating its contribution as an error.
https://developer.download.nvidia.com/ray-tracing-gems/rtg2-chapter46-preprint.pdf

Windows tests must create the complete PSO and execute the actual capture blocks,
including a known volume blend, ambient CB read, hit/miss context and final miss
conversion. Parser tests also reject missing/duplicate/mismatched records and
incorrect products, while accepting ambient-only zero-volume cases. These tests
validate diagnostic mechanics, not gameplay image quality.
