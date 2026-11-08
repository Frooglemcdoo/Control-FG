# ControlTOH changes and game settings inventory through R384

Recorded November 8, 2026. Project: Control Epic Compatiable / Control The Oldest House graphics and rendering overhaul.

## R385 settings decision

The user subsequently chose to bundle Owned GI, RELAX, four GI samples and Native reflection sampling into RR rather than expose separate options. The R384 inventory below remains a historical snapshot. See [R385 settings integration](visual-settings-r385.md) for the implemented menu and F10 changes.

## Accepted baseline

**R384 with Owned GI and NVIDIA NRD RELAX is the user-confirmed visual baseline.** After installing R384, Matthew reported: “man that fixed it right up. everything looks good.” He then explicitly confirmed that the test used **Owned, not Native**.

Preserve that combination when integrating the features into the game's settings. Native GI with RELAX is supported by the implementation but is not covered by this successful user test. The test confirms the reported visual result; it does not establish a measured frame-time improvement, universal scene coverage, or validation on other GPUs.

This inventory covers the accumulated lighting, GI, character, volume, performance, UI and capture work leading to R384. Earlier FG, RR, reflection and storage work is listed as inherited functionality. Historical experiments are distinguished from the current rendering path; their presence in source does not make them enabled features.

## Current feature inventory

| Feature or change | What it does | Current standing |
| --- | --- | --- |
| Owned GI producer | Selects our GI sampling/evaluation path within the game's established GI pipeline, retaining native scene, material and light semantics. | User-tested successfully with RELAX in R384. Native remains available for comparison. |
| Single GI producer | Avoids running Owned as an additional full GI lighting pass on top of Native. | Earlier duplicate-work performance regression was corrected; retain this architecture. |
| GI ray count | Changes indirect-light sampling from 2 through 16 in increments of 2. More samples can reduce sampling noise at increased rendering cost. | Shipped request is 4. User previously saw little benefit above 4. This is separate from denoising quality. |
| Owned hit reuse and visibility | Earlier work reuses an eligible previous-frame hit only after matching its receiver and checking current visibility, then evaluates current lighting rather than blending old RGB. | Producer-side work, distinct from RELAX's image history. See the R338 implementation note; do not expose it as a second denoiser. |
| RELAX diffuse denoising | Replaces the problematic GI temporal/spatial filter with NVIDIA NRD RELAX_DIFFUSE. Uses motion, depth, world normals/roughness and native hit distance to stabilize indirect light. | Active R384 replacement; the Owned + RELAX combination is visually accepted. |
| Native receiver shading preservation | Removes and reapplies the exact receiver throughput used by GI6 around denoising. Avoids substituting unrelated RR diffuse albedo for this factor. | Intended to retain lighting energy and material shading rather than flattening faces or deleting projector light. |
| Submission-owned history | Advances denoiser history only after actual queue submission. Uses the previous submitted camera and jitter; resets history for incompatible frames, views or producers. | Rendering correctness infrastructure, not a menu option. |
| RAW fallback and comparison | Keeps RAW GI if RELAX cannot safely accept the current inputs. GI FILTER OFF provides the RAW comparison under the shipped base stage policy. | Does not silently return to the failed older Owned filter. RAW may show noise. |
| Reflection clamp | Provides Native, fixed 60%, and Bypass comparison modes for the reflection clamp. | Shipped request is 60%. This is not a GI ray count or RELAX strength setting. |
| Character lighting reconstruction | Higher-quality character light-rig sampling and guarded diffuse reconstruction address noisy or unstable lighting on skin, clothing and hair shader families. | Retained under RIG STABLE. This modifies shading, not Jesse's texture assets. |
| Near-volume temporal correction | Changes near volumetric history behavior used with the stable character path. | Retained. The solution is not a requirement to keep Volumetric Lighting on Low. |
| Contact filtering comparison | Allows Auto, Filtered and Raw comparison of the contact contribution. | Diagnostic comparison; Raw disables filtering, not the shadows themselves. |
| Native Blend lighting | Retains the established native/RT lighting integration and RR compatibility, with ray-origin and map-guard choices. | Separate from the GI producer. “Native Blend” does not mean that GI: Owned is disabled. |
| Spotlight and reflection improvements | Existing spotlight filtering, origin/visibility fixes, material/reflection integration and specular sample control remain in the package. | Inherited functionality; R384's new denoiser is diffuse GI, not a replacement for every specular or shadow pass. |
| F10 panel | Restores the black/red three-column panel and groups rendering features, comparisons and captures. | Retained until normal settings integration is complete. |
| FPS display | Supplies an in-game FPS readout with an on/off control. | Retained. Do not treat it as a separate benchmark or proof of render-only FPS with FG active. |
| On-demand diagnostics | Starts capture work only on an explicit capture action; keeps ordinary play free of the legacy scene/shadow inventory monitoring. | Capture tools remain available. Required rendering-state tracking still runs when needed for RELAX. |

## What changed in R384

1. Embedded pinned NVIDIA NRD 4.18.0 RELAX_DIFFUSE and its shaders into the mod. The package includes NVIDIA's license. No standalone validation executable is needed to run the filter.
2. Added receiver exports at the exact native GI6 producer boundary: world normal/roughness, view depth and receiver throughput.
3. Reads raw GI, motion, native hit distance and output from the corresponding native filter boundary. Validates frame/view identity, dimensions, viewport mapping, resource states and camera data before recording.
4. Uses the native motion convention and audited jitter conversion. Previous camera and jitter come from the frame actually submitted to history.
5. Replaces the old filter dispatch and disables the native spatial filtering for this path, avoiding another denoiser stacked over RELAX.
6. Preserves the game's output contract, including the required alpha convention and native resource states.
7. Allocates fresh descriptors in the already-bound heap, restores compute state afterward, and retains resources until fence completion and command-list reset permit release.
8. Keys guides to the producer PSO that actually ran, rather than a UI request that might change mid-frame.
9. Excludes injected RELAX dispatches from the game's single-dispatch accounting. This prevents successful filtering from being incorrectly classified as a failed native invocation.
10. Shows RELAX, WAIT or RAW FALLBACK in F10. These reflect recording/admission status, not a claim that GPU completion has already been verified.
11. Links both NRD and its ShaderMakeBlob dependency into the complete Windows DLL.

The standalone GPU fixtures check constant and persistent light energy, light removal, moving foreground/background, camera/history resets and rendering-state restoration. The complete Windows build and packaging checks passed. The user subsequently confirmed the visual improvement with Owned GI.

## Shipped state versus intended settings defaults

These are source-level R384 startup requests, not a reconstruction of every setting used in the successful test. Saved game settings, diagnostic launch overrides and runtime availability can affect effective behavior.

| Control | R384 startup request | Settings integration treatment |
| --- | --- | --- |
| GI producer | Native in this diagnostic build (`ownedRequested=false` under `CONTROL_ON_DEMAND_DIAGNOSTICS`). | Use Owned as the intended default, consistent with the earlier user request and successful Owned test. This documentation does not change the binary. |
| GI filter | RELAX requested (`manualGI=3`). | Expose a persistent GI Denoiser choice; retain RAW comparison in diagnostics. |
| Base GI stage | RAW BYPASS. | Internal base policy. RELAX is selected by the separate filter request; the two labels must not be presented as conflicting user settings. |
| GI rays | 4. | Expose 2, 4, 6, 8, 10, 12, 14, 16. Preserve the separate native-count fallback internally. |
| Reflection clamp | 60%. | Preserve 60% as the intended normal value. Native/Bypass remain useful comparisons. |
| Character lighting | Stable (`quality=3`). | Candidate normal setting: Enhanced Character Lighting. Keep historical variants out of normal graphics settings. |
| Contact comparison | Auto. | Keep Auto for normal play; move forced Filtered/Raw to diagnostics. |
| Native Blend | Projection ray mode with Map Guard off when startup selection becomes available. | Preserve the accepted lighting behavior; avoid exposing internal mode names without clear descriptions. |
| Specular samples | Native (`specularSampleChoice=0`). | Keep independent from GI rays; do not silently set it to 4 or 16 during migration. |
| Captures | Inactive. | Never start from loading settings, opening the menu or launching the game. |

Only Owned was explicitly confirmed as the producer in the latest successful test. The exact test resolution, RR/FG state, clamp and sample selections were not restated with that confirmation; do not invent a benchmark preset from it.

## Settings migration map

The names below are proposed user-facing labels. No settings migration is implemented by this documentation change.

| Proposed game setting | Existing control | Meaning and dependency |
| --- | --- | --- |
| Indirect Lighting Method | GI: Native / Owned | Chooses the GI producer. Requires the relevant RT indirect-light path. Changing it must invalidate incompatible denoiser history. |
| Indirect Lighting Denoiser | GI FILTER | Chooses RELAX versus unfiltered comparison. Keep independent of producer selection. Show unavailable/fallback truthfully. |
| Indirect Lighting Samples | GI RAYS | Controls GI rays only. Current override is applied under the full RR path; the UI reports RR OFF when unavailable. Do not remove that gate merely by moving the control. |
| Enhanced Character Lighting | RIG: STABLE | Selects the stable rig/near-volume/diffuse reconstruction combination. Do not describe it as a texture-resolution setting or a global replacement for all shadows. |
| Reflection Clamp | REF CLAMP | Separate reflection control. Current implementation has Native, 60%, Bypass; a general slider would require additional implementation and validation. |
| Reflection Samples | SPEC | Independent specular sample request, with Native preserved. Higher values increase workload. |
| FPS Counter | FPS ON/OFF | Display preference. Must not enable capture or scene monitoring. |
| Enhanced Lighting | Native Blend | Preserve the accepted blend behavior; confirm the final UI wording and relationship to native RT options during integration. |

Keep ray origin (Projection/Emitter), Map Guard, GI REMOVE, GI STABLE alias, GI VIEW, SMOOTH, forced Contact modes, free camera and historical lighting modes in an advanced diagnostic area. Do not create two independent persisted toggles for GI STABLE and GI FILTER: in R384 they select the same RELAX replacement through different comparison routes.

Existing DLSS/DLAA, FG, RR and DirectStorage settings remain separate systems. Do not relabel RELAX as Ray Reconstruction: RELAX cleans the diffuse GI signal earlier; RR is a separate downstream reconstruction feature.

## Capture and diagnostic controls

| Control | Purpose |
| --- | --- |
| RECORD GI | Timed performance workflow with an arming delay and beep cues for stationary and slow-pan sections. Existing packaging may finish after the game closes normally. |
| CAPTURE GI PATH | Compares RAW GI and stabilized GI while preserving the selected producer and clamp. Captures signals along the GI-to-RR path; restores settings afterward. |
| CAPTURE RR / PACKAGE RR | Records and packages the RR comparison buffers. These are explicit actions. |
| CAPTURE MATERIALS | Collects material/guide evidence when a shading mismatch needs investigation. |
| TEST JESSE | Character shader coverage and comparison workflow. If required skin/clothing/hair coverage is absent, reports that fact rather than pretending comparison images were captured. |
| TEST VOLUME | Isolates volume-related behavior for comparison. |
| TRACE SHADOW | Explicit shadow investigation action; not an always-running monitor. |
| ISSUE REMAINS / ISSUE GONE | Records the comparison outcome in the diagnostic workflow. |
| RESET COMPARE | Clears manual comparison overrides. In current code it returns `manualGI` to zero, so it can leave the base RAW policy active; it is not a universal “restore RELAX defaults” command. |
| FREE CAMERA / NORMAL CAMERA | Repositions the view for diagnosis and restores ordinary camera behavior. |

Whole-screen GI-path collection removes the need to align a small region manually. The compact signal output uses stride-4 point samples; underlying GPU readbacks still use native resolution and incur temporary cost. A compact sample is not a full-resolution image-quality proof. Capture-time FPS is not gameplay performance. Keep capture idle during performance checks.

## Development history and retired approaches

| Stage | Change or finding | Consequence for the current baseline |
| --- | --- | --- |
| Early performance diagnosis | Compared clean Native, Native with tracking and Owned. Heavy tracking and duplicate GI work obscured the real renderer cost. | Retain the single-producer design and on-demand diagnostic policy. Do not claim every prior CPU catalog optimization is an active requirement of the current path. |
| Owned parity work | Owned was brought into the existing GI pipeline; the user reported the large performance penalty corrected and appearance matching Native. | This establishes the producer work preceding denoising; it is not a new R384 timing result. |
| Temporal reuse and visibility | Added previous-hit reuse, matching and visibility rejection, retaining fresh samples when reuse is invalid. | Separate producer reuse from accumulated image color history. |
| Ray-count control | Added even counts through 16. | More rays alone did not resolve every stability problem; keep 4 as the current request. |
| Character and volume investigation | Noise appeared on skin and jacket, including an RT-off case; lowering volume quality made it finer without fixing flicker. Later rig/volume changes improved stability. | Preserve these corrections. Do not attribute every character artifact to the same cause or require Low volume quality. |
| GI isolation | Later RT-on speckles and jaw noise improved with GI Stable, but early filtering lost facial depth/brightness and projector energy. | Preserve raw light energy and shading when filtering. |
| Custom filter iterations through R383 | Rejection, clipping, history and spatial variants alternately improved noise or caused light loss, boiling, flicker and ghosting. | Historical source/tests remain useful evidence, but these filters are not the active RELAX path or its fallback. |
| R384 overhaul | Integrated RELAX with the correct native signals, receiver shading, frame history and D3D12 state/lifetime management. | Owned + RELAX is now the user-accepted visual baseline. |

Inherited project work includes DLSS SR/DLAA and Frame Generation integration, RR material/normal/roughness guides, Native Blend and spotlight/reflection fixes, DirectStorage texture-streaming work, and Steam/Epic/GOG compatibility support. These are existing systems to preserve during settings integration, not newly validated R384 features on every platform. This GI work does not establish a complete path-traced lighting replacement, working three-bounce GI, or conversion of all baked lighting into RT lights.

## Implementation requirements for moving into game settings

- Persist feature selections separately from transient comparison/capture requests. Session-only atomics are not a finished settings implementation.
- Preserve the successful Owned + RELAX combination and the single-producer architecture.
- Apply renderer changes at a safe frame boundary; invalidate incompatible history on producer/filter/resolution changes and camera cuts.
- Keep requested, available and effective state distinct. An unavailable RELAX path must not appear successfully enabled.
- Preserve existing RR/RT dependency checks. Do not promise RR-off operation of the current GI sample override without implementing and testing it.
- Keep normal “restore defaults” separate from diagnostic RESET COMPARE.
- Do not persist active capture phases, forced GI removal, light IDs or free-camera state as ordinary quality settings.
- Keep captures idle and legacy inventory disabled during ordinary play. Necessary rendering-state tracking is not zero-cost monitoring and must not be removed blindly.
- Validate save/reload, restart, menu entry/exit, live toggles and reset behavior, then compare images against R384 Owned + RELAX before changing rendering algorithms again.
- Measure stationary, traversal and combat frame times with identical resolution, upscaling, FG/RR and sample settings. No new R384 performance number is established by the visual acceptance.

## Build provenance and source map

Accepted artifact: `ControlTOH-R384.zip`, SHA-256 `8846a4bd126db04c6cd6506e630c076f6ee06cca65fb1858a70e06ba5882c7aa`.

Build source commit: `7dc562f2dd05fea63e5455f00a3bba28cb4a4ce5`, branch `work/gi-modernization`, repository `Frooglemcdoo/Control-FG`. Successful full Windows build: https://github.com/Frooglemcdoo/Control-FG/actions/runs/42454770548.

| Area | Source or implementation note |
| --- | --- |
| Live RELAX boundary and queue lifetime | `src/nrd_diffuse_runtime.h` |
| NRD session and submitted history | `src/nrd_diffuse_session.h` |
| Conversion, filtering, remodulation | `src/nrd_diffuse_pipeline.h` |
| Native input and camera validation | `src/nrd_diffuse_native_inputs.h`, `src/nrd_diffuse_camera.h` |
| Guide export and state restoration | `src/nrd_diffuse_receiver_d3d12.h`, `src/nrd_diffuse_native_transaction.h` |
| Native dispatch replacement | `src/rr_gi_temporal_native.h`, `src/rr_gi_pipeline.h` |
| Producer request and selector | `src/gi_reuse/owned_request.h`, `src/gi_evaluator_runtime.inl` |
| Ray count and clamp | `src/rr_gi_sample_control.h`, `src/rr_gi_sample_policy.h`, `src/rr_clamp_strength_policy.h` |
| Character and volume | `src/character_surface_runtime.h`, `src/volume_temporal_runtime.h` |
| UI and comparison requests | `src/ui/native_lighting_diagnostic.h`, `src/rt_surface_source_capture.h` |
| Diagnostics activation | `src/diagnostic_activity.h` |
| Earlier producer reuse | `docs/native-lighting-r338.md` |
| NRD integration and synthetic evidence | `docs/nrd-diffuse-integration.md` |

The shipped ZIP remains unchanged by this documentation update.
