# R112: Selected spotlight light-loss isolation

R111 results: ROOM NONE and GI ONLY retain the cafeteria's table/floor illumination; SPOT ONLY and BOTH remove much of it. The log confirms optional temporal dispatches but the user reports TEMP 2/4 do not approach SPEC 16. Zero RR failures were recorded. These results localize the room regression to spotlight replacement, but do not distinguish occlusion from selection or composition.

R112 keeps the working centered ray/filter path and adds focused live diagnostics. It does not claim to fix the missing light or reduce SPEC 16 cost. TEMP buttons are removed from this test panel; SPEC 8 and SPEC 16 remain quality references.

Close Control to install. RR on, native RT contact shadows on, game RT sample count 1, saved GI samples unchanged, FG/glass enhancements off. Load the same cafeteria viewpoint. Press F10 to switch, return to gameplay, and let each settle for ten seconds.

| Button | Selected light ranking | Selected spotlight contribution | GI filter | Specular samples |
|---|---|---|---|---|
| GI ONLY | Native | Native shadows | On | 4 |
| BOTH | Unshadowed contribution | Centered filtered RT | On | 4 |
| NATIVE PICK | Native | Centered filtered RT | On | 4 |
| LIGHT OPEN | Unshadowed contribution | Selected RT visibility forced to 1 | On | 4 |
| LIGHT OFF | Unshadowed contribution | Selected diffuse/specular additions omitted | On | 4 |

LIGHT OPEN preserves material response, light falloff, cookie, the nonselected light accumulation and the point-light branch. Only the two final visibility operands change. It intentionally removes the selected spotlight's shadow, not all shadows. LIGHT OFF removes the selected light's contribution, not all direct lighting. NATIVE PICK retains our replacement shadows but restores native candidate ranking; it is an isolation mode, not a recommended permanent fix.

Compare GI ONLY -> BOTH -> LIGHT OPEN -> LIGHT OFF -> NATIVE PICK. If LIGHT OPEN recovers the missing illumination, investigate the ray visibility/filtering path, including emitter geometry occlusion. If it stays missing, investigate selected-light shading and composition. If NATIVE PICK recovers it, investigate which light our ranking selects and how that selection interacts with visibility. These are conditional interpretations; screenshots alone cannot identify an occluding mesh.

ROOM NONE and SPOT ONLY remain as R111 controls. Save one Lighting-R112 log and screenshots for the five primary modes. `spot_light_isolation` reports the actual selected composition PSO route; `rt_spot_selection` records producer ranking. Both report CPU binding/recording, not GPU pixel proof. A missing optional LIGHT OPEN PSO disables that mode without disabling baseline RR or the existing spotlight route.

Validation: hash-locked transform permits only four DWORD operand changes in the existing composition shader; disassembly and D3D12 WARP PSO validation; live PSO route and selector fallback checks; shared setting/readiness tests. All fourteen original shadow sidecars and three canonical glass assets are preserved byte-for-byte. No new per-frame allocations, GPU readbacks or waits are introduced.
