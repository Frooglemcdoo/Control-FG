M12R85 - reserve PT output for authenticated early surface draws

Surface recording no longer requires the opaque-ray callback to have already
reserved an output. If no output exists for the engine frame, an early surface
can reserve one using its bound render extent and authenticated full camera.
The later opaque producer reuses it only with the same full camera/frame/extent.
The existing ordered-producer submission guard still controls PT replacement.
Missing producers, changed cameras, ambiguous frames and invalid bindings keep
native lighting. No previous-frame output is borrowed.

R84 logged over 82,000 missing frame matches and over 1,800 missing sampler-table
bindings. These are draw counts, not frame counts. R85 removes a recording-order
dependency; those counts do not establish how many draws were early or visible.
Missing sampler root 16 remains unresolved and its validation remains required.
EarlySurfaceReservations and new skip reasons distinguish these remaining cases.
No claim that all flicker, noise, missing lights or smearing is fixed.

Ray counts, RR inputs and settings policy are unchanged. Existing output budgets
remain. An early view with no producer can temporarily consume an output slot;
submission keeps native lighting in that case. Windows/WARP validation cannot
establish in-game visual quality or performance.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Run Collect-PT-Primary-M12R85.cmd after normal gameplay.
