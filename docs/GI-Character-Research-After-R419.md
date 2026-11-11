# GI-dependent character instability: decision after R419

2026-11-11. Research only; no runtime changes or new user capture requested.

## Latest result and retained constraints

User reports R419 did not change jaw/neck noise or hair/face speckles; performance seemed better, unmeasured. Artifacts appear during movement when GI illuminates Jesse. Do not repeat stationary tests. Keep projector, desk, plant accepted appearance, native GI ray count and RELAX OFF. R419 remains a legitimate history-reset correction, not an established visual fix.

## Evidence ledger

- Older native GI also produced speckles; this is not uniquely an Owned GI regression. Lamp removal / RT indirect lighting off removed earlier plant/hair speckles.
- R359 diffuse isolation implicated the reconstructed diffuse term. Older RR-off images retained neck grain. Different poses/exposure and the changed native filtering prevent exclusive causal attribution to one pass.
- R360 already implements native-map-aware character diffuse reconstruction. Do not propose this again as an untried fix.
- R361 broad native DLF restoration had zero calls at the targeted sites: inactive intervention, not evidence that an active native filter failed.
- R362 contact-only policy matched baseline, so it was not an independent change.
- R378 raw GI input and RR-bound input matched in full-resolution counters, with finite guides. Exported images use stride four and a rear view; they cannot establish full-resolution jaw motion correspondence. Red-tinted bright samples already occur in the pre-RR signal, but brightness alone does not classify them as invalid.
- R416 removed biased endpoint reuse; fresh-ray GI remains. Do not reinstate unweighted accepted-endpoint reuse under a new cache name.
- R418 log confirms replacement records for skin-basic, skin-live-noise-b and hair opacity. It does not prove every affected pixel executes those replacements. No repeat activation run is needed.
- R419 reset change was visually ineffective per user. Stop treating resets as the primary explanation.

Evidence locations: research/rr-existing-evidence/README.md; research/character-surface-r346/{README.md,r359-base-buffer-split.md,r360-diffuse-reconstruction.md,r362-active-rt-sources.md}; research/gi-source-r267/sampling-audit.json; docs/GI-Endpoint-Estimator-R416.md; docs/Character-Activation-Evidence-R418.md; docs/RR-Motion-Research-R419.md.

## New reproducible finding: the current filter preserves large impulses

Production tools/character_diffuse.py applies a radiance weight:

    saturate(1 - max(abs(neighbor-center) / (0.5*max(abs(neighbor),abs(center)) + 0.0001)))

The center always retains weight four. With matching normal/material/mask and a valid reconstruction map, a center of 1 surrounded by 0.1 produces zero neighbor weights. That bright center survives unchanged. This is intentional edge preservation, not a broken arithmetic operation, but it limits the filter's ability to suppress bright speckles.

Reproduction: python tools/test-character-diffuse-impulses.py. Executes the production emitted instruction block using the existing DXBC interpreter, not merely a rewritten formula. All eight diffuse neighbors are actually read.

| Center | All neighbors | Output |
|---|---|---|
| 1 | 0.1 | 1.000000 |
| 1 | 0.49 | 1.000000 |
| 1 | 0.8 | 0.871422 |
| 1 | 1 | 1.000000 |

Four cases passed. This proves a filter limitation, not that these synthetic impulses are the current game's root cause. Loosening the threshold indiscriminately risks blurring real lighting edges, repeating earlier regressions.

## External research and applicability

NVIDIA NRD input guidance requires signals to converge within finite history and warns that high-energy fireflies are difficult to remove spatially. This is NRD guidance, not a specification of RR internals. It supports improving source signal quality rather than assuming another denoiser fixes inadequate sampling.
https://github.com/NVIDIA-RTX/NRD#noisy-inputs

NVIDIA RTXPT explicitly uses SHaRC for otherwise excessively noisy indirect diffuse before DLSS-RR. Its documented performance gain relies on ending multi-bounce paths early; Control's existing GI evaluator does not automatically offer that saving. RTXPT also documents bias, lighting lag and artifacts.
https://github.com/NVIDIA-RTX/RTXPT/blob/main/Docs/Design.md

SHaRC requires update, resolve and query integration plus resources/barriers. It caches radiance in world space, unlike R416's rejected endpoint replacement. It is a possible source-stabilization architecture, not a drop-in toggle or a promised FPS win.
https://github.com/NVIDIA-RTX/SHARC/blob/main/docs/Integration.md

## Ordered next work and decision gates

1. Audit the current fresh-ray GI producer-to-evaluator contract using existing source and shader captures. Trace random coordinates and basis, hit/miss handling, receiver throughput and normalization, position spaces and producer/consumer frame ownership. Existing cosine-PDF proof explicitly assumes uniform UV and an orthonormal basis; runtime matrix agreement remains unproven. Record what is verified versus unavailable, not a blanket assertion that sampling is correct.
2. Audit the character's reconstructed diffuse sample provenance during motion: original packed 2x2 map, which surface each sample represents, and consistency with the current primary guides/depth/motion. Existing material/normal guards lack depth/object identity; that is a candidate failure mode, not proof of wrong-surface reads. Do not label native motion vectors defective merely because symptoms occur in motion.
3. Select the correction from those results. A mapping/space/weighting defect takes priority over filtering. If correspondence is correct and the signal is valid but high variance, investigate better sample allocation/importance sampling first, with proper PDF weights and unchanged lighting energy. Evaluate radiance caching only with an explicit cost and invalidation design.
4. Before another user build, require a reproduction tied to the changed code, exact intervention coverage, and checks that projector/desk/plant lighting energy and detail survive. No global clamp increase, more RELAX, or unsupported actor-specific suppression.

The next milestone is a source-level diagnosis separating variance from wrong-surface reconstruction. This research does not yet establish a complete fix or measurable performance gain.
