# R107: separate lighting sample costs

The user's native **Ray Tracing Samples Count = 8** fixes the painting appearance and improves the doorway highlight, but costs too much performance. This build provides controlled live comparisons to identify which sampling increase is necessary. It does not claim to reproduce eight-sample quality cheaply yet.

## Test

Install with Control closed. Use RR on, native RT Contact Shadows and Indirect Diffuse on, FG and enhanced glass off. Keep the same resolution, exposure, camera position and scene. F10 opens the comparison panel. All GI count buttons retain centered filtered spotlight shadows, GI filtering, textured world albedo and the fixed 60% clamp. They do not enable VIEW ANGLE or the rejected SPEC FILTER experiment.

Compare these four combinations at the painting and doorway. Wait ten seconds after each change and note image quality and FPS. No restart between comparisons; changing the game's sample slider may briefly rebuild native resources.

| Game Ray Tracing Samples Count | F10 button | Purpose |
|---|---|---|
| 2 | GI 2 | Low-sample reference |
| 2 | GI 8 | Increase GI only |
| 8 | GI 2 | Increase native specular sampling while holding GI at 2 |
| 8 | GI 8 | High-sample reference |

If game count 2 / GI 8 is clean, it is a candidate cheaper combination to measure. If only the rows with game count 8 are clean, prioritize specular sampling/reconstruction next; try game count 4 / GI 2 to establish a middle-cost reference. These outcomes do not establish every pixel's lighting provenance or exclude all native side effects.

Click SAVE LOG once after the comparisons and send the Lighting-R107 log plus the four quality/FPS observations. The **SPECKLES GONE / REMAIN** report buttons can annotate each mode. BASELINE restores the saved GI setting. GI FOLLOW MENU temporarily bypasses the saved GI override; it does not write a settings file. GI 2/4/8 are temporary diagnostic overrides, and returning to another mode restores the saved choice. RR-off retains native sampling.

## Verified findings

Static analysis of the supported renderer shows one graphics setting integer at object offset 0x6F4 clamped to 1..16, then written to separate specular, diffuse and contact-shadow sample options for enabled effects. The specular producer reads its option before hit-buffer allocation. The existing mod GI hook instead overrides its producer-local count before both allocation and shader-provider binding. Therefore changing the game slider to eight does not guarantee GI actually runs at eight if the mod's saved GI count is four.

The same native settings path also selects four specular spatial-filter passes below six samples and two at six or more. In R107's GI comparison modes, the existing RR clamp path continues to request zero native specular spatial passes; that native filter change is observed but not enabled. The full native specular filter remains a rejected experiment.

The R97 replacement spotlight shader uses its fixed four-ray algorithm independently of the native shadow option. Native shadow option values do not prove replacement ray counts, nor establish every other shadow route's behavior.

See `research/rr-sample-isolation-r107/audit.py` and its checked-in disassembly for reproducible evidence. No game binaries are distributed.

## Implementation and validation

The live state has four additional diagnostic modes, using six mode bits so values above 31 cannot corrupt observation flags. Sample selection is latched once before GI allocation and shared with the filter policies. An effective request change uses the existing RR history reset. The native saved sample options are never written by this build.

`rt_sample_options` logs read-only native specular/diffuse/shadow/filter option values; those are not GPU execution counts. `gi_sample_producer` logs the actual producer-returned count used by the existing shared allocation/provider local. Logging uses the existing bounded queue and UI-thread file writer, on changes and periodic snapshots. No new GPU readbacks, shader compilation, resource allocation, waits or render-thread file writes.

Policy tests cover all modes, six-bit selection/observation separation, unchanged shader routes, saved-setting restoration, RR-off preservation and history transitions. Windows checks retain production WARP shader validation. All fourteen spotlight assets, three glass assets, guide shader and clamp shader must remain unchanged from the prior verified build.
