Control-FG Primary PT M12R29

Close Control and replace every supplied file. Use F10 > PT Reflections.
After testing run Collect-PT-Primary-M12R29.cmd and send its results ZIP.

R28 now produces a visible change, but the user reports lost reflections.
R29 corrects primary-surface reflectivity and roughness to use the final native
G-buffer values. The previous texture reconstruction multiplied packed specular
color by a specular texture; native general-material F0 instead uses GBuffer2.g
with normalized material tint. Primary roughness now uses GBuffer1.b.
Secondary hits retain the existing texture material decoder. This is a targeted
material correction, not a claim that all missing reflected lighting is solved.

Early roulette is reverted to start after three continuations, reducing variance
in reflected paths. Four-hit transport and direct native-light-buffer output stay.
This may increase GPU cost compared with R27/R28.

The logs locate large CPU costs in preparation (about 15-26 ms steady state) and
late validation (about 11-22 ms, with higher spikes). Allocation lookup previously
scanned every retained allocation for each uncached address. R29 indexes address
intervals and skips impossible owners, retaining exact range, overlap and token
checks. Tests cover long overlapping ranges, removal, address reuse and overflow
boundaries. No scene ordering or resource-lifetime check is disabled.

Windows/WARP compilation and CPU ownership regressions are run before packaging.
In-game reflection quality and performance remain pending user validation.
