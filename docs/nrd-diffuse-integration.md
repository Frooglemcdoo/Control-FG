# NRD diffuse integration checkpoint — 2026-11-08

Status: R384 wires RELAX_DIFFUSE into the native GI6 producer and temporal
filter boundary. The backend passes standalone Windows D3D12 WARP validation.
The full Windows build packages the embedded backend; live game quality and
RTX 5090 performance remain unverified.

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
  R384 enables this tracker without enabling scene/shadow inventory.
- Previous camera/jitter are selected inside Session from submitted history.
  Cancelled recordings restart with the current camera; callers cannot inject
  a stale previous camera into a continuous history.
- Read-only native input reader resolves current GI/motion/hit providers through
  the existing authenticated TLS API. It checks viewport mapping, frame/view,
  native state mode, resource dimensions and provider stability. It is compiled
  into the proxy and called synchronously at the native temporal boundary.
- Tests reject mismatched guides, invalid transforms, duplicate/pending frames,
  wrong queues; cancellation, frame gaps and view changes clear history.

## Evidence

Windows run 42446718211 passed actual receiver PSO creation against the audited
native root fixture plus the complete conversion/RELAX/remodulation pipeline.
Windows run 42446954107 additionally passed a shader dispatched immediately after
RELAX without rebinding: the restored PSO, root signature, constants and root UAV
produced the expected per-frame values. All 96 frames passed with a clean D3D12
debug layer. Native graphics-table preservation still needs live verification.

Windows run 42447203821 extended both GPU paths to 128 frames with a moving
foreground surface, nonzero motion and changing depth. Foreground target 0.8
produced 0.79880905; the worst just-exposed background was 0.09242249 against a
0.05 target (test bound 0.14). This is not proof of zero edge leakage or zero
in-game ghosting. Run 42447324564 also passed after submitted-camera ownership.

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

## Live R384 integration

- Exact GI6 bind/dispatch hooks mirror current descriptors and export the last
  matching producer guides. Guides are keyed to frame, view, actual producer
  PSO mode and command-list generation.
- The temporal boundary reads current raw GI, hit distance, motion and output
  providers; validates CPU camera against bound GPU matrices; records a complete
  RELAX transaction in the already-bound descriptor heap.
- Native spatial filtering is disabled for this path. Admission failure copies
  RAW rather than silently reverting to the old denoiser. F10 reports RELAX only
  when the transaction records successfully; otherwise it reports RAW FALLBACK.
- Queue submission advances history. Fence completion plus command-list Reset
  releases retained recordings. Abandoned work, size/root/producer/view changes
  invalidate history. Failed recorded work is retained through retirement.
- Injected dispatches temporarily suspend native single-dispatch accounting and
  snapshot tracking, then restore compute state and the original request.
- The existing panel, FPS, RAW comparison, clamp60 and on-demand captures remain.
  No legacy scene/shadow inventory is enabled. NRD is embedded in the proxy;
  its license accompanies the complete installable package.

## Required live validation

Compare projector energy, character jaw/neck, desk/plant detail and disocclusions
against RAW, then measure render-time cost with captures idle. Automated WARP
checks establish resource and signal contracts, not in-game image quality or
hardware performance. No game quality/performance result has been measured for
this path.
