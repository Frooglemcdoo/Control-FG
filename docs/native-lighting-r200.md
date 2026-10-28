# R200: retain native-map blockers in Native Blend

The R199 captures showed 178 outside-window samples where the native map
blocked the selected light but the full-length native ray and both opaque
triangle probes missed. Moving the camera inside did not eliminate the mismatch
for the same physical light. R200 adds a native-map fallback in Native Blend.
It does not change RT scene inclusion or prove which geometry is missing.

## Test at the affected window

Launch normally: Native Blend selects MAP GUARD ON when ready. Keep RR ON,
SPEC Native, hit distance OFF and the accepted game RT settings, including
Ray Traced Contact Shadows. These settings are shared by both comparisons.

- F10 -> MAP GUARD: ON uses R200's native-map protection.
- Click it to select MAP GUARD: OFF, the exact R199 composition shader.
- Close F10 and allow the image to settle before comparing. Keep the same
  camera position and check both shadow placement and FPS. No restart needed.
- F10 -> TRACE SHADOW captures either setting without switching it. Hold still
  until completion. Send the ON and OFF Shadow-R200 folders if the result is
  ambiguous. Their manifests identify map_guard and the selected composition.
- FREE CAMERA / NORMAL CAMERA remain available for inspection. A new inside
  capture is optional; the first comparison should use the affected outside view.

Selecting NATIVE BLEND again returns to MAP GUARD ON. The toggle is blocked
during shadow/performance/reflection capture. Changing rendering mode through
other controls cancels a Native Blend trace rather than mixing its settings.

## Scope and cost

The native selected-spotlight composition already contains its PCF/VSM atlas
sampling, authored fade and static/dynamic layer selection. The RT variant
previously discarded the final map result. R200 saves that result, retains the
accepted RT fade, then uses:

    final selected-spot visibility = min(native map, R199 RT visibility)

The minimum preserves whichever path reports stronger occlusion. It does not
multiply two penumbras or apply fade twice. A fully open native map leaves the
R199 result unchanged; a blocked map prevents the selected light leaking through.
Material evaluation still receives the existing map bypass so the map is not
applied a second time in material attenuation. Light ranking, light color,
point lights, nonselected lights, GI, reflection sampling/reconstruction,
authored ray endpoints and free-camera controls retain their accepted behavior.

The shipping shader adds two instructions to R199, uses an unused lane of its
existing temporary register, and adds no resource declarations, ray samples,
dispatches, texture allocations or synchronization. However, retaining map
results can keep atlas fetches alive that the GPU compiler previously removed.
This is not a claim of zero GPU cost; compare ON/OFF after settling. Native map
resolution, bias or edge noise can also become visible again where the map is
more occluded than RT. This fallback cannot correct cases where RT itself
incorrectly over-shadows a surface.

The capture's filtered RT visibility and geometry probes remain raw RT evidence;
they can still show a miss after displayed lighting is protected by the map.
Composition before/after records and PNGs show the resulting lighting. The
producer's private map capture and the composition's map evaluation are separate
evaluations, so exact penumbra equality is not assumed.

## Verification

The native map-selection instruction was recovered and independently verified
by reconstructing the original composition SHA-256
6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621.
Static tests prove that the new shader differs from R199 only by saving the map
and applying the minimum; all remaining instructions, bindings and temporary
counts match. Scalar tests cover all layer-bit combinations, fade endpoints,
open/blocked maps, and equal penumbras without double attenuation.

A WARP test executes the actual emitted save/fade/min instructions extracted
from the shipping shader with synthetic inputs and checks that point visibility
is untouched. The complete guarded and R199 shader pipelines are validated too.
Runtime tests cover distinct composition routing and shared baseline policies.
These checks do not substitute for the in-game visual/performance comparison.

This package is experimental. No main-branch or release assets are updated.
