# R212 — GI history color clipping

R211 now loads and applies successfully. The supplied 09:57 lighting log reports 4,769 successful RR evaluations, zero failures, and every sampled adaptive GI dispatch applied. The user reports no boiling or speckles but prevalent ghosting. RR is unselected toward the end of that log; visual observations have not been explicitly correlated to that interval.

R212 retains R211's separate GPU pipeline and scope restoration. It adds a current-frame color neighborhood bound to the reprojected GI history. Motion/depth checks alone can accept history with stale lighting when the reprojected surface remains valid.

The shader reuses the native filtered current sample at LOD 1.2 and adds four samples one current-texture texel left/right/up/down at the same LOD. Componentwise RGB minima/maxima include the native energy-clamped center. Bounds expand by 25% of the upper bound plus 0.001 in the shader's radiance units. The lower bound is nonnegative. Reprojected history RGB is clamped to this interval before the unchanged native blend. The margin allows normal noise variation to retain temporal averaging rather than forcing every flat pixel to current-frame color.

R211's 70% stable-history weight, motion scale 400, relative depth tolerance 0.01, native current-frame energy clamp, CameraCut handling, and spatial passes remain. Only scratch registers r5-r8 are added. No new textures, descriptors, providers, native record copies, or history clears are introduced. The additional four texture samples have not been performance-measured in game.

Validation: the exact source SHA is required; all non-SHEX chunks are unchanged; the native instruction sequence is preserved except the previously audited R211 constants and temporary register count. New instructions appear only at three checked boundaries. The actual shipped native/adaptive bytecode is compiled into WARP PSOs. Synthetic texture dispatch/readback checks stable history, bright/dark stale history, native control, current-frame firefly clamp, depth rejection, CameraCut, and dark stable noise. Pipeline scope, fallback, and exception restoration tests remain. These fixtures do not prove scene quality.

Test Native Blend, RR ON, FG OFF, SPEC Native, hit distance OFF. Walk over the patterned carpet, stop, and pan; inspect stationary and moving dark shadows and the previous speckle area. Save F10 lighting and probe logs while RR is still enabled. Expected trace: gi_temporal_adaptive build=R212 applied=1 with continued RR successes. Compare FPS in the same view to R211.

Projection rays, MAP GUARD OFF, restored angled reflections, and accepted R205 settings remain. Production is unchanged. History bounds and margin are experimental; watch for lost indirect-light detail or renewed noise near movement.
