# R183 component captures

Both uploaded sets passed all nine archive hashes, all chunk hashes and complete
file reconstruction. Option 2 reconstructed 67 files; option 3 reconstructed 68.
Each contains two consecutive settled RR frames at both painting and elevator,
2560x1440 input, 3840x2160 output, SPEC4, reset=0, successful RR, pre-exposure
5.55555534, and 14 captured resources per frame.

Runtime logs confirm option 2 `requested=shading applied=1 native_fallback=0`
(PID 72280), and option 3 `requested=emission applied=1 native_fallback=0`
(PID 55000). The R183 launcher correction reached the game in both cases.

In the selected interior painting region, option 2 retains noisy specular
lighting. The selected painting region in option 3 has exactly zero captured
specular RGB in both frames. This supports prioritizing non-emissive reflected
shading for the painting issue; it does not identify local-light specular versus
diffuse/environment shading within that branch.

The elevator needs a more qualified conclusion. Option 2 retains broad noisy
reflected lighting. Option 3 largely removes it but leaves sparse bright
contributions: in the measured back-wall crop, 0.6025% of first-frame pixels
have nonzero specular luminance, with peak 0.0140066 and mean 0.00002214.
Emission therefore cannot be dismissed as a source of elevator outliers.

The camera positions differ substantially between captures (especially the
painting). Regions were selected independently on the pictured materials.
Do not infer contribution percentages or compute an image subtraction. Two
frames also differ in jitter. JSON records coordinates and absolute metrics;
these measurements do not prove RR convergence, pure sampling variance, or
the exact stage introducing each remaining artifact.

No capture repetition is required for these qualitative findings. Next source
target is the non-emissive reflected shading branch for painting stability,
while retaining the separate emission-outlier case in elevator evaluation.
Rendering is unchanged by this analysis.
