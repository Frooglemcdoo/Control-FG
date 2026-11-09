# R215 — GI cleanup mask and zero-signal diagnostics

R214 produced no visible improvement in the supplied screenshots. The 11:42 log confirms cleanup on/off both applied successfully, with 10,155 RR successes and zero failures. This build adds visual diagnostics rather than tightening the cleanup threshold.

Default: Native Blend, GI CLAMP ONLY, CLEANUP OFF, GI VIEW NORMAL. This preserves the R213 Clamp Only comparison reported free of ghosting/boiling. Use RR ON, FG OFF, SPEC Native, hit distance OFF.

At the projector-room speckle location, hold the camera still. The new F10 GI VIEW button cycles:

1. NORMAL: normal Clamp Only rendering. Capture the speckles.
2. MASK: this GI output is replaced by magenta where R214 would reduce the native-clamped luminance by more than 0.000001, and zero elsewhere. The R214 cleanup calculation runs regardless of the saved cleanup toggle. Capture the result.
3. ZERO: this GI output is zero everywhere, retaining native dispatch/copies, the rest of lighting and RR. Capture the result and note whether the speckle pattern survives.
4. Return to NORMAL, wait, then save the F10 log while RR remains enabled.

Wait ten seconds in each mode for downstream reconstruction history to settle. No F9 sequence is needed. Cleanup displays N/A during diagnostic views; diagnostic views display N/A outside Clamp Only. Diagnostics and cleanup scope are latched once per GI filter call and restored on exception. They are not persisted across launch.

Interpretation: MASK computes its predicate immediately before the GI output write, so it identifies the cleanup's affected source pixels. The displayed result still passes through the game's lighting composition, material response, RR, exposure and tone mapping. It is not a raw-buffer capture, pixel-count measurement, or final-screen pixel-exact overlay. Absence of visible magenta alone cannot establish that no pixels were clipped. ZERO is an independent causal control: persistent settled speckles would point beyond this GI contribution, while their removal would justify investigating this signal and its downstream interactions. The views deliberately alter lighting; do not assess normal quality while they are active.

Implementation: R214's normal cleanup/comparison shaders are unchanged. Two additional immutable PSOs share the native root signature and ABI. MASK preserves the native-clamped luminance before the current-frame cleanup limit, subtracts the post-cleanup luminance, compares the difference against 1e-6, and writes (8,0,8,8) or zero. ZERO overrides the final write to all zeros. Both retain zero temporal coefficient and zero spatial passes. No new resource bindings, engine object copies, provider overrides or history clears. Optional diagnostic PSO failures preserve availability of the normal variants and log the missing view; applied=0 is not a valid diagnostic result.

Validation covers shader checksum/binding invariants, exact mask predicate and final writes, actual WARP PSO creation and output readback for isolated/adjacent bright samples, supported edges, uniform lighting and zero output, pipeline restoration, and all stage/cleanup/view combinations with fallback and exceptions. The fixtures validate shader behavior, not scene interpretation or runtime integration.

Accepted projection rays, MAP GUARD OFF, angled reflections and other R205 defaults remain. Production unchanged.
