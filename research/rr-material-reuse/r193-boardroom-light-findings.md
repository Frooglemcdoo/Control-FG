# Board Room missing-light capture after R193

User supplied lighting(9).log, manifest(9).txt, and three
00-shadow-blocker-{0,1,2}(4).png screenshots. Related comparison video:
20261028-1219-55.1356853.mp4. Rendering baseline remains accepted R193 long
reflection sequence, SPEC Native, compact hit-distance OFF; user reports
approximately 10 FPS higher than SPEC4 with no visible loss in tested scenes.

Capture is complete, zero failures, mode default_no_history_rr_off, restored
native. Three PNGs succeeded. Ray and composition capture both present 70705;
GPU completion validated; 4096 valid records, 3654 hits and 442 misses, no
invalid records. Full log decode and existing blocker/mesh analyzers succeed.
This is corrected RR-off shadow evidence, not direct Native/Blend paired GPU
capture. RR is deliberately off in these screenshots, not a user setup error.

All 3654 reference hits lie within the offline authored clip segment. No
past-clip hits were found. This does not prove every filtered ray correct,
but does not reproduce the earlier beyond-clip blocker problem.

Nine selected spotlight IDs appear in the 64x64 sampling grid; the light table
contains 113 records. Grid selection is not a census of active scene lights.
Light 22 is selected at 3718 samples. Its filtered spot visibility is exactly
zero at 3118 samples and positive at 600. Light 24 is selected at 161 samples,
154 zero. These lights are present, not missing from the captured table.

Light 22: 2065 reference hits hit instance_index=1604, geometry_index=0,
instance_id=0. It is the dominant captured blocker. Observed world hit bounds
are approximately [-95.7165,22.0142,31.7322] to [-90.7681,24.9550,31.8532]: a
thin spatial band, not sufficient to name its game asset or material. Screen
regions in input 2560x1440 coordinates:
- Left wall x[0,1100), y[260,800): 648 samples; 625 select light22; 498 hit1604.
- Left floor x[0,1100), y[800,1440): 756 samples, all select light22; 580 hit1604.
- Right wall x[1800,2560), y[250,900): 551 samples; 450 select light22; 308 hit1604.
These are descriptive grid counts, not fractions of lost image illumination.
IDs are local to this capture, never safe persistent exclusion keys.

Light22 and light24 flags=257 (0x101), native fade at byte200=1.0. The audited
Native Blend formula for PCF-only flags is 1+fade*(rt-1), so those lights reduce
to rt visibility. Fade alone cannot restore them. Light9 has fade~0.0108795;
not every captured light shares the same behavior.

Next focus: compare native shadow-map visibility/caster treatment with RT for
light22 and the dominant geometry. Potential material/alpha/caster-selection
mismatches remain hypotheses. Reference full-length hits are not the four-ray
filtered result; native visibility was not captured here. Do not blanket-disable
shadows, hard-code light/instance IDs, restore failed clamp experiments, or
claim this capture proves a particular mesh should not cast shadows.
No additional identical blocker capture is needed. No new fix shipped yet.
