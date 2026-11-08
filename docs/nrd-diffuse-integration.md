# NRD diffuse integration checkpoint — 2026-11-08

Status: dedicated RELAX_DIFFUSE backend passes standalone Windows D3D12 WARP
validation. It is not connected to the game. There is no new installable build
claiming an NRD quality fix. The existing build workflow still builds R383.

## Implemented

- Pinned NVIDIA NRD 4.18.0 at c862a54f4ed845378eeebd832491eebdc203c537.
- Full NRD permanent/transient history ownership, immutable dispatch snapshots,
  explicit resources/barriers, retained GPU objects and submission-controlled
  frame advancement. Abandoned recordings invalidate the backend.
- Native camera conversion checks affine inverses and projection composition
  before converting double matrices to NRD's float, column-vector convention.
- Receiver shader uses the exact existing GI6 receiver decoder. It exports
  current world normal/roughness, reconstructed view Z and receiver throughput.
- Prepare/finish shaders divide/multiply the same receiver throughput. They
  preserve native data on rejected input and preserve the temporal output ABI.
- Tests reject mismatched guides, invalid transforms, duplicate/pending frames,
  wrong queues; cancellation, frame gaps and view changes clear history.

## Evidence

Windows run 42444378059 passed actual NRD compilation, CPU contracts and a full
D3D12 RELAX dispatch chain preserving uniform light.

Windows run 42444463780 additionally passed persistent local illumination,
moving sparse outliers and removal of illumination. These synthetic tests are
integration checks, not evidence of in-game quality or NVIDIA hardware cost.
All three conversion shaders also compile locally with DXC, warnings as errors.
The generated native receiver decoder requires HLSL 2018 semantics.

## Native signal semantics

`gi_owned_lighting.hlsl` computes `throughput = asfloat(receiver.r3.xyz)` and
accumulates `throughput * secondary radiance`, followed by native depth fade
and division by ray count. Native GI6 dwords 907–937 compute this receiver
factor from F0 and EnvBRDF. It is not RR diffuse albedo. Using that albedo as a
substitute would change the signal and risks loss of shading/lighting energy.

The receiver export reuses `GIInitializeReceiver`, not a second independently
rewritten material decoder. Its r7 contains world normal. Its r2 retains the
homogeneous reconstructed view position with reciprocal W. No secondary
evaluation runs in the receiver export. Sky/invalid receivers remain rejected.

## Remaining live integration

1. Allocate/bind private receiver exports at the exact GI6 bindings, alongside
   the same frame's raw GI and native u1 mean first-hit-distance output.
2. Verify native motion-vector jitter convention and bind those vectors at this
   boundary. RR's later guide output cannot be reused as an earlier-frame guide.
3. Record prepare, RELAX and finish as one transaction; restore the engine's
   root, heaps, constants and PSO. Disable the native spatial history path for
   that successfully admitted transaction, without double filtering.
4. Connect queue submission/reset retirement and resize/view/producer changes.
   Never advertise RELAX as active if admission or resource acquisition fails.
5. Link NRD and embed conversion shaders in the actual mod build. Keep existing
   F10 UI, FPS, RAW comparison, clamp60 and on-demand capture controls. Do not
   enable scene/shadow inventory monitoring.
6. Validate projector energy, character jaw/neck and desk/plant temporal detail
   against RAW, then measure render-time cost with captures idle.

Do not ship the standalone validation executable or repackage R383 as the new
denoiser. No game quality/performance result has been measured for this path.
