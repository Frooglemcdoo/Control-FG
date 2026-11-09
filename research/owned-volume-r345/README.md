# R345 owned participating-media integration

R344 capture Volume-Light-20261105-213420-26836-0 completed with zero failures.
Its coefficient-filter replacement recorded 530/530 binds in the revised phase,
versus zero replacement binds in the original phases. User reported no visible
improvement; still frames did not establish useful improvement either.

Character shader 155 reads view-volume lighting at t8/t9, then separately reads
participating media at t10 and computes output = surface * media.a + media.rgb.
The exact media producer is deferredlight_volume variant 7 (SHA256 checked by
prepare-owned-volume.py). This is a candidate source, not a proven root cause.

R345 removes the R344 coefficient-filter runtime substitution. It substitutes
an owned HLSL media integrator at the exact native media PSO, using the original
root bindings, input textures, output UAV, grid dimensions and dispatch.
Native lights, atmosphere, grid producer jitter and density authorship remain.

The reference mode reconstructs the original shader's depth intervals, analytic
height-fog integral, fade curve, optical-depth addition, Beer-Lambert attenuation,
front-to-back radiance, and RGB-transmittance luminance packing. GPU tests compare
this reference to the original DXBC, including spatially varying inputs.

Owned mode integrates four half-cell XY-offset columns, bilinearly sampling light
and density. Each column maintains separate cumulative radiance/transmittance.
Only final integrated outputs are averaged. Density depth slices and the native
lighting sample depth mapping remain distinct and unchanged. There are no new
GPU resources, CPU waits, extra dispatches, frozen phases, or separate fog-history
resources. This can soften narrow features. The pass performs more sampling and
arithmetic; in-game performance is unknown.

A neutral diagnostic writes RGB=0, A=1 into the native media UAV. This suppresses
only that fog-composition contribution, not the separate surface-light volume.
TEST VOLUME automates native / neutral / owned / native-repeat with full-client
JPEGs, explicit PSO bind counters and native-setting preservation. Default owned
mode resumes after success/cancel; missing shaders fall back to native.

Next decision requires evidence from the neutral versus native phases. Dedicated
fog-history reprojection and increased grid allocation are NOT implemented here.
Do not describe this stage as a complete volumetric renderer or a confirmed fix.
