# Full PT conversion — development checkpoint, not a user build

User requirement: all lights evaluated through PT, no baked or native lighting.
R102 was visually worse and its authored no-shadow policy is rejected.

Completed in this checkpoint:
- Every selected analytic light uses traced visibility again.
- Production shader no longer imports hit/miss/final-bounce baked GI.
- Removed post-trace GI compute dispatch and its PSO/sidecar dependency.
- Reflection-probe environment lighting is disabled; an unlit scene miss is black.
- DXIL validation rejects all GI request/cache and environment-probe bindings.

Not complete and not advertised as full PT:
- Emissive geometry contributes on BSDF hits but lacks explicit scene-wide light
  sampling and matching MIS. Tiny sources can remain effectively unsampled.
- Unsupported primary materials and correspondence failures retain native
  composition; transparent/specialized material models are not replaced.
- Scene completeness still depends on the captured native TLAS and light lists.
- The four-hit limit remains. There is no physical sky-source import.

Shipping package is deliberately disabled while these gaps remain. This avoids
asking the user to judge a darker intermediate state as a purported lighting fix.

Next implementation: establish a retained world-space emitter registry from
scene instances, geometry records and emissive material textures; sample emitter
surfaces with area-to-solid-angle PDFs; combine with BSDF-hit emission using MIS.
Then replace specialized primary/fallback shading with PT material handling and
verify that production composition cannot source native illumination when PT owns
a pixel. Ownership/synchronization failure must remain explicitly diagnosed.

Reference: https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
and https://pbr-book.org/4ed/Light_Sources/Area_Lights .
