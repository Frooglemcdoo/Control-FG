M12R100 - authored lighting at reflected diffuse hits

R99 capture has no binding failures or secondary termination faults. The next
correction targets reflected-hit illumination: authored GI previously entered
only at a miss or the fourth surface, after extra material attenuation. R100
can finish the diffuse indirect tail at the actual secondary hit, where Control
stores and samples its baked lighting. Primary rays retain both lobes.

The hit must lie inside an active authored GI volume. When the diffuse branch
is selected it emits one deferred cache request and stops that indirect tail.
The specular branch continues tracing with its selection probability compensated.
This keeps glossy multibounce transport up to the existing four-hit limit.
Direct lights and emission remain once; cache lighting is not added on top of a
second traced diffuse tail. Outside volume coverage or with cache disabled,
the existing path remains. Primary material admission and R96-R99 fixes remain.

This is cached path tracing with authored diffuse approximation, not an exact
unbiased full-scene simulation. Missing geometry, finite light-source highlights,
RR reflection motion/distance and remaining texture flicker are not declared
fixed. Cache sampling stays in the post-trace compute pass; no callable shader,
new image buffer, payload growth or extra ray is introduced.

Close Control and replace package files in the SAME installation. Keep settings;
do not rerun PT-Enable. Compare the affected floor and lit objects in reflections
with PT OFF/ON, then run Collect-PT-Primary-M12R100.cmd and send the results ZIP.
The new HitGI counters distinguish requests, positive cache and failed lookups.
