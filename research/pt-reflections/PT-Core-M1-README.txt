PT CORE M1 -- AUTOMATED COMPONENT BUILD

This package is NOT a Control installation or a playable PT build.
Do not copy the DXIL files into the game. There is no replacement dxgi.dll.

Implemented components:
- Shared diffuse/GGX path transport and compensated path termination.
- Private DXR surface-hit and alpha-aware shadow-visibility shader exports.
- Checked 16/32-bit index and packed normal/UV access without clamping invalid
  addresses into unrelated geometry. Invalid records invalidate the query.
- Direct-light evaluation with light-selection probability and power-heuristic
  MIS for non-delta lights. The caller still supplies the light sample.

Validation included:
- Three C++ test executables and their run logs.
- Compiled scene-hit library, ray-query fixture and transport compute fixture.
- DXC reflection dumps and SHA256 hashes.

These tests run automatically in the build. The shader libraries are compiled,
not executed on a GPU by this milestone. No user game test is requested.

Private scene-query ABI:
- Surface payload is 32 bytes: distance, float2 UV, float3 shading normal,
  uint material ID, uint status (0 miss, 1 valid hit, 2 invalid).
- Visibility payload is 4 bytes: 0 occluded, 1 visible, 2 invalid.
- Maximum configured payload size must be at least 32, attributes at least 8.
- NEW state object and rebuilt two-ray-type SBT are required. Ray contribution
  indices 0=surface, 1=visibility; geometry multiplier 2; miss indices 0 and 1.
- Preserve instance contribution offsets and the native opaque/alpha hit-group
  distinction when rebuilding records. Never graft these payloads into native
  hit groups. TLAS, geometry and records need independent admission first.
- Local root: index t0 space3, vertex t1 space3, ten DWORD constants b0 space3.
- Global material table t3; material texture array t0 space1; sampler s5 space1.
- Test raygen adds TLAS t5 and private test input/output t0/u0 space4.
- Alpha rule is base texture alpha times material word 16 >= 0.5. Only bind the
  alpha any-hit shader to the corresponding admitted alpha material groups.

Still required before Control integration:
Actual private state-object/SBT construction and GPU tests; geometric normals
and robust ray origin offsets (returned normal is shading normal only); primary
material sampling, admitted light selection, segment construction to each light,
emitter/environment accounting, per-frame RNG, multi-bounce dispatch scheduling,
output resources, composition adapter, denoiser guides and history management.
This module does not claim all Control material types are decoded or supported.
