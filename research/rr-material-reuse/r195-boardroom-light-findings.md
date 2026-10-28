# R195 Native Blend capture validation

Inputs: Lighting-R195-20261028-082033-28940-0.log, lighting(10).log,
manifest(10).txt, and 00-shadow-blocker-{0,1,2}(5).png.

Capture completed with zero failures. Manifest preserves Native Blend,
RR requested, SPEC Native (choice 0), RT effects 0x6B and compact hit distance
OFF. All three PNG records report RR=1. Runtime sample producer reports native=1,
effective=1, requested=0. Long sequence replacement applied successfully in the
session log. RR success count advances, with zero recorded failures. The initial
snapshot RR-selected=0 precedes resumed gameplay; it does not describe the later
capture frames. No claim of independently measuring RR GPU completion is made.

Ray generation 327285 and composition generation 327287 both identify present
14063, extent 2560x1440. GPU completion is confirmed for the ray capture.
Existing strict decoder and authored-clip analyzer accept all 4096 records:
3784 hits, 312 misses, zero invalid records. All hits are within authored clip;
none are beyond it. These are full-length reference rays, not the individual
four filtered shadow rays.

Selected light 23 has 3772 grid samples, 3316 with filtered visibility exactly
zero; mean visibility 0.05450318. Light 25 has 122 samples, 117 zero. The table
contains 112 records; selected grid samples are not a census of all lights.
These lights are represented in the pipeline, rather than absent from its table.

Dominant blocker for light 23: instance index 1856, instance ID 0, geometry 0,
1740 hits. World hit bounds approximately [-95.7414,22.0337,31.7339] to
[-90.7670,24.9764,31.8785]. This spatial band closely overlaps the dominant
blocker in the earlier R193 capture, despite changed capture-local IDs and
camera placement. This is spatial corroboration, not proof of asset identity.

The accepted Native Blend baseline can now be captured with RR active.
The remaining missing-light investigation should compare native shadow-map
visibility/caster treatment against RT for matching pixels without changing
the displayed Native Blend rendering. This capture does not contain that native
map comparison and cannot establish which shadow result is correct. Do not
hard-code instance exclusions, disable alpha tests, or repeat this same capture
as a substitute for the missing comparison. No rendering change made here.
