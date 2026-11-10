# Existing RR noise evidence: do not restart capture triage

The user rejected another general CAPTURE RR request on November 10. That request is withdrawn. No new user run is currently required. Investigate existing artifacts and changes first; a future request must name a specific unanswered measurement and establish why existing captures cannot answer it.

## Reopened actual capture

GI-Path-R378-3632-27477578.zip (Library libfile_8e9b3a8ff4588191bf975e572211a741) contains Raw frames 4619/4620 and Stable frames 5681/5682. ZIP CRCs checked. Same-frame pre-filter GI, GI at RR, scene input, native depth/motion/G-buffers, RR guides and output are present. `r378-recheck.json` records fresh byte-buffer checks. Raw exported RGB is exactly preserved, agreeing with the capture's full-resolution zero-difference counters. All four frames have RR result 1, reset 0, matching pre-GI frame IDs, and finite inspected GI/normal/motion buffers. Stable changes the GI input. This already answers the broad boundary-capture question previously asked again.

Limits: exports are stride-four point samples; camera jitter and animation differ. Do not report unaligned frame differences as pure noise. This older build is not evidence that R415 behaves identically, but its existence means another generic capture is not a justified first step.

## Established history and current relevance

| Evidence | Finding | Action |
|---|---|---|
| research/gi-source-r267/signal-origin-audit.json | Bright GI candidates also appear in the pre-RR composed input; e.g. frame 4578 has 4900 candidates, 4483 also above the input neighborhood threshold. Candidate counts are not proof all bright samples are erroneous. | Trace source variance and composition using those coordinates; do not restart by asking whether noise exists before RR. |
| research/rr-input-r258/README.md; research/foliage-guide-r265 and r266 | Specific textured foliage replay coverage gaps were found and evidence-gated variants added. Current rr_diffuse_replay.h still includes the foliage policy and replacement path. | Distinguish historical repaired gaps from any still-uncovered variants; no blanket foliage rewrite. |
| docs/gi-guide-followup-r276.md | Existing six-frame audit found finite guides, matching normal/roughness decode, full-coverage albedo equality and exact RGB raw bypass. | Do not treat NaN, obvious normal packing or raw-copy corruption as wholly untested theories. Semantic correctness remains separate. |
| research/gi-temporal-r208/history-feedback-review.md | Stronger motion/depth rejection and clipping did not establish a visual fix; spatial output fed history, with another temporal blend in the native spatial shader. | Do not repeat threshold tuning as a new approach. Those old shaders are not proof of a defect in RELAX's independent history. |
| research/character-surface-r346/r360-diffuse-reconstruction.md and r361-upstream-diffuse.md | Character diffuse isolated as priority; consumer smoothing did not resolve distortion. | Preserve the distinction between character lighting and guide replay. |
| research/character-surface-r346/r362-active-rt-sources.md | R361 broad diffuse restoration had zero calls at either target site for effect mask 0x6b. | Exclude this inactive experiment from evidence of an effective fix or failure of that filter. |
| docs/gi-history-footprint-r381.md and gi-history-slope-r382.md | Synthetic history defects corrected; R381 was visually rejected; R382 did not establish final game quality. | Passing synthetic fixtures cannot close the in-game issue. |
| docs/nrd-diffuse-integration.md and RELAX-Detail-R415-validation.md | RELAX was user-accepted earlier, then rejected for softness and the latest lighting artifact. R415 four-pass candidate passed synthetic motion while game quality failed. | Treat acceptance as scene-limited. Keep RR-first/raw target and stop increasing smoothing. |

## Work boundary

No newly confirmed root cause or rendering fix is claimed by this audit. Next work is to reconcile the already captured source/guide behavior with later Owned GI, character diffuse, RR guide and denoiser changes. Keep the new Jesse halo separate from the older lamp speckles until evidence connects them. The user is not being asked for another capture.

