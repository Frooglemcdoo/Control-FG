# R309 — Complete GI evaluator integration test

Install over R308, preserving package folders. Run
`ControlTOH/Diagnostics/Start-GI-Evaluator-Test.cmd` with the game closed.
Keep RT indirect lighting and RR enabled, select GI RAW BYPASS, and leave ray
counts/settings unchanged. The R307 sampler remains enabled in both comparisons.

F10 now shows `GI EVAL: TRANSLATED` or `GI EVAL: NATIVE` in the sampler button's
place for this launcher. `WAIT` means the exact GI6 shader has not been prepared;
load the level first. Compare the usual plant/hair and a room with mixed lighting,
standing still and moving. Allow RR history to settle after each switch. No
capture hotkey, fixed camera or zoom is required. Exit normally to package logs.
Send the resulting diagnostics ZIP and report any missing lights, color changes,
flicker, crash or performance loss. If WAIT persists, send those same logs.

This is a complete editable port of the captured native GI6 evaluator, not a new
lighting estimator yet. Its purpose is to validate the owned lighting pass using
the existing game resources before changing GI transport/reuse. It retains
material sampling, local lights, volume fallback and native output layout.
ReSTIR spatial/temporal reuse is not active. Previous fixtures validate accounting
and native root compatibility, but do not establish full in-game image parity.

Only the exact native shader hash is admitted. Other variants keep native PSOs.
Creation failure keeps native rendering. The native PSO owns its alternate, and
both normal creation and successful pipeline-library loads pass through the
existing observer. No new descriptor heaps, extra shading dispatch, GPU readback,
AS inventory or capture is added. The toggle changes selection at PSO binding;
logs distinguish requested state from dispatch recording (not GPU completion).

Launch normally to leave evaluator testing disabled. The R308 sampler comparison
launcher remains available independently. The test does not change DirectStorage.
