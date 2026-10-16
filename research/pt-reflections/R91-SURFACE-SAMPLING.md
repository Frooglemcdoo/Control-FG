# R91: pending camera admission and primary sampling

R90 user log confirms all 24 companion PSOs require global samplers. The captured
7518cf8f native PS uses s6/space1 and s19/space1; it reads camera basis registers
6,7,8 and translation9. No sampler omission or camera-field relaxation is justified.
The early camera mismatch is an observation at CPU recording, not proof that the
final GPU-visible CBV is wrong. R91 retains a stable TLS candidate, requires the
same full camera for the producer, and compares the actual surface CBV against
that expected camera at submission. Wrong-view candidates remain native. This
addresses late upload completion if it is the cause; in-game CAMERA_RESOLVED logs
must establish how often that happens. It does not assert the timing hypothesis
has already been observed in the game.

Submission now also rejects a surface when the enclosing batch is not admitted.
All 64 combinations of the six consumer gates are checked in test-pt-native-view.
Pending camera tests reject stale and other-view bytes and accept finalized
matching bytes. Existing GPU consumer-guard tests remain required.

The unit-color primary diffuse convention changes kd to 1-F0 before continuation.
For F0=.04 the old mixture therefore picks specular only at the 5% floor. R91 uses
a 50/50 primary proposal with the corresponding mixed PDF; BRDF evaluation is
unchanged. Independent hemisphere quadrature checks six balanced-proposal cases
at two view angles and three roughnesses, in addition to the existing twelve
transport cases. This reduces glossy starvation; it is not a brightness boost.

No in-game visual validation exists for R91 yet. Baked-only direct lighting,
secondary material parity, emitter NEE and full RR guide correspondence remain
unresolved. Terminal volume GI is still a fourth-vertex approximation. A clean
Windows build must not be reported as proof that all visual issues are fixed.
