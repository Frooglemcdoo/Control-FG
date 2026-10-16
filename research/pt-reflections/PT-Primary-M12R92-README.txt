M12R92 - shader-correct foliage cameras and secondary emitter preservation

R91's logged pending cameras all failed final validation (reason8); it did not
confirm the late-upload hypothesis. Two reviewed foliage PS variants do not read
ViewToWorld. R92 takes their camera matrix from the hash-reviewed matching VS
(root8), while retaining the pixel render extent from root12. Both actual buffers
are independently rechecked at submission. Other shader variants remain strict.

Secondary material emission is now read before unsupported BRDF rejection. A
verified front-facing emitting hit can contribute once through path throughput,
then terminate without inventing a scattering model. Unsupported primary
materials, non-finite values and back faces are still rejected. A WARP test runs
the production integrator for eight emission/admission cases.

This is NOT a claim that missing floor reflections or all baked lighting are
solved. These changes correct two verified omissions. Finite-area light sampling,
baked-only direct sources and complete secondary material coverage remain absent.
The fourth-hit diffuse volume cache is unchanged. More glossy samples alone did
not restore the user's reported missing reflections in M12R91.

Close Control and replace all package files. Keep current settings and do not
rerun PT-Enable. Collect-PT-Primary-M12R92.cmd records the resulting run.
