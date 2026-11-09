# R282: GI secondary-hit spotlight stages

R281 successfully captured 1,024 pixels, 6,144 complete rays and 104 executed
spotlight terms in one dispatch. Its brightest captured GI pixel received about
50.14% of its luminance from one recorded spotlight term, while recorded spotlight
terms supplied about 0.49% of ROI luminance. Native attenuation and final accounting
checks passed. This identifies a concentrated contribution, not a proven invalid
estimator or a named lamp. Other bright pixels had no executed spotlight term.
Source trace SHA256: 95d01b5e0d6cabbdd28d966ad5b384cd8104bc39bc737dcf36c6736e4a36f6b1.

R282 preserves that complete-ray capture and adds every visited spotlight's
projection, shadow visibility, cookie and diffuse-response stages. It is a
source diagnostic, not a visual fix. Native arithmetic, texture lookups and
branches remain unchanged; native byte recovery is tested. Additional private
registers and UAV stores can slow the single captured dispatch.

## Run

1. Install all package contents beside Control_DX12.exe, preserving folders.
2. With Control closed, run ControlTOH/Diagnostics/Start-GI-Joint-Trace.cmd.
3. Keep RR and RT indirect ON, Native Blend, FG OFF, GI RAW BYPASS and
   REF CLAMP: BYPASS. Leave the desk lamp intact.
4. Center the crosshair on the same speckled plant leaf. Mouse zoom can remain
   held. The dense capture covers the central 32x32 dispatch pixels.
5. Press F7 once. Wait for the visible ARMED countdown and then SAVED.
   Exit normally and send every generated ZIP. Do not use CAPTURE RR/PACKAGE RR.

Output: GI-Joint-R282-PID.bin. There is no need to repeat R281.
The launcher disables the paused geometry probe. Capture waits for its owning
submission fence before readback. A failed or overflowing trace is not complete.

## Binary contract

Header: count, 0x282, ROI origin X, ROI origin Y. Records remain 256 bytes,
capacity 65536. Kinds 1/2 retain complete-ray and final accounting; kind 3 retains
R281 executed spotlight terms (internal marker 0x281). Kind 4 is one visited
spotlight candidate, emitted after its projection and visibility branches finish.
Keys are dispatch pixel, ray index and session-local light index. A rejected
candidate has no kind-3 contribution. The parser verifies both directions of this
join, stage ordering, material context, cookie multiplication, visibility gating
and final contribution agreement.

Kind-4 byte offsets:

| Offset | Contents |
| --- | --- |
| 0 | Pixel XY, ray index, kind 4 |
| 16 | Native projected [0,1] XYZ, clip W |
| 32 | First/second shadow visibility, blend input, final visibility |
| 48 | Range fade, saturated N dot L, diffuse factor, front gate |
| 64 | Cookie RGB; validity uint at 76 |
| 80 | Depth shadow atlas UV, receiver depth, sampled depth |
| 96 | Native light flags, light index, secondary material, primary material |
| 112 | Native shading-space secondary normal |
| 128 | Native shading-space secondary-to-light direction |
| 144 | Cookie atlas UV |
| 160 | Secondary world position saved before native register reuse |
| 176 | 0x282 marker, total ray count |
| 192 | RGB before cookie |
| 208 | RGB after cookie |
| 224 | RGB after visibility/front gating |

Validity bits: 1 projected; 2 inside projection; 4 visibility evaluated;
8 executed contribution; 16 cookie sampled; 32 depth shadow sampled;
64 first variance shadow evaluated; 128 second variance shadow evaluated.
Fields without their stage are reset padding, not measurements. Cookie defaults
to multiplicative identity when disabled. Projection may be nonfinite for a
rejected candidate; the analyzer reports that explicitly without JSON NaN.
The light index does not identify an authored lamp. Shading-space normals and
light directions are not labeled as world-space vectors.

## Research and scope

The pinned GI6 disassembly evaluates listed lights at each secondary hit using
shadow-map visibility and conditional cookies, then multiplies secondary albedo
and primary throughput and applies depth/ray-count normalization. It does not
justify adding a light-selection PDF or importing MIS weights into this loop.
PBRT explains why sampling must match the actual estimator and why concentrated
contributions can have high variance:
https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Direct_Lighting
https://www.pbr-book.org/4ed/Light_Sources/Light_Sampling
These references do not establish a Control-specific defect. The next decision
will use the recorded stage data; no clamp, light dimming, material exclusion or
geometry/sample-reuse implementation is enabled here.

Windows validation creates the complete instrumented PSO and executes the actual
snapshot/store blocks on WARP with a known cookie/shadow product, a zero-visibility
candidate, an out-of-projection candidate, and buffer overflow guard checks.
This verifies capture mechanics, not gameplay quality or temporal convergence.
