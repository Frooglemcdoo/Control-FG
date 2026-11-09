# R74: isolate spotlight history

R73 logs show the replacement submitted with per-sample endpoints, expanded
caster coverage and composition active. The user reports all remaining issues
unchanged. The logs do not contain per-pixel light identities or ray hits.

Inspection of both native spatial passes confirms that they compare selected
light IDs before weighting neighbors. The temporal pass has depth/motion checks
but no previous-light identity input. R68 neighborhood clipping cannot establish
identity in mixed neighborhoods.

R74 is a diagnostic build, not a confirmed visual fix. It replaces the spotlight
history value with current visibility immediately before the native temporal
blend. The point channel retains native history. Spatial filtering remains active;
ray count, ray endpoints, light selection, composition and caster coverage match
R73. Native mode remains available through F10 without restarting. No new logging
I/O, resource readback or render-thread waits are introduced.

## Test

Close Control to install all package files. Load the same affected areas and use
F10 to select RT SPOTLIGHT REPLACEMENT. Keep RR and other graphics settings the
same as the R73 comparison. Check wall flicker, hallway shadow pop-in and Jesse's
shadow close to/far from the light separately. SAVE LOG creates a Lighting-R74 log.

Removing spotlight temporal accumulation may increase noise; that is a possible
diagnostic tradeoff, not an intended final quality setting. If a symptom remains,
stale spotlight history is insufficient to explain it. If it improves, that
supports implementing history validation with previous-light identity rather
than leaving temporal filtering disabled. Missing caster geometry and light
selection boundaries are not repaired by this change.

## Validation

The shipped shader instruction test checks the actual lane mask and swizzle,
current spotlight output and preserved point blending. Windows CI disassembles
and creates all compute pipelines on WARP and tests live switching and logging.
These checks do not replace in-game visual validation.
