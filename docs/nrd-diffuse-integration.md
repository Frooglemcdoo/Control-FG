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
- Complete prepare → RELAX → remodulation transaction, including native input
  state restoration and all-before-recording descriptor/resource allocation.
- Fresh variable-size native descriptor arena allocation rejects heap rollover,
  material-prefix overlap, native-table overlap and mismatched CPU/GPU handles.
  The live transaction never changes descriptor heaps, preserving graphics tables.
- Native receiver GPU owner creates private world-normal, throughput and view-Z
  exports against the captured native root. It mirrors only exact GI6 bindings.
- Native compute transaction restores root arguments and PSO even on exceptions.
  Its state tracker can be enabled independently of legacy scene/shadow inventory;
  this opt-in is not enabled in the live game yet.
- Tests reject mismatched guides, invalid transforms, duplicate/pending frames,
  wrong queues; cancellation, frame gaps and view changes clear history.

## Evidence

Windows run 42446718211 passed actual receiver PSO creation against the audited
native root fixture plus the complete conversion/RELAX/remodulation pipeline.
Windows run 42446954107 additionally passed a shader dispatched immediately after
RELAX without rebinding: the restored PSO, root signature, constants and root UAV
produced the expected per-frame values. All 96 frames passed with a clean D3D12
debug layer. Native graphics-table preservation still needs live verification.

Full-pipeline synthetic measurements: uniform target 0.4 → 0.39942307; persistent
light target 0.8 → 0.79830933; removed light target 0.01 → 0.00997646. Sparse moving
outliers did not repeatedly reset the independent history. These WARP tests are
integration checks, not in-game quality or NVIDIA hardware cost measurements.

`tools/nrd/audit_native_inputs.py` verifies the exact native temporal bytecode:
`g_tGeometryVelocityTexture` is t2/space0, sampled with s6/space1 at pixel centers
scaled by sys[1].zw; native previous UV = current UV + sampled motion. Live
admission must verify the current resource and viewport convention, not reuse a
later RR resource. Existing stationary RR captures show zero motion despite
changing jitter, but do not establish earlier boundary resource identity.

Native jitter translation is already audited in `validation/jitter-native`:
renderer INSTANCE +0x480/+0x488 contains clip shifts. NRD sample jitter is
(-width*dx/2, +height*dy/2), opposite NGX raster jitter. Endpoint tolerance is
clamped to NRD's exact [-0.5,0.5] contract. No new capture is needed for this ABI.

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

1. Connect the implemented private receiver export to the exact GI6 bind/dispatch
   hooks. GI6 may dispatch more than once before filtering; keep the last matching
   producer's guides with that frame, view, queue and command-list generation.
   The existing "weight" globals 0x1297478 / 0x12974a0 are actually g_rwtHitInfo /
   g_tHitInfo, both referring to native IDHitInfo (raytracing-passes.asm). Acquire
   this native first-hit-distance texture directly with its tracked state.
2. Acquire current native t2 motion at the temporal boundary and validate
   sys[1].zw against the GI viewport. Use the audited jitter conversion and
   previous SUBMITTED camera, not the last prepared or cancelled camera.
3. Wire the tested NativeTransaction to that boundary with a complete compute
   snapshot and fresh arena in the already-bound heap. Disable native spatial
   history for
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
