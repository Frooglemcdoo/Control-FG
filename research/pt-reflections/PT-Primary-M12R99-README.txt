M12R99 - reflected-surface mesh normals

R98 capture confirms sampler replay is running without logged binding failures.
R99 corrects a separate code defect: secondary hits already calculate transformed,
interpolated mesh normals, but lighting previously discarded them and used flat
triangle normals. Secondary lighting, continuation sampling and terminal GI now
use the mesh normal when valid and facing the viewer. Back-face normals follow
the oriented triangle; invalid or silhouette-incompatible normals retain the
geometric frame. Primary surfaces keep their existing raster G-buffer normals.

Ray origins still use the geometric normal and existing precision-safe offset.
Direct lighting and continuation rays are rejected below the geometric surface;
rejected samples are not resampled or brightened. No new DXR payload, buffers,
descriptors, ray count or RR input is added. R96-R98 corrections remain.

Tests cover retained interpolation, back faces, invalid normals, silhouette
fallback and GPU integration of tilted surfaces with blocked back-hemisphere
lighting. This is a targeted shading correction, not confirmation that missing
floor reflections, lighting or texture flicker are fully resolved. Mesh normal
maps, scene coverage and RR reflection motion/distance remain separate issues.

Close Control and replace package files in the SAME installation. Keep settings;
do not rerun PT-Enable. Compare the affected floor from the same camera with PT
OFF and ON, then run Collect-PT-Primary-M12R99.cmd and send the results ZIP.
