# R416: remove biased endpoint replacement

## Evidence and scope

R415 RELAX Off bypassed diffuse denoising but did not bypass `TemporalGI`.
`gi_live_toggle::Dispatch` traced fresh Owned rays, then ran this additional pass.
`gi_temporal_hit.hlsl` saved fresh hits before reuse, selected a prior endpoint
from a sparse receiver grid, and reused it only after receiver, range, hit and
visibility checks. The shared closest-hit shader overwrote one current slice.
`gi_owned_lighting.hlsl` reevaluated its lighting but still divided the equally
weighted sum by ray count. No acceptance probability or proposal correction
accompanied the selective replacement.

The exact fixture in `tools/test-gi-endpoint-estimator.py` isolates the 65-unit
reuse gate. At a 100-unit base range, endpoints at 10 and 70 units are both
within the fresh producer's minimum range, but only the near endpoint is reusable.
For independent equiprobable near/far draws and alternating receiver parity,
a one-ray near-bright contribution changes from 0.5 to 0.625. Swapping brightness
changes it to 0.375. For N rays with one replaced slice, the shift is +/-1/(8N).
All endpoints in this fixture are valid, visible hits. If both classes are
accepted, the fixture returns 0.5. This proves the replacement rule does not
preserve an unbiased input distribution in general. It is not a measurement
of this game's current scene, nor a replay of its particular random sequence.

## Correction

Remove temporal shader augmentation and the post-ray dispatch callback from the
production path. Preserve the current fresh producer, native ray-count policy,
current-frame evaluator, material guides, and miss/volume shading. The retired
shader/runtime remain in source for audit; no production include calls them.
This eliminates their history allocation, UAV barriers and visibility rays.
Shared hit-tag compatibility remains unchanged to keep this correction narrow.
RELAX Off startup/reset from the preceding source checkpoint ships in this build;
manual Balanced/Original comparisons remain available. No new sampling seed,
spatial blur, radiance clamp or sharpening change is introduced.

## Limits and acceptance

R263 established a growing RR-output pattern while the incoming noise changed;
R264's accepted sample-sequence correction predates this endpoint mechanism.
R378 verifies raw bypass identity in that capture, not correct endpoint weights.
Those captures do not identify endpoint replacement as the cause of the latest
Jesse halo. Existing character, foliage and projector findings remain separate.
See `research/rr-existing-evidence/README.md` for the consolidated evidence.

The regression demonstrates the estimator defect and guards production wiring.
Windows shader/runtime tests and package verification are required before delivery.
Only in-game observation can establish whether all three reported scenes improve
without lost lighting, speckles, boiling or excessive softness. No FPS gain or
complete noise fix is claimed from removing this work alone.
