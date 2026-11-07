# R369 owned GI temporal correction

R368 video: colored speckles on the planter/hand, changing low-frequency blotches during movement. The final-color recording does not establish sole buffer provenance or certify the selected runtime PSO.

Source audit found that R364/R368 temporal filtering let an intense sample influence both its own averaging weight and the history min/max box. R368 also reduced history weight with screen-space speed even when reprojection passed depth validation.

R369 gathers depth-compatible 3x3 samples, detects unsupported one/two-sample peaks using the third-ranked luminance and a trimmed mean, and replaces only those exceptional samples before constructing the filter and history statistics. Supported thin lines and broad bright regions are not subject to a mip-based energy cap. History uses cleaned local variance bounds and retains 94% weight at full bilinear validity, independent of screen speed. Native depth/cut checks, previous-depth offset, UV conventions and red-in-alpha ABI remain intact. The existing single surface-guided spatial pass is unchanged.

Limitations: single-pixel genuine indirect features may be classified as unsupported, and the temporal stage lacks a normal/material guide. The spatial pass retains those guides. This does not add NRD or a persistent moments buffer. No in-game improvement is claimed from synthetic testing.

Validation: existing production PSO, brightness, depth/normal/material-edge and camera-cut fixtures; new 32-frame WARP test executes the shipped temporal and spatial bytecode with spatial output feeding subsequent history. It compares R368 temporal with R369 on stationary and moving colored fireflies, light removal and unbiased diffuse noise. Acceptance checks measure energy, error, temporal fluctuation and output ABI.

References reviewed:
- Control developers, Ray Tracing Gems II chapter 46, sections 46.6.1 and 46.6.2: https://developer.download.nvidia.com/ray-tracing-gems/rtg2-chapter46-preprint.pdf
- NVIDIA NRD integration/reprojection guidance: https://github.com/NVIDIA-RTX/NRD

These inform the investigation; the replacement remains bound to the audited Control ABI rather than an assumed NRD resource layout.
