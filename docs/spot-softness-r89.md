# R89: spotlight penumbra estimate

R88 is the user-confirmed baseline. The screenshots show a tighter shadow and
reduced soft fringe. The log confirms selected-spotlight replacement and native
spatial/temporal filtering were recorded, including with RR enabled.

The previous four-ray shader used the minimum hit distance for its penumbra
estimate. One nearby blocker could therefore shrink the filter width for all
four samples. R89 instead averages the hit distances of occluded rays only.
Misses are excluded, the no-blocker sentinel remains, and each ray still finds
its closest hit. The visibility fraction is unchanged.

This is a targeted hypothesis test, not a confirmed diagnosis. It cannot restore
missing casters, resolve light-selection changes, or replace shadows from other
lights. Mixed-depth blockers may become too soft; compare Jesse and the cabinet
in the same scene. Equal-depth blockers retain their previous estimate.

Four rays, emitter sampling, point shadows, composition, filters and shader
bindings remain unchanged. No new passes, buffers, or all-light runtime.

Close Control to install all files, enable RT Contact Shadows, and select
F10 > RT SPOTLIGHT. Check the fringe around Jesse and the cabinet, contact at
her feet, motion stability and FPS. Save one Lighting-R89 log.

Reference: NVIDIA PCSS uses average blocker depth for penumbra estimation.
Our change applies that principle to the existing ray-hit distance estimate;
it is not a complete PCSS implementation.
https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf
