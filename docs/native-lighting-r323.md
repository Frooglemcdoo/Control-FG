# R323 — stop failed-path preparation and reuse geometry mapping

R322 admitted 1081 owned passes, then permanently invalidated scene tracking with `live_incomplete_or_replayed_recording`. Preparation nevertheless ran 5762 times. That old reason combines several causes; the existing capture cannot identify which occurred. Earlier `instance_blas_owner` rejection is scoped to an acceleration structure and is not proof of the later global failure.

Across R322 CPU metric windows, scene admission averaged 15.104 ms, including 8.670 ms mapping, 2.792 ms selection and 3.100 ms table commit. Preparation averaged 8.684 ms. These are inclusive CPU scopes, not GPU timings or an additive frame-time breakdown.

Changes:
- Reject an unhealthy catalog before preflight shortcuts allocate or record private GI work, including failure discovered while polling fences. Native fallback remains available.
- Preserve the first recording failure reason and report incomplete, duplicate and replay failures separately. Complete command-list recordings with no scene events can be replayed without invalidating scene history. Incomplete recordings and replay of scene/producer/owned-use events still fail closed; GPU lifetime retention remains unchanged.
- Reuse a validated geometry conversion for adjacent instances only when BLAS address, allocation token, revision, geometry index and the complete native shader-table row match. This cache exists only inside one owner-locked preparation call. Witness validation, row-conflict checks and owner retention still run. Material/alpha/shader differences cannot reuse the cached conversion.

Validation: scene-packet regression covers both index widths, repeated identical instances, distinct materials, closure, stale ownership, bounds and conflicting rows. Clean-baseline verification passes. Windows CI must additionally pass the complete mod build and existing GI/queue tests before distribution.

This does not establish the cause of R322's global recording failure or promise restored native FPS. The new diagnostic distinguishes that cause if it recurs. Native GI still executes for fallback, and spatial/temporal sample reuse is not enabled. Resolution, ray count and shading are unchanged in R323.

Visual expectation: broadly native brightness and color, with the eventual target of steadier lamp-lit hair/foliage, fewer stationary speckles and no motion trails. Similar overall appearance alone does not prove the replacement is inactive. An OWNED indication is CPU admission status, not independent GPU-output validation; WAIT indicates it has not admitted the replacement.

Install preserving folders. Launch ControlTOH/Diagnostics/Start-GI-Owned-Test.cmd, retaining RR, RT indirect lighting and GI RAW BYPASS. Keep sampling settings unchanged for comparison. Report FPS and whether OWNED remains active, then exit normally and send the automatic ZIP. No Capture RR or standalone producer recapture is needed.
