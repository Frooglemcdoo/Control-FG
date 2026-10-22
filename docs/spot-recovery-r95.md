# R95: withdraw the R94 selection experiment

R94 was reported to remove shadows, over-soften others, and green-screen the
machine. The screenshots show Jesse shadow coverage returning with the R93
baseline. Both new-selector raw and filtered modes lose coverage. These are
visual regression evidence, not evidence of the system crash cause.

R95 restores the R93 runtime and all R93 shader assets exactly. It removes the
new selector sidecar, its loading/preparation/readiness logic, and its mode.
It also restores the original command-list Reset hook and pass-scoped shader
selection. No R94 selector can execute, even if its old sidecar remains on disk.
The old selector generator and its tests are removed; Git retains the history.
RR, native graphics settings, glass enhancements and the RTX 40 provider are
unchanged. This recovery does not fix the earlier blob or stair-step shadows.

## What the review established

R94 changed both candidate ranking and the zero-map-visibility gate. It computed
BRDF responses for lights the native shader previously skipped. The selected
light determines which shadow is ray traced and its emitter size, so this change
can alter both coverage and softness without changing ray/filter shader bytes.
The change also introduced producer replacement outside the original RT pass
and through command-list Reset. Those are removed together, rather than leaving
one experiment active while guessing which change caused the crash.

Possible extra GPU work, invalid arithmetic in newly evaluated candidates, and
mode/history interactions remain hypotheses. No stop code, crash dump or R94
log was supplied. A passing WARP test or successful build cannot establish
hardware stability. Do not reproduce the crash to collect more evidence.

## Installation and remaining work

Close Control. Replace the installed files with this package. One restart is
required for the DLL replacement; F10 changes modes live afterward. The default
remains NATIVE SHADOWS. RT FILTERED restores R93 behavior. RT NEW SELECTION and
RT R93 BASELINE comparison buttons are gone.

RAW VISIBILITY, FILTERED VISIBILITY and LIGHT ID COLORS remain the R93 views;
downstream material/lighting/postprocessing affects their final screenshots.
They are not pure buffer dumps. Use RR and FG off when interpreting those views.

Further shadow work needs to preserve native light assignment while examining
selected-light ray coverage and composition. Raw equaling filtered does not,
by itself, prove selection is faulty. Existing crash stop-code or dump evidence
is needed to investigate the system failure without asking for another crash.
