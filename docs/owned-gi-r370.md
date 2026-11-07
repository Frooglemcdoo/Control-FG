# R370: preserve light, reject motion trails

User reported R369 removed projector-floor light and ghosted Jesse. These are
reported in-game regressions, not yet independently reproduced in the game.

The R369 temporal shader deleted the brightest one or two spatially unsupported
samples. A compact real light meets that criterion, so those samples are now
preserved. A spatial neighborhood alone does not establish that a ray is noise.

History now responds to shading disagreement before clipping (clipping first
can hide disagreement). Velocity discontinuities and depth edges reject history;
motion reduces accumulation. Uniform camera movement is treated conservatively
because this binding has no independently verified material/normal history.

The fixed resource ABI, red-in-alpha, native temporal/spatial ordering and one
spatial pass remain unchanged. UI and on-demand diagnostics are unchanged.

New GPU regressions compare the actual R369 bytecode against current bytecode:
1. One- and two-pixel real lights must preserve integrated current energy.
2. Repeated compact illumination must not vanish over eight feedback frames.
3. Moving foreground against a similar-depth background must reject the stale
   trail relative to a current-frame reference, including velocity discontinuity.

The old colored-firefly fixture conflated true light energy with outliers. Its
metrics remain printed, but zeroing legitimate positive samples is no longer a
success criterion. Unbiased noise, brightness, alpha, finite output, and removed
light tests remain enforced. Removing destructive rejection can expose isolated
speckles again; this build is not claimed to solve all ray noise. Real-game
projector and Jesse results are still required.
