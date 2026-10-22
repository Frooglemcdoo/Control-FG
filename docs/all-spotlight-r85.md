# R85: use constants from their native producing pass

R84's completed GPU samples had zero tasks and zero valid lighting. Most
receivers failed native input checks (stage 2); others failed world geometry
(stage 4), with some invalid light-list entries (stage 7). CPU command recording
was successful, but the replacement did not produce lighting for those samples.

Two unsupported assumptions in the adapter are corrected:

- The native producer's `g_uSpotLightTotalCount` has RDEF used flags=0.
  Enumeration now follows the native tile list and checks IDs against the bound
  structured-buffer length. Sentinel and configured tile-capacity termination
  remain identical to the native shader. No unchecked light reads are allowed.
- The producer's `g_mViewToWorld` also has RDEF used flags=0. World-space receiver,
  normal and emitter transforms now use the native RT pass's system CBV, whose
  contact-ray DXIL actually reads matrix rows 6 through 9. Producer clip-to-view
  and deferred tile constants retain their original native bindings.

The WARP fixture now poisons the unused producer matrix with NaNs and checks both
zero and garbage unused total-count values, using a separate valid RT matrix.
It verifies receiver, emitter, normal, tile identity and out-of-bounds rejection.

## Test

Close Control to install; copy all files beside Control_DX12.exe. Keep native
RT Contact Shadows enabled. Use F10 to compare NATIVE SHADOWS and ALL RT
SPOTLIGHTS live in the same affected room. No restart is needed between modes.
Wait several seconds in ALL RT SPOTLIGHTS, then SAVE LOG. The file is named
Lighting-R85-*.log and includes completed `rt_all_gpu` results.

This corrects the invalid input dependencies found in R84. In-game image quality,
coverage and performance still require confirmation. Point and sun shadows remain
native. Existing RR, glass, graphics-menu and RTX 40-series fixes are retained.
