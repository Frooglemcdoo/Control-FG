Control primary PT - M12R34

Close Control and replace all package files. Run Collect-PT-Primary-M12R34.cmd
after comparing RT and PT in the same scene. F10/F11 controls are unchanged.

R33 reduced geometry-copy volume but did not fix FPS or lighting. Its median
PT preparation/validation/GPU timings were 7.28/1.43/4.21 ms. The capture ended
with 44,287 buffer allocations and an almost-full 64 MiB idle pool.

R34 fixes a concrete pool retention bug. Once full, the old pool refused all
new buffer sizes and retained obsolete sizes indefinitely. That could allocate
and destroy current-scene buffers every frame, even with PT off. The pool now
evicts its oldest fully retired idle resources to admit freshly returned sizes.
Exact-size reuse, device identity, active budgets and GPU retirement remain
unchanged. pool_evictions is now logged. A Windows/WARP regression fills the
cache, changes buffer size and requires 100 reuses with no further allocation.

THIS DOES NOT FIX THE PT LIGHTING MODEL. Primary direct light and shadows are
still native. Secondary point/spot lights are delta sources, not sampled area
emitters. PT indirect is still additive to native output. Soft area shadows,
native indirect replacement, denoising and remaining material parity are not
implemented by this build. FPS improvement requires in-game verification.
