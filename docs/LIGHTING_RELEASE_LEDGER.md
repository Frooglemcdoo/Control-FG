# Experimental lighting release ledger

Updated 2026-10-24. Branch: `experiment/native-hit-capture-r118`.
This is the release-preparation record, not a public release announcement.
No experiment here is automatically a supported feature of the existing release.
The current tester uses an RTX 5090; Windows/WARP checks are not additional
real-hardware compatibility coverage.

## Confirmed fixes to preserve

| Issue | Implemented change | Evidence and status |
| --- | --- | --- |
| Noisy/broken spotlight softness | Centered spotlight filtering with the established RT spotlight path | User reported centered filtering looked correct; preserve existing ray/spatial binaries. Historical full-shadow replacement attempts were rejected for quality/performance. |
| RR amplifies indirect-light speckles | Preserve the active native GI filter when using the corrected lighting modes | User reported GI Filter completely fixed the tested issue. This does not establish that all RR noise is fixed. See `rr-gi-filter-r100.md`. |
| Missing authored light on tables/floors across rooms | R134 clips only valid crossing spotlight rays at the authored cone-near plane | User checked affected rooms and reported the missing lighting completely resolved. Invalid/noncrossing rays retain their full segment. See `spot-authored-clip-r134.md`. |
| Broad false dark shadow around Jesse, also present in native comparison | R139 uses decoded surface normals instead of bent normals in direct spot/point shading; authored clipping retained | R138 Bent Normal Off removed the patch in all three samples; user confirmed R139 completely fixed it with no observed regression. Indirect bent-normal paths remain unchanged. See `direct-surface-normal-fix-r139.md`. |
| Difficult and inconsistent lighting evidence collection | F9 live suite, named PNGs, one session log, mode/readiness checks and asynchronous GPU hit capture | R137: 54 images/18 modes; R138: 60 images/20 modes, zero capture failures. R140 adds corrected motion comparisons; static captures cannot prove motion stability. |

## Active issues and acceptance criteria

R142 decision: the user reports **No History fixes the flicker**, with no visible
noise or smoothness penalty. History Guard only partially helps. Promote the exact
current-frame spotlight path, retaining spatial softness and point history.
The symptom is reported resolved; broader release validation remains open.

| Issue | Current evidence | Next work / closure criterion |
| --- | --- | --- |
| Shadows disappear during room transitions | User reports the issue also in Native; exposure adaptation itself is acceptable. Video alone does not isolate visibility loss from contrast loss. | R143 adds corrected spotlight signal/raw/ID comparisons. Default and exposure unchanged. Await motion video and log; downstream processing still affects these views. |
| Motion-only shadow flicker at stairs/railings | User reports No History fixes it with no visible smoothness/noise penalty; R141 Guard only partially helps. | [#14](https://github.com/Frooglemcdoo/Control-FG/issues/14): R142 promotes the tested current-frame spotlight path. Other-room/transition tests and frame-time comparisons remain release checks. |
| Expensive specular sampling needed for some RR materials | User reported SPEC 16 clears the tested artifacts; 8 is the minimum useful level but still expensive. Prior temporal substitutes did not match 16. | Optimize without restoring noise/boiling. Keep specular filter and 90% clamp experiments classified as rejected; 60% clamp remains the intended baseline. No universal low-cost fix is claimed. |
| Release integration and compatibility | Current F10/F9 panel is a diagnostic build; public settings use the game's menu architecture. | Choose validated defaults, integrate necessary controls using shared settings/font methods, remove unneeded diagnostic code from public packaging, and recheck overlay coexistence. |
| Cross-hardware/storefront validation | Current lighting evidence is principally RTX 5090. Existing storefront support and older FG fixes are separate milestones. | Test the final release package on supported storefronts, FG modes, HDR/SDR and DLSS resolutions. [#9](https://github.com/Frooglemcdoo/Control-FG/issues/9) RTX 4080 MFG remains separately tracked; do not close it based on 5090 lighting results. |
| HBAO+ replacement | Planned in ROADMAP.md | Preserve as future work, not a prerequisite or a claimed fix for the isolated lighting bugs. |

## Implementation already recorded elsewhere

The native graphics menu work uses shared option definitions and styling; FG is
under Monitor, RR under RT and streaming budget under Quality. Model F and the
60% clamp are renderer constants; Model E and the clamp control were removed.
The opaque-reflection option was removed while retaining glass enhancements.
See `settings-architecture.md` for the implementation contract. Recheck these
behaviors against the final release binary rather than assuming old diagnostics
represent the shipping UI. Preserve the accepted skin guide and glass fixes.

## Build and evidence traceability

| Build | Exact remote commit | Automated build | Runtime result |
| --- | --- | --- | --- |
| R134 | `7644613c1fc97d8d852008bce35588a1da5c9d49` | See commit checks | User confirmed authored lighting restored across affected rooms. |
| R138 | `ac56317532c55cdd63a7aac9792e3cb317176e9d` | [40104648742](https://github.com/Frooglemcdoo/Control-FG/actions/runs/40104648742) passed | `Shadow-R138-20261024-101346-49136-0.zip` isolated direct bent normals. |
| R139 | `83a126403066cb26d4e5509400298e672d2dadec` | [40106649871](https://github.com/Frooglemcdoo/Control-FG/actions/runs/40106649871) passed | Broad false shadow fixed; user observed no regressions. |
| R140 | `daa61d649409ba74bbde387a0ab2b31a43bd3ee0` | [40108087558](https://github.com/Frooglemcdoo/Control-FG/actions/runs/40108087558) passed | No History appears to resolve motion flicker; permanent correction remains open. |
| R141 | `8431746fee70b0ab0b8a0f990f0f8fc29ccaa7cd` | [40113754267](https://github.com/Frooglemcdoo/Control-FG/actions/runs/40113754267) passed | Guard partially helps; No History reported fully effective without visible noise/softness cost. |
| R142 | `299a827cfd202db9f11f16e8049aabbbbb063936` | [40115047334](https://github.com/Frooglemcdoo/Control-FG/actions/runs/40115047334) passed | Reuses the accepted No History shader path unchanged. Broad release validation pending. |

Evidence filenames: `20261024-1532-26.4811424.mp4` (original railing flicker),
`20261024-1645-24.5201649.mp4` (user's No History test). Videos are private test
evidence, not bundled in the public repository. No R140 session log was supplied
with the latter video; mode attribution is the user's report, not a verified log.

R139 download SHA256: `762d3dce3f713342adb668d72ce35fe98c0693a66a388e3301b357cb3c85cc77`.
R140 download SHA256: `2a77f9b422ce55242c23c772e9c26c1f18bc3f3028169bc39ab2bd1f0b0e5bd2`.

## Public release gate

- [ ] Close motion-history issue using target-GPU results, not build success alone.
- [ ] Record stationary/moving comparisons, RR on/off, thin geometry, close-light shadows and previously affected rooms.
- [ ] Record CPU/GPU frame times and VRAM for final settings; no unmeasured performance promise.
- [ ] Exercise camera cuts, load/restart, resize/DLSS changes and HDR transitions.
- [ ] Validate FG, overlays, native menu persistence and glass/skin behavior.
- [ ] Publish exact commit, package hashes, supported configuration limits and remaining known issues.
- [ ] Remove test-only panel/capture payloads from the public package where appropriate; keep a separate diagnostic package.
- [ ] Review release notes against this ledger. No merge to main/master or public tag is implied by this tracking update.

R143 adds diagnostics only; see `room-transition-shadow-signal-r143.md`. Source and build identity are the commit containing that document and its CI run. No shader binaries change. Private evidence: `20261024-1740-36.1166846.mp4`.

R144 ([issue #15](https://github.com/Frooglemcdoo/Control-FG/issues/15)) follows the submitted R143 motion video/log with a bounded six-sample selected-light trace in Default. Raw signal and ID colors changed during traversal, but physical light switches are not proved. The analyzer reconstructs world-space geometry and reports ambiguous matches. See `motion-light-context-r144.md`. Exposure and final render shaders are unchanged; no rendering fix is claimed.
