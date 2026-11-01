# R264: GI sampling sequence experiment

This is a diagnostic build, not a confirmed speckle fix or a new baseline.
Install the complete package beside Control_DX12.exe and launch normally.
Keep RR ON, Native Blend, GI samples 6, GI RAW BYPASS, and FG OFF.
At the same projector floor, stand still for at least 30 seconds. Then move the
camera to check ghosting and boiling. Send the compact logs and the visual
result. A large capture is optional: the existing one-click four-pair CAPTURE RR
and PACKAGE RR controls are retained if we need more pixel evidence.

Only GI ray-sampling phase selection changes from R263. The two exact SHA-matched
native GI ray libraries use `(g_uCurrentFrame & 255)` in place of
`g_uTemporalFrame` when forming `(frame * passCount + pass) * rayCount + ray`.
Ray counts, allocation, ray direction mapping, radii, geometry/material handling,
lighting weights, RR guides, GI bypass, reflections and DirectStorage are unchanged.

The native renderer writes its temporal phase as currentFrame & 3 when temporal
sampling is enabled (and a constant 1 in the alternate branch). The GI ray shader
reads that field at sys_constants[69].y. R263 shows recurrent noisy GI and a
persistent RR output pattern, but it does not prove that period is the sole cause.
This build tests that causal hypothesis with a 256-frame bounded sequence. No
shared native provider is overwritten. No extra rays are requested. Actual GPU
cost and image quality still require the in-game test.

IMPORTANT: substitution happens when the GI DXR pipeline is created. Therefore
GI sampling changes in native/RR-off modes too in this diagnostic build. This is
not an RR-only production integration. Do not treat native mode in this build as
an untouched native reference. A production implementation needs mode-scoped
selection after the hypothesis is verified.

Only exact 24472/27904-byte, SHA256-matched libraries are replaced. Unknown
libraries, ambiguous duplicate matches, invalid association layouts and failed
creation fall back to the original. Descriptor arrays and associations are copied
and rebased; engine descriptors are not modified. Compact logs record
`GI_SEQUENCE_R264 applied=1 variant=... frame_mask=255 ...` on successful creation.
That proves creation, not that an arbitrary captured frame executed the library.

The stationary capture protocol and its R263 markers are retained because its
buffer layout and validation rules are unchanged. Metadata adds
`gi_sampling_experiment: R264_current_frame_256_creation` to distinguish this test.
Offline checks verify exact/reversible source edits, altered-source rejection,
pass/ray separation and wraparound. Windows checks assemble and validate both
actual DXIL libraries and test creation/association/fallback behavior. These are
not GPU image-quality tests.
