# R111: Temporal specular and room-light isolation

R110 restored RR and verified that specular 8/16 reached the producer. The user reports SPEC 16 cleans the problem completely, with eight the minimum usable level, but both cost too much. This build tests history reuse at lower ray counts. It does not claim equivalent quality or an FPS improvement before in-game results.

## Painting / doorway test

Install with Control closed. RR on, game Ray Tracing Samples Count **1**, saved GI Samples unchanged (six in the supplied log), FG and enhanced glass off. Keep the same scene, camera and resolution.

Compare **SPEC 2 → TEMP 2**, **SPEC 4 → TEMP 4**, then **SPEC 16** as the clean reference. Wait ten seconds after selecting, inspect the stationary image, then pan and move Jesse to check trails, boiling and delayed lighting. Note FPS. Return to SPEC 4 or SPEC 16 to disable temporal reuse. All five modes preserve centered filtered spotlight shadows, GI filtering, textured world albedo, skin/hair guides and 60% specular clamp.

TEMP modes keep zero spatial passes and use the native motion/depth history inputs with a maximum 60% history blend, native camera-cut rejection, depth rejection, offscreen rejection and nonfinite-history rejection. This is a bounded temporal experiment, not the old full SPEC FILTER mode. History is reset when entering temporal mode or changing its sample count. The normal shader remains byte-identical and has history compiled out. No GPU readback, new render-thread waits, per-click shader compilation or per-frame resource allocation is introduced by this diagnostic; the native history resources and filtering setup are reused.

## Room lighting test

The supplied screenshots change GI filtering and spotlight replacement together. They show a real illumination loss but do not establish fake/baked lights as its cause.

Compare **ROOM NONE → GI ONLY → SPOT ONLY → BOTH** from the same camera. These four modes all use SPEC **4**, the same saved GI samples and the same textured world/skin/hair guides. None uses temporal specular accumulation.

| Mode | Native GI filtering retained | Replacement spotlight shadows |
|---|---|---|
| ROOM NONE | No | No |
| GI ONLY | Yes | No |
| SPOT ONLY | No | Yes |
| BOTH | Yes | Yes |

These labels describe our two interventions, not turning off the scene's actual GI or all shadows. Native lighting and shadows remain available when replacement is off. If SPOT ONLY removes the light, prioritize the spotlight selection/visibility path. If GI ONLY does, prioritize the GI filter/history path. If only BOTH does, investigate their interaction.

Save one **Lighting-R111** log after both tests and report which room button loses the lighting and whether TEMP 2/4 approaches SPEC 16 quality at a usable FPS.

## Evidence and limits

`specular_sample_producer` records actual CPU producer counts; `specular_temporal` reports validated temporal dispatch and optional shader use; `lighting_policy` separates GI, spotlight, history and sample choices. GPU image quality still requires user observation. The R110 optional sample hook and its address-range correction are preserved; no startup dependency on the experimental hook is reintroduced.

The compiler verifies the original shader CRC, production resource slots/register spaces and relevant constant-buffer offsets. A WARP arithmetic harness executes the production temporal HLSL with only sampler spaces flattened for D3D11, checking valid history, camera cuts, depth changes, offscreen history, edges and oversized dispatches. Existing D3D12 WARP tests verify the spotlight route and unchanged guide behavior. The package retains all fourteen R97 shadow assets, three R97 glass assets, and the original 60% baseline shader.
