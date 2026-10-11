# PT Reflections P3 — Native-preserving redesign

**Status:** Architecture reset / implementation gate. **No new PT binary has been built on this branch.**  
**Base:** stable Control-FG `v2.1.2` (`3d639bd3d08b0dca6d43a9a767ec32da3c080034`).  
**Prior work retained as source reference:** `experiment/pt-reflections-p0`, `experiment/pt-two-layer-bounce-proof`, `experiment/pt-secondary-normal-proof-v7`.

## Why P3 is necessary — A/B test, October 11, 2026

User reproduced severe frame-to-frame reflection flicker with the experimental P2 two-layer shader and explicitly confirmed the flicker **stopped after physically removing the P1/P2 ray sidecars**, even with RR and 2x FG enabled.

The decisive compact-log capture is `Control-FG-Compact-Logs-20261011-084223-1315637c.zip`, which includes both runs:

| Capture | P2 shader | P2 injection | Returned raygen | Post-composite | RR/FG |
| --- | --- | --- | --- | --- | --- |
| `probe-20261011-133446-112-51232.log` | `PT_REFLECTION_P2_SIDECAR loaded=1` | `enabled=1` | `p2_injected=1` | `PT_REFLECTION_P2_POST_DISPATCH` present | RR off, FG off |
| `probe-20261011-133856-436-62056.log` | `PT_REFLECTION_P2_SIDECAR loaded=0` | `enabled=0` | `p2_injected=0`, `native_result_preserved=1` | No P2 post dispatch | RR evaluations succeed, FG 2x used |

The user's clean visual comparison corresponds to the **second run**, not the first. The A/B isolate identifies the *P2 injection path as a whole*, not yet whether the ray replacement, post-composite, or their interaction individually caused the flicker.

### Source-level defects of P2

1. Control's original reflection code expects **two first-bounce hit samples**. P2 temporarily repurposed native layer 1 for a secondary hit (`src/shaders/pt_reflection_p2.hlsl`), reducing first-bounce coverage to one and destabilizing/altering the native composite.
2. An earlier P2 wrote secondary hits to layers **2 and 3** even though the captured native texture arrays have **two layers**. Shader dispatch success did not prove valid writes.
3. Bitcasting integer IDs/UVs into `RGBA16_FLOAT` inputs cannot reliably preserve arbitrary raw 32-bit bit patterns; numeric packing was a temporary workaround, not an adequate storage architecture.
4. The extra P2 compute composite depended on engine-private binding state after the native pass. The earlier custom GI dispatch caused a freeze after the first successful dispatch, probably because it replaced compute bindings without restoring the full native root argument state; that requires independent validation.
5. The old bounce-2 radiance path was still primarily environment/emissive and did not correctly evaluate native local-light selection and occlusion. Reflections looking too similar to native Control were **not** evidence that brightness needed to be multiplied.

**Do not ship or re-enable the old two-layer P2 ray replacement as a fix for flicker.**

## Non-negotiable P3 invariants

1. Preserve both native first-bounce reflection rays, their shader record, their two hit-texture layers, their temporal inputs, and the native deferred-reflection composite.
2. No P3 writes to Control's `g_rwtMaterialId`, `g_rwtNormal_TexcoordX`, `g_rwtPosition_TexcoordY`, or `g_rwtShadow` from an injected raygen.
3. Do not replace Control's native `reflectionRayGeneration` shader, mutate native PSOs, or hijack a native hit layer to hold a secondary hit. P3 must be an **additional** pipeline, not a replacement.
4. Own all P3 scratch allocations and descriptor ranges. Secondary material ID storage must use an actual `R32_UINT` UAV/SRV; position and normal must use explicit float formats. Use a separate validity mask and bounded indices; no float-bitcast material sentinel.
5. Don't leave Control's recording command list with altered descriptor heaps, root signatures, root arguments, or PSOs. Prefer a P3-owned command list and explicit queue sequencing/fences, with engine resource lifetime and barriers validated at each boundary.
6. Changing resolution, HDR, RR, FG, loading a level, disabling P3, and exiting the game must release/rebuild P3 resources deterministically. Native presentation and shader selection must remain unchanged when P3 is off.
7. Experimental P3 is **off by default**, with independent toggles for tracing and compositing. RR and FG are unrelated controls, never prerequisites for proving native-pass stability.

## Architecture stages / acceptance gates

### P3-A — native baseline observer (no PT GPU work)

Build on v2.1.2 with native reflection pass unchanged. Capture native reflection texture shape/formats, TLAS access, command queue/list context, and submission boundary with CPU-only telemetry.

**Acceptance:** With P3 enabled in observe-only mode, the native reflection raygen and deferred composite identities are unchanged; two native hit layers intact; no extra GPU dispatch or resource write; no flicker regression versus the cleaned A/B baseline.

### P3-B — owned scratch and synchronization (no rays)

Allocate P3-owned scratch textures, e.g. `R32G32B32A32_FLOAT` position, `R16G16B16A16_FLOAT` normal, `R32_UINT` material ID, `R8_UINT` validity, and `R16G16B16A16_FLOAT` radiance at an explicitly defined resolution. Use proper UAV state barriers, descriptor lifetime, and fences.

Record a deterministic marker into P3-owned storage. Only preview/copy that marker in **explicit diagnostic mode** after state and lifetime checks. Do not alter the native reflector's two layers.

**Acceptance:** No DXGI device removal/hang; no reflection flicker; no divergence with P3 preview disabled; scratch checker survives to diagnostic presentation when enabled.

### P3-C — secondary-ray pass (still no additive lighting)

After native reflection data is available and its state is known, dispatch a separate P3 DXR pass using the *native two first-hit samples as inputs*, reusing a valid TLAS reference but writing only to P3-owned scratch. Keep per-pixel, per-path sample/validity counters; independent SRV/UAV root/heap layout. Do not use an unproven native shader-record clone.

**Acceptance:** secondary-hit normal/position/material and validity masks are measurable and stable during motion, with no out-of-bounds writes; native-first two paths visually unchanged. Static scene movement test and stop/start test pass.

### P3-D — actual bounced radiance and native-light visibility

Compute P3 radiance from hit albedo, emissive sources, environment lighting, native point/spot counts and **verified** light-cell indexing, correct view/world transforms, attenuation, and actual shadow visibility. Keep raw source, range, N·L, cone, and shadow visibility independent in diagnostics. Do not introduce light through concrete walls.

**Acceptance:** bounce-only radiance is measurably nonzero for representative lighting scenes, and remains bounded without NaNs/flicker. Demonstrate physically plausible visibility on both matte and reflective surfaces.

### P3-E — final composite and RR/GI integration

Accumulate **only P3 bounce radiance** (not the native already-composited first bounce) into appropriately chosen reflection and diffuse GI targets with correct lifetime, barriers and ordering, without replacing or corrupting Control's command-list bindings. RR consumes updated radiance only after a validated handoff. Provide three test switches: trace, integrate, visualize bounce-only.

**Acceptance:** native/P3 ON/OFF A/B screenshots and GPU pixel metrics prove an actual, stable final-image contribution. FG/RR OFF and ON have equivalent native-reflection stability; no HDR, VRAM or traversal regressions.

## What not to repeat

- No further VRAM/RR/clamp tuning as a proposed cure for the established PT-induced flicker.
- No one-ray replacement of Control's two-sample native reflection path.
- No extra layers on two-layer native arrays.
- No normal/ID data encoded using arbitrary uint->float bitcasts into FP16 storage.
- No new giant visual-proof gain in the production image until the underlying radiance has been measured.
- No custom GI pass on the native command list while its heap/root argument restoration remains unproven.

### First deliverable from this branch

A **native-preserving P3 observer/build** that can be installed while keeping all official v2.1.2 FG/RR files intact, with P3 trace and composite both OFF by default and logs establishing the native shader identity and array resources. Do **not** label this a PT lighting build until P3-C onward has passed runtime acceptance.
