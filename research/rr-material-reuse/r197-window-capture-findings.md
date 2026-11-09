# R197 window map/RT comparison — Diagnostics1(1).zip

Three complete captures, zero failures, 4096 valid map/RT pairs each. Native
Blend, RR requested and observed on all PNGs, SPEC Native, RT effects 0x6B,
compact hit distance OFF. Warmup skips the first pass and captures the next;
map, ray and composition presents match (8207, 10166, 4876 respectively).
The R197 startup fix is validated by these in-game captures.

| Capture folder suffix | View | Map blocked / RT lit | Map lit / RT blocked |
|---|---|---:|---:|
| 110540-9016-0 | Wide window | 164 | 6 |
| 110608-9016-1 | Narrow window | 70 | 15 |
| 110806-16540-0 | Hallway | 36 | 27 |

Thresholds: blocked <0.1 visibility, lit >0.9. These are grid sample counts,
not percentages of image illumination. Strict existing decoder validates all
4096 payloads in each capture, including exact selected-light identity.

All 270 map-blocked/RT-lit samples have MISS results for the full-length
reference ray too. Of these, 14/8/34 reference segments respectively would be
shortened by the authored clipping plane; the unshortened segment still misses.
Thus merely restoring the extra segment past that plane does not recover an
accepted blocker for these center rays. Reference endpoint matches the nominal
emitter to floating-point tolerance in inspected examples. Reference rays use
mask 1 and preserve the native alpha any-hit shader; misses do not prove absent
triangles. Mask exclusion, material/alpha rejection, and differing shadow-map
caster geometry remain alternatives to culling or bounds exclusion.

First view: 161 of 164 leak-direction samples select light24; 3 select light26.
Second view: all 70 select light24, screen pixels x940..1340, y483..1181 in
2560x1440 input coordinates. Third view distributes 36 samples across seven
lights, including light23 (10). Light24 in first/second and light23 in third
have nominal emitter approximately [-94.23233,24.50591,36.57794]; IDs are
capture-local and must never become hard-coded exclusions.

Full reference hits: 3682 / 3904 / 3763; misses: 414 / 192 / 333. Every hit
classifies within authored clip. All 6/15/27 opposite-direction discrepancies
have within-clip reference hits. Both mismatch directions exist, so this does
not by itself explain every earlier missing-light report.

User's behind-window blocker hypothesis is supported as a map/RT caster
mismatch, but literal out-of-bounds culling is not established. Next investigation:
RT instance inclusion/mask and native alpha/caster treatment for rays where the
map blocks but full-length RT misses. Do not repeat these captures or change
SPEC/clamp/RR to investigate this result. No rendering fix shipped from analysis.

Reproduction: tools/analyze-map-visibility-probe.py and
 tools/analyze-shadow-blocker.py against each folder's lighting.log.
All results above were decoded from the uploaded logs, not inferred from PNGs.
