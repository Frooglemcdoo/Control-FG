# R199 free-camera window captures — 2026-10-28

Input: user-provided Diagnosticswfreecam.zip. Four complete Native Blend shadow
captures, each with 4096 valid, strictly decoded records and three successful
PNGs. All manifests: RR requested/on, SPEC Native (choice 0), RT effects 0x6B,
hit distance OFF, end=complete, failures=0. No rendering-setting changes.

## Camera validation

PID 21820 captures are outside the windows, mode 0/generation 1. PID 80752
captures are inside the room, mode 1/generation 2. The second session reports
successful activation (result 2/Applied) and successful return to mode 0,
generation 3, with native player-input flag verification. No timeout or failed
camera switch was found. Camera movement is visible in the PNGs. Jesse was
not teleported by the tool. Both activation and restoration took roughly two
seconds to reach the hooked native update; do not claim per-render-frame cadence.

## Matched native-map/RT discrepancy

Threshold: native map visibility <0.1 and filtered RT visibility >0.9. Every
listed discrepancy also has a MISS on the full-length native reference ray.

| Capture suffix | View | Discrepancies | Both opaque queries miss | Mask-1 opaque hit | Wider-mask-only hit |
|---|---|---:|---:|---:|---:|
| 120647-21820-1 | Outside narrow window | 46 | 46 | 0 | 0 |
| 120707-21820-2 | Outside wide window | 132 | 132 | 0 | 0 |
| 120908-80752-0 | Inside, covered equipment | 1319 | 1318 | 1 | 0 |
| 120937-80752-1 | Inside, mannequins | 1157 | 1154 | 3 | 0 |

Total: 2654 discrepancies; 2650 remain misses even with force-opaque triangle
queries using mask 255. All 178 outside discrepancies remain misses. Four
inside samples hit with forced opacity using the normal mask. There are no
wider-mask-only hits among the discrepancies or among all reference misses.

All reference-miss classifications (not only discrepancies):
- Narrow outside: 112 both miss, 183 same-mask opaque hit.
- Wide outside: 253 both miss, 69 same-mask opaque hit.
- Covered-equipment inside: 3271 both miss, 81 same-mask opaque hit.
- Mannequins inside: 3213 both miss, 93 same-mask opaque hit.

Thus opaque probes demonstrably recover geometry elsewhere in these captures;
their failure at the outside discrepancy locations is not simply an empty
probe output. Strict decoding verified same ray/frame/pixel metadata for the
probe records, sparse completeness, and map/composition provenance.

## Physical light identity and limits

Outside light ID 24 and inside light ID 21 have the same nominal world emitter:
approximately (-94.2323303, 24.505907, 36.577942), allowing floating-point error.
All outside discrepancies and 2475/2476 inside discrepancies select this light.
One inside sample selects a different light (ID 20). IDs are capture-local.

Opposite mismatches (map >0.9, RT <0.1) number 21, 8, 146 and 58 respectively;
all have native reference hits. This remains a bidirectional caster mismatch,
not evidence that replacing every RT shadow with the map is globally correct.

The camera views and visible surfaces differ. Raw mismatch counts are not an
outside/inside quality comparison, and these are not repeated identical rays.
Moving the camera inside did not eliminate the mismatch for the physical light,
but it does not prove the same missing object stayed excluded in both runs.

This strongly deprioritizes opacity rejection or a simple nonzero instance-mask
exclusion as the cause of the outside leak. Geometry missing from the traced
scene, mask-zero instances, different native shadow-map caster representations,
procedural geometry, or ray/map path differences remain possibilities. Forced
opaque probes skip procedural geometry and still cannot hit mask-zero instances.
The data does not yet prove an out-of-bounds or player-streaming mechanism.

The full reference segment already misses, so merely extending the authored
shadow-ray endpoint cannot recover these sampled blockers. Increasing SPEC or
changing RR reconstruction does not address this demonstrated visibility mismatch.

## Next engineering target

Investigate RT scene inclusion / caster representation for the window room,
and retaining native-map occlusion where RT has no accepted blocker. Current
Native Blend composition applies the native PCF-only fade to RT visibility;
it does not preserve all native-map blockers. A map-based protection would be
a rendering fallback, not proof that the underlying RT scene inclusion is fixed.
Any such change must preserve light identity, authored fade and other shadow
layers and be measured for cost. Do not use blanket force-opaque or mask-255
queries as a gameplay replacement.

Analysis used the repository's decode-spot-hit-capture.py, map-probe analyzer,
window-blocker classifier and authored-clip classifier. A temporary str subclass
indexed full log lines to avoid repeated linear .index searches without changing
the decoder's ordering or record validation semantics. No source binaries or
user screenshots are added to the repository.
