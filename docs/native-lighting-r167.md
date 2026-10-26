# R167: Independent specular quality

R166 user captures completed without timing errors with RR active. Native and new lighting inadvertently used different specular requests (0 versus 4). Their measured shadow chains were 0.547 and 1.386 ms respectively; the overall cadence differed by about 3.48 ms. These separate runs do not isolate a causal spotlight-only delta.

R167 adds a shared SPEC button beside PERF CAPTURE. It cycles Native, 1, 2, 4, 8, 12, 16. Default 4 preserves corrected-lighting quality and now applies to the native comparison too. Native means no specular sample override, not zero rays. The selection lasts for the game session and does not change when selecting a lighting mode. Legacy explicitly named specular/temporal test modes retain their counts. RR eligibility still gates actual application. Existing frame-boundary handling resets specular history after a quality change.

Compare NATIVE and NATIVE BLEND with SPEC 4, RR enabled, FG disabled, identical graphics settings and a stationary matching view. PERF CAPTURE closes the overlay, waits five seconds after gameplay resumes, and records twenty seconds. Capture each mode separately. Optional SPEC Native comparisons must use that value for both modes; reducing samples can reintroduce noise and is not the proposed fix.

Capture refuses quality changes while running; its manifest now records the full measurement interval present count and present rate. Existing runtime.log includes RR GPU timing and CPU replay timing. Present cadence is not a GPU frame timer; individual GPU stages do not cover the full frame. Shadow timing combines spotlight and point dispatches. Dispatch dimensions are not ray counts. Reference context cannot identify other-room work. Context is optional for native mode. Capture timing uses sparse queries and asynchronous fence polling, with no rendering-thread file I/O or blocking GPU waits.

No shader, shadow selection, softness, clipping, history, RR, skin, glass, or ray-culling changes. No performance gain claimed pending matched captures. Windows CI validates policy, capture and actual proxy compilation. User visual verification remains required.
