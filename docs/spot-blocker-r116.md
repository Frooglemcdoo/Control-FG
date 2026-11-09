# R116: Colored spotlight blocker diagnostic

R113's wide endpoint exclusion restored cafeteria lighting but produced shadow artifacts. R114's tighter exclusion activated correctly but did not fix the issue. R114 recorded 8,017 successful RR evaluations and zero failures. R116 measures where the first centered shadow ray hits, relative to those two exclusions. It does not shorten any ray or claim to fix the lighting.

## Test without restarting between views

Close Control to install. Keep native RT Contact Shadows on and the game's RT Samples Count at 1. Turn FG and glass enhancements OFF. Signal views automatically bypass RR at the next native frame boundary, preserving the saved preference. Returning to BOTH restores eligibility through the existing SR/RR transition path. Later material composition and tone mapping still affect intensity; colors encode categories, not calibrated brightness. Leave saved GI samples unchanged.

At the affected table/floor viewpoint, capture **BOTH**, **HIT BANDS**, **VISIBILITY**, and **LIGHT IDS**, keeping the camera fixed. Repeat at the wall/lockers where R113 produced artifacts. Return to gameplay after selecting each mode; allow ten seconds. Save one **Lighting-R116** log. The buttons switch live; no restart between modes. Restore RR after the diagnostic capture if desired.

| HIT BANDS signal | Meaning before downstream game postprocessing |
|---|---|
| Blue (signal 0) | First blocker lies before both excluded intervals; neither guard skips it |
| Orange (signal 0.5) | Blocker lies in R113's wide interval, outside R114's tight interval |
| Red (signal 0.75) | Blocker lies inside R114's tight interval |
| Green (signal 1) | Full-length centered ray found no blocker |
| Magenta | No selected spotlight at this pixel |

VISIBILITY is the existing raw shadow visibility view: dark means blocked, white means clear, magenta means no selected spotlight. LIGHT IDS uses diagnostic colors to show boundaries between selected lights, not their physical colors or mesh identities. BOTH returns normal centered shadows. GI ONLY and LIGHT OPEN remain normal-image references. SPEC 8/16 remain separate quality controls.

## Implementation and limits

The new optional ray shader changes only the spotlight visibility output of the hash-locked R97 centered shader. It classifies the first ray's closest hit; all four centered rays, original endpoints, receiver bias, native hit handlers, point-light work and mean blocker-distance output are preserved. The four centered rays share the same direction. The shader compares hit distance to `originalTMax - min(radius, length*0.1)` and `originalTMax - min(radius*0.1, length*0.01)`, matching the R113/R114 equations including endpoint epsilon and equality behavior. Misses are classified before either interval.

HIT BANDS bypasses spotlight temporal/spatial filtering, preserving native point filtering, and uses a separately embedded categorical-color composition shader. Its values are diagnostic data, never normal light visibility. Selecting BOTH restores the original ray record and composition; existing transition reset clears shadow history. The diagnostic still passes through later game postprocessing, so screenshot colors are qualitative, not precise numerical readback. It does not visualize a world-space light gizmo or identify a mesh. The log confirms CPU command recording, not GPU image completion.

The optional blocker record replaces the failed endpoint experiment slot, still offset 128 in the 192-byte record buffer. Optional state creation failure retries without it. No new per-frame buffers, readbacks, GPU waits, rays or synchronous log writes are introduced. The fourteen canonical shadow sidecars and three glass sidecars remain unchanged. Skin/hair guides and RR clamp are unchanged.

Validation: DXIL assembly/signing; reverse the one output transformation to recover the entire original disassembly; synthetic misses, early blockers, both intervals, threshold equality, invalid radii; shared mode policy, raw-filter/composition routing and record bounds. WARP checks cover the existing signal composition at all band values, not hardware DXR execution. The R116 log adds rr_diagnostic_bypass entry/exit records. RR success counters should stop increasing inside signal views and resume when requested after returning to BOTH. The returned images must establish whether the missing-light region coincides with the R113-only interval, an earlier hit, or a selected-light boundary before choosing another correction.

R116 corrects R115 diagnostic readability: R115 screenshots retained shaded materials and its log recorded RR evaluation during all signal modes. The ray shader itself remains byte-identical to R115. WARP verifies the four band colors and invalid-light magenta; mode tests verify bypass preserves the saved preference and independent resize pause. Baseline verification reverses only the exact reviewed RR bypass additions before checking the original hashes. This does not remove material composition or tone mapping, identify a mesh, or fix lighting.
