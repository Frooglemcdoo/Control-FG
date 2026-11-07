# R364 owned GI denoiser

Candidate build, not a confirmed visual fix. F10 GI FILTER selects the new filter; GI STABLE retains the R363 comparison. These are mutually exclusive with GI REMOVE. RESET COMPARE restores saved gameplay behavior.

The R363 native temporal shader caps current luminance to min(raw luminance, mip1.2 luminance). The steady spatial variant retains six native passes and replaces the variance multiplier with one. Those are verified shader behaviors; the screenshots do not independently prove which operation caused every reported artifact.

R364 replaces the exact native GI temporal dispatch with authored SM5.1 HLSL. It preserves root bindings, dispatch dimensions, RGB/red-alpha output ABI and native history ownership. Following spatial passes are zero in this mode. No second GI lighting producer is added. The current sample is not capped to mip brightness. One normalized 3x3 full-resolution kernel has relative-depth rejection and radiance-edge weights. Four reprojected history taps are individually depth-tested, bounded to the current neighborhood, and blended according to motion. Native reset and depth-offset constants are retained.

This boundary binds depth, previous depth, motion, radiance and history; it does NOT bind normal or material guides. This initial implementation cannot reject coplanar normal/material boundaries or validate light-path visibility directly. Light changes are handled by current-neighborhood history bounds. Thin isolated features may still be smoothed. In-game brightness, detail, boiling and cost must be compared before making this a default. The old Raw and Stable options remain intact.

Validation: compiler reflection checks slots/spaces/threadgroup; production PSO replacement/fallback/restoration; actual WARP pixel readbacks for uniform brightness despite dark mip, stale bright/dark history, native cut, depth disocclusion/silhouette, offscreen reprojection and checker noise. Existing baseline and capture tests retained.

Research reference: https://github.com/NVIDIA-RTX/NRD documents normal, roughness, depth and motion guides for full-feature denoising. This implementation does not integrate NRD or claim that full guide set.
