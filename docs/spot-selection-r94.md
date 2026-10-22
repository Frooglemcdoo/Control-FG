# R94: separate RT light selection from map visibility

R93 screenshots showed blob/staircase shapes corresponding closely to colors in
the selected-light view. The user reports RAW looked the same as FILTERED. This
makes selection/composition a useful next target, but does not prove the cause.
R93's views still undergo material/lighting/postprocessing downstream and are
not pure final-screen buffer dumps. Do not infer exact visibility values from
them. R94 does not claim to repair that display limitation.

The captured native producer a9e5b544... evaluates spotlight map visibility
before candidate ranking: final visibility at SHEX DWORD 7304, zero gate at
7313, energy multiplication at 7596, brightness at 10435 and ranking at 10445.
Its original score uses the difference from the previous candidate brightness.
Our full-distance RT replacement then removes selected-light map visibility in
a different pass. This allows map silhouettes to influence which light receives
RT even though that light's final visibility is evaluated differently.

R94's new selector evaluates candidate diffuse/specular response without map
visibility, ranks the resulting contribution directly, and restores the saved
map visibility before native nonselected-light accumulation. Native frustum,
material, light cookie and near-plane checks remain. Other lights keep their
mapped contributions; the selected one is still evaluated by the existing RT
ray/filter/composition path. Point-light selection is byte-identical. Four-ray
sampling, ray endpoints, caster masks, and all R93 filter/composition assets are
unchanged. This still handles one selected spotlight per pixel, not all lights.

The modified producer is chosen only for ready RT modes. RT R93 BASELINE uses
the exact old selector and existing filtered RT path. NATIVE SHADOWS remains
native. UI switches can occur between producer and later pass recording; either
selector still excludes and publishes the same light it chose, so composition
uses the matching ID even on that transition. A one-frame policy transition
is possible. Logs record producer policy separately from ray submission.

Validation: hash-locked generation; DXBC structure; exact unchanged point-branch
suffix; numerical selection/energy bookkeeping; WARP pipeline validation;
runtime preparation and mode routing tests. These do not prove the in-game
coverage fix. Computing candidates that maps fully shadowed previously skipped
can increase shader cost; measure FPS. Map-independent selection can expose RT
caster gaps or coverage differences that were previously hidden.

## Test

Close Control and install all files beside Control_DX12.exe. Keep RT Contact
Shadows ON. Start with RR and FG OFF. At the same camera near pillar 14, compare
RT R93 BASELINE against RT NEW SELECTION. Check the blob under the open panel,
stair-stepped floor edges, missing Jesse/rail shadows, and frame rate. Move
slowly toward/away from the light, then stop. No restart between modes.

Send matching screenshots or a short video and one saved Lighting-R94 log.
The additional RAW/visibility/ID modes use the new selector; they are optional
and retain R93's display limitations. If the new selection worsens the result,
RT R93 BASELINE restores the previous RT appearance live.
