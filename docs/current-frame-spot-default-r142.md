# R142 — current-frame spotlight shadows by default

After R141 testing, the user reports History Guard helps but does not completely
remove the motion flicker. No History fixes the reported issue, with no visible
difference in smoothness or noise. R142 promotes that exact tested behavior.

**Default (Fixed)** now automatically uses current-frame spotlight visibility,
retaining centered spatial filtering, native point-light history, the R139
direct-light surface-normal correction and R134 authored-light clipping.
No shader bytecode, sampling count, resources or dispatch dimensions change.
This does not disable RR, GI filtering or temporal processing elsewhere.

**History On** restores the R139/R140 default for comparison. **History Guard**
remains available as the partially effective R141 experiment; it is not the
preferred fix. RR Off and Screen AO Off now also retain the new current-frame
spotlight policy so each isolates only its named feature. Native and Both retain
their previous meanings. Raw Shadows additionally removes spatial filtering.

Install with Control closed, copying the package beside Control_DX12.exe while
retaining existing NVIDIA runtimes. Default activates when its verified shader
and sample prerequisites are ready. No manual No History selection is needed.
Continue checking other rooms, thin shadows, close-light softness and motion.

F9 still performs 25 distinct comparisons (up to 75 PNGs), including the new
default and previous History On baseline. It restores the selected mode. Save
Log plus a short video is useful for motion-only bugs; PNGs alone do not prove
motion stability. The startup log identifies R142 and current-frame spotlight
policy. Settings are temporary diagnostic overrides, not saved user preferences.

Validation covers unique buttons/capture modes, preserved normal/clip routing,
RR-off isolation, default readiness and the actual history/spatial PSO selection.
The accepted shader sidecars remain unchanged. The tested symptom is reported
resolved; broad release compatibility and performance validation remain open.
