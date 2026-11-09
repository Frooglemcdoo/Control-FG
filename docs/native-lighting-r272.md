# R272 — Capture recovery and export diagnostics

This is a capture reliability update, not a GI speckle correction. Existing
R271 captures remain usable; no repeat capture is requested for this update.

PACKAGE RR remains available during capture. While busy it collects existing
logs and attempt metadata without reading partially written image buffers or
changing GPU resource ownership. Lighting mutations remain blocked during capture.

Image export now holds a separate worker-only lock across disk I/O. The shared
capture lock is used for ownership checks and final resource release, not Map
or file writes. Exports remain serialized, preserving the previous disk workload.
Each image is unmapped after its write. Per-buffer Map and write timing plus
whole-frame export timing identify which stage stalls in future diagnostics.
GPU completion is still required before export; a timeout cannot resurrect a
failed slot or release in-flight resources.

The interrupted R271 session (PID 24416) recovered three pairs, frames 4578/4579,
5286/5287, and 6069/6070. All 14 package hashes and 130 chunk hashes verified;
117 files were restored. All three pairs and six GI-hit payloads passed the
collector's validation. Each frame contains one native batch, six physical
array slices, two 128x128 regions, and 3,538,944 hit-buffer bytes.

Raw pre-filter GI RGB is exactly equal to GI RGB at the RR boundary in all six
frames. Neither full-frame raw GI RGB nor valid captured secondary-hit position
and normal/UV values contains NaN or infinity. This verifies the bypass for this
run; it does not establish the cause of the lamp-dependent speckles.

The native consumer compares hit material with 0xffff at SHEX DWORD 1093.
Using that sentinel, the captured sample miss fractions range from 57.4% to
69.6%. A miss is not corrupted data: the native shader selects a fallback
position/normal. These fractions alone cannot diagnose variance or lighting.
Runtime SRV slice mapping and per-light radiance operands remain unverified.
No light attenuation, sampling, filtering, material admission, or RR-guide
changes are included in R272.

The log has adjacent gaps of 31.31 and 13.61 seconds during the sixth export.
The export ultimately succeeds. Pair four then prepares but never records;
RR later reports off. The exact blocking call is not logged in R271, so the
45-second freeze is not claimed fixed by this lock change.
