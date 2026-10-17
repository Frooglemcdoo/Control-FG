# R97: bounded cache-backed misses

R96 capture: no secondary-material termination records, but the environment
scale is zero. In pt_path_integrator.hlsli every miss previously evaluated only
PTPathEnvironment; the cache was requested only at the fourth surface vertex.
A ray leaving the finite, camera-culled scene thus had no baked-light closure.
This is a confirmed missing code path, not proof that it explains all floor or
texture symptoms. The eight R96 binding failures all identify foliage shader
68b09510..., parameter16 invalidated on a heap change; that remains separate.

Remedy/NVIDIA, Ray Tracing Gems II chapter46, sections46.1.3,46.2.1,46.2.3-4:
https://developer.download.nvidia.com/ray-tracing-gems/rtg2-chapter46-preprint.pdf
https://www.researchgate.net/publication/354065089_Ray_Tracing_in_Control
The native implementation bounds reflection rays and approximates miss radiance
using directional irradiance at the ray endpoint, converted to average radiance.
It documents approximation and light-leak limitations. The TLAS is not full-world.

Repository native evidence: research/native-integration-audit/reflection-000.ll.
%95 is GBuffer1.b; %134=1-%95. %348=6*sqrt(saturate(%134)); DXIL Exp is exp2.
%349=179.1999969482422/exp2(%348). %334..%336 multiply by 1.5+Frc65.
The new helper retains this equation. PT uses its own uniform RNG and extends
the range rule to every continuation (including the diffuse/specular mixture).
This policy is a finite-range approximation, not identical native sampling.
Primary visibility and direct-light shadow lengths stay unchanged. When GI
admission is absent, continuations retain the prior unbounded/cube behavior.

Miss request kind2 stores endpoint, outgoing ray direction, accumulated
throughput and original primary diffuse fraction. No extra hit material kd is
multiplied on a miss. The cube fallback occupies the three unused request words.
Kind1 retains fourth-hit throughput*kd. Both use the existing compute handoff,
resource ordering, buffer dimensions and replay admission. Valid cached zero
must remain authoritative; only unavailable/invalid coverage selects the cube.
The cache and cube are not summed. Cache E/pi conversion remains exactly once.

Sparse metrics bytes1184/1188 count requests and positive cached miss tails.
The CPU metric storage grows eight bytes; no image or DXR payload growth.
The collector includes MissGI records. Existing GI-positive count now includes
both terminal-hit and miss requests, explicitly marked in the log.

Validation: production-integrator WARP cases cover first/second continuation
misses, throughput, endpoint/direction and fallback payload, disabled GI,
terminal hit, invalid hit/extent, absorption and primary miss. Production compute
cases additionally cover cache-vs-cube selection, valid black cache, invalid
request tags/fallback, RGB/lobe weights and preserved guide words. Windows CI
also compiles the actual DXR and compute shaders and all prior regression gates.
Visual equivalence, NVIDIA execution, remaining light lists and texture flicker
still require in-game evidence. No settings reset or master update is required.
