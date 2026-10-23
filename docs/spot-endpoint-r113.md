# R113: Spotlight endpoint and filter isolation

R112 LIGHT OPEN restored the cafeteria's table/floor/Jesse illumination; LIGHT OFF removed it, and NATIVE PICK did not restore the expected lighting. The log recorded the expected composition routes with zero RR failures. The remaining question is whether excessive occlusion originates in the ray result or its filtering. Fixture geometry near a light is a hypothesis, not an identified blocker.

R113 adds two live room modes while retaining the exact R97 centered baseline. It does not claim a confirmed lighting fix or SPEC 16 performance improvement.

Close Control to install. RR on, native RT contact shadows on, game Ray Tracing Samples Count 1, saved GI samples unchanged, FG/glass enhancements off. Use the same cafeteria viewpoint, return to gameplay after selecting and allow ten seconds per mode.

| Button | Purpose |
|---|---|
| GI ONLY | Native spotlight shadows; bright reference |
| BOTH | Existing centered rays with spotlight filtering; dark reference |
| RAW | Same centered rays and light selection, bypassing spotlight temporal/spatial filtering |
| END GUARD | Same centered rays and filtering, with a bounded exclusion at the light endpoint |
| LIGHT OPEN | Bypass only selected spotlight visibility; intentionally removes its shadow |

All five room modes retain GI filtering, SPEC 4, saved GI sample count, and the same material guides. RAW changes spotlight filtering only: native point filtering is preserved. SPEC 8 / SPEC 16 remain separate quality references. NATIVE PICK and LIGHT OFF remain available as R112 controls.

END GUARD shortens the ray by the smaller of one native emitter radius and 10% of the receiver-to-light segment. It keeps the receiver-side biased origin, ray direction, four centered samples, closest-hit behavior and native hit/miss handlers. Its miss payload, TraceRay TMax and hit classification use the same shortened endpoint. Zero/negative/NaN radius adds no guard; oversized radius is capped. It deliberately ignores possible blockers in that final interval, so inspect objects close to fixtures for newly missing shadows or light leaks. This is a bounded test, not a blanket mesh exclusion or verified fixture fix.

If RAW restores the brightness, investigate spotlight filtering. If RAW stays dark but END GUARD restores it, the endpoint interval is implicated; this does not identify which mesh caused the hit. If both remain dark, retain that negative result and investigate the earlier ray segment rather than increasing the exclusion blindly.

Save one Lighting-R113 log after the comparisons. Capture BOTH, RAW and END GUARD at the same viewpoint, plus whether the chair/table/Jesse shadows remain intact. `rt_spot_ray_route` records the actual ray record offset (END GUARD=128) and raw-filter request; `rt_spot_endpoint` records shader readiness. These are CPU command-recording observations, not GPU image proof.

Validation: the new shader is derived from hash-locked R97 bytecode using `tools/rt_spot_endpoint.py --generate --dxc ... --assembler ... --dxv ...`; DXIL assembler and validator passed. Reversing the four endpoint transformations reproduces the complete original disassembly. Numerical tests cover zero and extreme radii, the 10% cap, terminal exclusion, retained near/mid-segment occluders and miss classification. CI checks shared policy, optional readiness, PSO routing and ray-record bounds. Optional endpoint state creation failure retries the existing augmented state and leaves END GUARD unavailable. All fourteen existing spotlight sidecars, canonical glass assets and baseline RR clamp remain unchanged. Runtime ray behavior still requires testing on the user's GPU; WARP checks do not establish visual correctness or hardware DXR success.
