# R76: restore bounded spotlight temporal filtering

R75 successfully activated the configurable ray variant, but the user reported
large shadow patches and weapon flicker. Existing shadow pop-in and incomplete
character shadows remained. Wall flicker is unresolved: the changed appearance
obscured the affected region. The log cannot identify the cause of weapon flicker.

R76 removes the current-frame-only history experiment. It restores the exact
bounded temporal shader used in R73 (SHA256
`0c30a3cb345a5ccb7f5a10d639b611118f683d007ec4c4944453187c62928e91`).
Spotlight history is clipped to current neighborhood visibility before native
blending; point history remains native. Both ray variants, their sampling,
composition, spatial filters and caster coverage remain identical to R75.
This isolates temporal filtering without bringing back R74's activation blocker.

This is not a confirmed fix for the blobs or weapon flicker. If either persists,
the configurable ray path and composition need further isolation. Neighborhood
clipping is not light-identity validation and does not resolve that known gap.

Close Control and replace every package file beside Control_DX12.exe, including
rt_spot_multi.dxil. Load the same location, enable native RT Contact Shadows, and
use F10 to compare Native and RT Spotlight Replacement. Check the large wall
patches and weapon visibility first, then the original three shadow issues.
Save one Lighting-R76 log and a short video for any remaining flicker.

Validation checks the shipped history shader contains the tested clamp and native
blend, verifies all packaged hashes against the runtime, and runs Windows compute
pipeline, switching and build checks. In-game visual confirmation is still needed.
