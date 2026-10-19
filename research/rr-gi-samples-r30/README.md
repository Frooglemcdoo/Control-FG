# R30: four-sample comparison after R29 improvement

User feedback: R29 improved the blinds artifact but did not fix it.
Capture `Control-RR-GI-Audit-R28-20261019-141537.zip` contains PID 21044 frames 3324/3325 at 3840x2160, preset F, successful RR output and no reset. The R28 filename/schema are retained capture-format identifiers, not proof of an old executable. Runtime log entries explicitly identify R29 with native=1, effective=2, requested=2, full_rr=1; no RR_FRAME_STOP entries were present. Raw GI and RR output RGB values were finite. Residual horizontal correlation was present in raw GI and input color in the sampled region. No matched-camera quantitative improvement percentage is claimed.

R30 extends the existing audited producer-local override to an explicit four-sample option. The same local count feeds native hit-buffer allocation and g_uRTDiffuseRayCount; it never changes the shader count alone. Refer to ../rr-gi-samples-r29/native-sample-path.asm for the audited renderer path. Existing binary identity and call-target checks remain in force.

INI: Samples=4 selects this comparison; Samples=2 reproduces R29; zero, missing, or other values use the native count. Restart between comparisons. Overrides apply only when native count is one and full RR is active, selected, and not stopped. RR off/partial and other native counts are preserved. Four-sample visual benefit, FPS cost, and VRAM cost require in-game comparison. Increasing samples alone might not eliminate structured noise.

All shaders, hair/skin guides, native GI radius, temporal behavior, glass code, and PT bridge behavior remain as in R29/R21. The only runtime changes from R29 are accepted request values and the diagnostic build marker. Tests cover both accepted counts, invalid configuration, all representative native counts, and RR-off behavior. Windows CI retains the baseline, hair/character and capture tests.
