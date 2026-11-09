# R160 — map-visible spotlight priority comparison

## R159 finding

The submitted R159 trace completed with zero failures and 4096 valid rays: 1790 hits, 2306 misses. Every sampled hit lies within the offline authored clip limit. Six adjacent floor sample pairs along the diagonal edge switch from light 47 with filtered visibility 0 to light 44 with visibility 1. The light-47 hits are approximately 3.53–4.91 units from the receiver, versus full ray limits 9.24–10.61; several instances/triangles contribute. This does not support shortening endpoints or excluding one mesh as a fix. IDs refer to this captured generation, not persistent engine lights/assets. Screenshots are nearby, not exact GPU-frame matches.

The selected/nonselected bookkeeping audit found the selected light is held out while mapped contributions from other lights remain. R94 ranks the unshadowed candidate; at a winner change the same light changes between RT and map visibility. A discrepancy between those visibility methods can produce a visible boundary. The capture does not yet establish each light's native map visibility or prove the new ranking fixes it.

## Optional MAP PRIORITY comparison

Only the score instruction changes against the hash-locked geometric-normal producer: candidate unshadowed energy multiplied by its saved map visibility. The direct candidate score is retained; the old native adjacent-candidate score difference is not restored. Selection/accumulation instructions, material/cookie checks, normal correction, native point branch and the entire selected-light composition remain unchanged. The selected light still uses the accepted authored-clipped RT rays, centered filtering and current-frame spotlight history. RR is bypassed for a matched comparison with RR OFF. Default and RR OFF remain unchanged.

This is a selection experiment, not a confirmed universal fix. It may leave the boundary unchanged, move it, or reintroduce map-related ranking artifacts. It does not make all lights use RT or guarantee selection-invariant illumination. No light ID, room coordinate or mesh exception is hardcoded.

## Test

Start fresh, keep usual DLSS/RT settings and stand on the stairs with the incorrect shadow visible. Compare RR OFF and MAP PRIORITY without moving the camera; wait several seconds per mode and record video. Check the diagonal edge and existing soft/Jesse shadows, then check previously fixed rooms for blobs or noise.

Leave MAP PRIORITY selected and click TRACE SHADOW. The trace now retains that selection, waits five seconds and saves the existing bounded GPU capture and three PNGs. Send lighting.log, manifest.txt, PNGs and video. TRACE SHADOW still selects corrected RR OFF from other modes. Prior mode restores afterward. Existing split-ZIP packager remains below 200 MB.

The MAP PRIORITY shortcut replaces SCREEN AO OFF; that older diagnostic remains in the F9 suite. The optional producer has its own readiness flag and cannot disable Default if preparation fails. No extra ray samples, GPU buffers, readback paths or waits are added.

## Validation

Independent DXBC decoding checks the single score change and exact remaining instruction stream, including point-light suffix. CPU tests cover mode readiness, preserved rendering policies, focused capture selection and mapped-energy bookkeeping. Windows CI checks actual WARP PSO creation and distinct producer/composition routing before delivery. In-game behavior remains to be tested.
