# RR-first investigation after R415

User accepted target: stable Ray Reconstruction with RELAX disabled. R415 Balanced remains too soft and video 20261110-2015-40.8074597.mp4 shows uneven lighting around Jesse. Root cause is not established.

Source change: CONTROL_NRD_GI startup and ResetManual now select manualGI=4 (existing raw bypass), with the performance-only filter control also defaulting off. Manual RELAX comparisons and automated capture overrides remain available. Legacy non-NRD default remains unchanged. Packaged R415 has NOT been rebuilt with this change; selecting RELAX OFF in that existing build uses the same runtime route.

Local tests: test-relax-quality checks NRD default, Off/Balanced/Original routing, reset and capture override priority; test-rt-surface-source validates legacy routing; 132-file baseline integrity passed. No live game stability or performance gain is claimed.

Existing evidence: R414 performance ZIP has timings/settings and an empty RELAX failure CSV, but no image buffers; the run is incomplete due to focus loss. Video shows Balanced at ~12s, Original at ~17s, Off at ~22s. Different poses and camera/pause state prevent attributing the artifact from these frames alone. Prior native input audit establishes currentUV+motion and audited camera jitter conversion; no evidence currently justifies changing sign or units.

RR guide binding audit: diffuse/specular albedo, normal/roughness, optional specular hit distance and matrices are bound and checked. Identity checks do not validate semantic correctness per pixel. RELAX receiver exports and RR guides are distinct paths. Do not assume RELAX guide correctness proves RR guide correctness.

Next evidence: use existing R415, RR ON, FG OFF, RELAX OFF in the projector/Jesse scene. CAPTURE RR then PACKAGE RR collects pre-filter GI, GI at RR input, composed RR input, native depth/motion/G-buffers, RR guides and RR output. Hold still for its existing automatic pairs. If a capture with Original is already underway, preserve it; it remains a useful filtered reference. No new performance run, PIX trace or blanket smoothing change is required. The stationary capture can isolate spatial/signal defects; it cannot by itself prove dynamic disocclusion correctness.

