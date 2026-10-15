M12R54 - PT-on DLAA / Quality CPU and GPU scaling diagnostic

Close Control, extract all files beside dxgi.dll, and launch with PT enabled.
The R53 save fix and restart-only mode are retained. No enable script needed.
Keep native RT, RR, FG, scene and camera unchanged throughout this comparison.
Stand still in a representative scene for 30 seconds on DLAA, switch to Quality
and wait 30 seconds, then return to DLAA for 30 seconds. Note displayed FPS for
each segment. Close Control and run Collect-PT-Primary-M12R54.cmd; send its ZIP.

R53 confirmed PT dispatch dimensions changed 3840x2160 -> 2560x1440, with sampled
PT ray GPU cost falling from about 4-4.8 ms to 1.8-2 ms. CPU preparation remained
roughly 6-8 ms plus submission validation. This does not establish a single cause
for total frame time. R54 isolates preparation stage costs in completed, armed,
non-probe PT dispatch windows. Every 120 samples logs dimensions, GPU dispatch
average and CPU prepare/lease/SBT/camera/metrics/record/remaining setup averages.
A resolution change clears the window; samples from different sizes never mix.
CPU/GPU measurements can overlap other work: do not add them as total frame time.
Existing queue/capture/validation counters remain available in the same log.

This is a diagnostic build, not an FPS fix. Shader output, resource ownership,
scene validation and PT-on/off behavior are unchanged. No renderer.ini supplied.
