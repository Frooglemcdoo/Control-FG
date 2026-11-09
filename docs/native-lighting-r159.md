# R159 — focused shadow blocker capture

## Evidence

R158 PID66884 completed pairs 12288/12289 (corrected RR OFF) and 16944/16945 (RT SPOT OPEN), nine resources per frame, RR bypassed and no reset. All five archive hashes and chunk hashes match; raw sizes are complete. CPU logs record the corrected producer, authored ray offset 384 and selected visibility-one composition. The broad diagonal boundary is absent in the open comparison, including accumulated diffuse lighting. The camera angle changed slightly; this implicates selected spotlight visibility without proving a particular light or blocker.

## Next test

Start a fresh game session, stand on the stairs with the false shadow visible, open F10 and click TRACE SHADOW. Hold the camera still. The panel hides, the trace selects corrected RR OFF and waits five seconds plus at least 90 frames and recorded bindings. It requests one existing bounded GPU capture and three nearby screenshots. A completion message gives the output folder. Send lighting.log, manifest.txt and PNGs; drag that folder onto Package-Lighting-Capture.cmd to produce ZIPs below 200 MB if desired. PACKAGE LIGHT and Shift+F9 still belong to the separate image-pair capture. Plain F9 cancels the active trace. Previous mode is restored on completion/cancellation.

The trace reuses the previously working input extraction, native alpha-tested closest-hit tracing, 4096-record budget, private descriptors, separate submission fences and asynchronous export. No new GPU buffers, tracing shaders, gameplay shaders, queue waits or render-thread file I/O are added. A failed previous GPU trace requires a fresh session. Concurrent image-pair captures are rejected.

## Interpretation

The full-length reference ray supplies biased origin, sampled direction/endpoint, selected light, hit instance/geometry/triangle identity and barycentrics. The context supplies authored cone/emitter/axis/near-plane parameters and view-to-world transform. Composition data supplies actual filtered spot visibility and HDR diffuse/specular before/after the pass.

The offline analyze-shadow-blocker.py script reconstructs the sampled endpoint and applies the accepted authored_clip CPU reference. It classifies hits within the clipped segment, beyond it, near the clip boundary (uncertain), or misses. It groups exact integer blocker IDs per capture/light. Tests cover translated and rotated camera frames, boundary cases and invalid parameters.

These are diagnostic full-length reference rays, not new traces of the clipped four-ray gameplay result. One center-ray hit is not equivalent to filtered visibility; alpha handling, four-ray sampling and filtering remain separate considerations. IDs are scoped to the captured TLAS/table generation and are not asset names. Screenshots provide nearby visual context, not exact synchronization with every readback. No room-specific fix or shadow-removal fix is claimed.

## Validation

Local decoder, blocker classification and capture-policy tests pass. Windows CI validates compilation, WARP capture resources/routing and existing shader checks before delivery. In-game blocker capture and the underlying fix remain pending.
