# R336 Owned GI

Launch Control DX12 normally with RT indirect lighting enabled. F10 starts with GI Owned, GI Raw Bypass, and Clamp Native. The GI Owned button switches both ray generation and lighting evaluation to Native and back during gameplay. The separate reflection clamp button cycles native (100%), the 60% override and bypass. The raw GI filter control is independent.

Owned now runs its own current-frame hit loop and output resolve in the existing GI6 dispatch. It reads every configured hit slice, evaluates the captured game's material, volume, point-light and spotlight equations, and writes raw diffuse light and mean hit distance for RR. Owned visits four angular subcells over four frames per pixel and ray. Each frame traces and shades its own samples. Temporal reservoir reprojection, cross-pixel reuse and visibility validation of reused hits are still outstanding.
