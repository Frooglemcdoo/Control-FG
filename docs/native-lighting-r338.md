# R338 temporal GI and visibility

Owned keeps a fresh, unshaded hit per 2-by-2 group and alternates between two frame-tagged history buffers. A candidate must come from the immediately preceding frame, project to a matching world-space receiver position and normal, and pass extent, sample-count and surface checks. A native alpha-tested visibility ray must hit the endpoint within 3 mm with a matching material fingerprint. Rejection preserves the fresh sample. Accepted hits use the current material, geometry and lights in the existing GI6 evaluation. No previous RGB is blended.

Half of pixels are eligible each frame. There is at most one additional validation ray per eligible pixel. Fresh rays and lighting evaluation run once. The history allocation is capped at 80 MiB per active GI state, with coarser storage at larger resolutions. Unsupported temporal state or resource creation falls back to current-frame GI. The compact capture includes the recorded temporal dispatch count. This is a recording counter, not a count of accepted GPU hits.

Install over R337. Launch normally with RT indirect light enabled. F10 switches between Owned and Native.
