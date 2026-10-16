M12R73 - validated bilinear history reprojection

R72 supplied previous-frame history, but compatibility often failed before the
history depth check: frame 2519 had 1994 covered samples and only 95 compatible;
frame 3280 had 1913 covered and only 58 compatible. Its combined material/normal
counter cannot establish which condition dominates. Missing lights are not fixed
or explained by those counters.

R73 replaces nearest-only history lookup with four bilinear taps. Every tap must
pass the existing coverage, finite-radiance, material, normal, sample-count and
depth checks. Surviving weights are renormalized. Total valid weight below 0.01
is rejected to avoid amplifying infinitesimal projection error. Thresholds and
history caps are unchanged. This addresses a reproducible case where subpixel
camera motion selects an incompatible nearest normal beside valid history.

Counters now separately record material and normal matches, plus pixels rescued
by a valid neighbor when the nearest tap fails. Coverage/compatibility counters
refer to any weighted tap, so they are not identical to R72's nearest-only counts.
The GPU fixture checks a translated camera, alternating normals, recovered
radiance, missing edge taps, and the existing material/depth rejection cases.
This is not proof that nearest-only lookup accounts for all observed rejection.

Close Control and replace all package files. Keep current PT/game settings.
Do not rerun PT-Enable when already enabled. Collect-PT-Primary-M12R73.cmd packages
logs and settings. No additional rays or light candidates are introduced; history
reads increase from one to at most four. In-game quality and FPS remain unverified.
Native-pass removal, area/emissive-light sampling, all-material support and the
zero environment-light source remain unfinished.
