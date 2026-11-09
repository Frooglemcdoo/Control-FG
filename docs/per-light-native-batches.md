# Native spotlight batches and geometry

Implemented as independent GPU adapter stages; not connected to live rendering.
The Windows WARP fixtures execute batching and batch composition in sequence.
The hardware DXR fixture previously passed on the user's RTX 5090. Neither result
proves native TLAS coverage or absence of the in-game shadow defects.

## Verified native inputs

The captured producer is hash
`a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f`.
The eventual runtime must identify this producer before accepting its bindings.
Its t16 is the spotlight cell texture; t15 is the 320-byte spotlight buffer.
SHEX DWORDs 430, 4444–4470, 5685–5712 and 10533–10564 establish:

- Pixel tiles are 64 by 64 pixels.
- Cell texture X = tileX * g_uMaxSpotLightsPerTile + candidate.
- Cell texture Y = tileY * g_uDepthTileCount + resolved depth slice.
- Candidate IDs occupy the texture's x channel; 65535 terminates a list.
- All entries are evaluated before the separate strongest-light selection.
- Depth is reconstructed with sys_constants.g_mClipToView.
- If view Z <= g_vTileDepthNearFar.x, slice is zero. Otherwise slice is
  min(ceil((resolve.y-ln(viewZ))*resolve.x), g_vTileWidthHeightDepth.z-1).

`rt_lights_native_geometry.hlsli` centralizes native coordinate transforms,
packed-normal decoding and the emitter basis. Matrix offsets and normal packing
were checked against the captured contact-shadow DXIL. Its actual emitter is
vWedPositionInView at byte 16, while byte 0 is the virtual cone position. The
emitter radius is byte 72. The new tasks target the actual emitter in world space;
they do not subtract fNearDistance from the receiver-to-emitter reach.

This is a geometry contract, not a renewed theory that RT sample count caused
the old blotch. The user confirmed the blotch was already in the original R71.

## Batch contract

`rt_lights_native_batch.hlsl` binds b0=native sys_constants,
b1=native deferredlight_constants, b2=batch controls;
t0=GBuffer1, t1=clip depth, t2=spot cells, t3=spotlight records;
u0=work atlas, u1=ShadowTask, u2=ReceiverRange.

Its twelve control DWORDs are firstReceiver, receiverCount, slotsPerReceiver,
frameTag, sampleCount, instanceMask, originBias, endpointBias, workWidth,
workHeight, emitterScale and reserved. Float controls retain float bit patterns.
The test uses 32 samples to exercise the requested high-sample path; this is not
a chosen live performance default. Bias and radius scale are explicit policy
inputs, not embedded in individual task writes.

One thread processes one receiver. It validates the entire native candidate list
before publishing a valid range. Fixed slots in this correctness/reference stage
avoid atomics and CPU count readback. Entries preserve native list order. A batch
can begin at any linear receiver index. Local output indices remain separate from
global receiver identities.

Unused reserved slots are cleared. An invalid receiver range has first=UINT_MAX,
count=1, so the composer rejects it. Valid empty lists have count=0. Missing
sentinels, invalid IDs, insufficient slot capacity, mismatched dimensions and
incorrect screen inverse resolution do not become partial replacement lighting.
If a later geometry check fails after some writes, the range remains invalid;
those tasks may execute but their partial lighting must never be committed.

The fixed-slot entry remains a reference fixture. The `RT_LIGHTS_COMPACT=1`
entry reserves only the complete receiver's actual candidate count through a
bounded compare/exchange counter at u3. Reset that counter on the GPU before each
batch. Overflow leaves the receiver invalid; reservations cannot wrap, overlap,
or exceed either the task buffer or work atlas. Reserved tasks are initialized so
a later geometry rejection cannot trace stale work. No CPU count readback is used.

`rt_lights_native_arguments.hlsl` consumes that counter after a UAV barrier. It
preserves the first 88 bytes of a submission-owned native ray-table template,
writes ray dimensions at byte 88, native 8x8 extraction dispatch arguments at
112, and packing arguments at 124. The buffer is 136 bytes. Transition it to
INDIRECT_ARGUMENT before ExecuteIndirect. The extraction atlas dimensions must
be multiples of eight; the shader disables only the final partial group rows.
A zero count or invalid capacity produces zero work. These argument bytes and
padding are executed/checked on WARP, including poisoned prior atlas contents.
Actual native ExecuteIndirect ray dispatch remains runtime integration work.

`rt_lights_compose.hlsl` now has `batchMain` alongside the original test entry.
Its b0 is firstReceiver, receiverCount, frameTag, reserved. It checks global
receiver IDs, external frame identity (including empty ranges), complete ranges,
finite input/output and per-light visibility. Output remains local to the batch.

## Native ray binding compatibility

Captured global root SHA256:
`451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3`.
The R65 log associates this root with the contact-shadow state object.
Root parameter 0 contains b4..8, t0..35 and u0..7, in that order. Parameters 2..5
are b0..3. Parameters 6 and 7 hold space-1 samplers and material textures.

Compiling `rt_lights_visibility.hlsl` with RT_LIGHTS_NATIVE_BINDINGS=1 uses t6 for
the native TLAS, t8 for tasks, u2 for visibility and b4 for hit-table controls.
These are covered by the captured root. In root table 0, their offsets are
TLAS=11, tasks=13, visibility=43 and controls=0. Existing native contact hit/miss
payload and local material bindings must remain intact. The captured native
TraceRay uses hit offset 0, geometry stride 2, miss index 0 and instance mask 1.
Do not copy the standalone fixture's geometry stride 1 into the game adapter.

Windows deserializes the captured root, checks the required descriptor ranges,
and creates the extraction compute PSO against that root. The native-binding
ray library compiles successfully. This does not validate native hit-table linking
or dispatch; those remain runtime work. DXC's standalone root-verification command
failed for both the captured native DXIL library and the replacement library, so
that command is not treated as evidence of DXR state-object compatibility.

## Executed checks

The WARP batch fixture starts at receiver 62 and crosses the tile boundary at 64.
It covers two lights on a receiver, a one-light neighboring tile, an empty depth
slice, insufficient capacity, an unterminated list, an out-of-range light,
incorrect inverse resolution and stale composition. It checks translated/rotated
world positions, the physical emitter field, packed normals, orthogonal emitter
axes, sample/mask/bias policy and complete batch composition.

Remaining: live descriptor/CBV forwarding and queue-fenced resources, connecting
compact scheduling and native DXR dispatch, native hit-table/geometry verification,
independent per-light filtering, and wiring the private lighting replay/commit.
The shaders described here are not yet invoked by the game runtime.
Native light indices are frame identities only until cross-frame stability is
proven. Do not use them alone as temporal history keys.

## Private base replay and paired lighting commit

`tools/rt_lights_native_base.py` creates two hash-locked replay shaders. The
producer variant inserts an unconditional break at the start of the spotlight
loop (SHEX 5685), after its accumulators have been initialized. The selected-light
variant clears only the spotlight guard r5.z after SHEX 362, preserving the point
guard and original point identity. Both pipelines successfully compile against
the captured global root on WARP. Their non-spotlight output equivalence still
needs a native-input execution check; pipeline creation alone does not prove it.

The live adapter must replay these into PRIVATE diffuse/specular outputs. The
producer's selection UAV must also be private, while the selected-point replay
must READ the ORIGINAL native selection texture. Neither replay shader is a
drop-in replacement for the original game pass. Original dispatches run normally
and leave a complete fallback image.

`rt_lights_native_commit.hlsl` writes private base + independent RT spotlight
lighting only for complete valid receivers. It validates diffuse and specular
together before either store, preserves base alpha, rejects nonfinite values and
half-float overflow, and leaves both original outputs unchanged on rejection.
WARP tests cover nonzero batch origins, out-of-batch pixels, invalid visibility
composition, NaN/infinity, overflow, and avoiding double-addition of native light.

Renderer disassembly confirms producer and selected-light work are within the
existing 0x23ed10 pass scope: producer shader selection occurs at 0x23f757 and the
native ray setup follows at 0x23f8cc. This supplies a candidate recording scope;
it does not establish live descriptor snapshot completeness or queue lifetime.

Executed Windows validation for compact batches, argument generation, native
replay PSO compatibility, extraction, and paired commit:
https://github.com/Frooglemcdoo/Control-FG/actions/runs/39695423333

R80 now connects this backend to the live native pass, descriptor snapshots and
queue-fenced resource pool. The main workflow packages the integrated runtime.
See `all-spotlight-r80.md` for installation, coverage and in-game validation limits.
