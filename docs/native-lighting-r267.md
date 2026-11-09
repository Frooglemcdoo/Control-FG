# R267: native indirect-light source isolation

This build diagnoses the speckles; it is not a claimed fix. Normal launch
retains the R266 rendering baseline, including the accepted R264 GI sequence.

`ControlTOH/Diagnostics/Start-GI-Source-Probe.cmd` offers full GI, no point-light
GI, no spot-light GI, and no local-light GI. Close the game between selections.
Each selection lasts only for that process. The lamp's direct illumination,
ray count, shadow evaluation, material sampling, baked illumination, and RR
configuration are untouched. Disabling a contribution removes its indirect
brightness intentionally; it is not a performance optimization.

Keep the lamp intact. Keep RT indirect lighting ON, RR ON, Native Blend,
GI RAW BYPASS, the same sample count, and FG OFF. Use the existing affected
plant/hair view; no whole-game survey is needed. In F10, the hint must identify
the selected GI SOURCE. A message saying no matching shader has bound means
the result is not usable. `GI_SOURCE_R267 prepare` records shader identity and
PSO creation; `bound` records CPU command recording, not GPU completion.

The diagnostic edits only the point and/or spot RGB accumulation MAD inside
the eight exact native GI shaders. It replaces each selected update with a
MOV of the previous accumulator plus NOP padding. Resource bindings, loops,
sample weights, final normalization and all other shader bytes are preserved,
apart from the container checksum. Unknown shader identities remain native.
Native cached pipeline creation passes through the existing observation hook.

This distinguishes light-source contributions before they are combined, which
the prior combined-GI captures cannot do. Brightness loss alone is not proof
of a bug. A correction still requires checking sampling/visibility/weighting in
the identified path and retaining legitimate bounced illumination.

Validation: generator verifies the 24 patches against captured full shader
identities, unchanged binding reflection and unrelated bytes. Windows test
checks C++ patch agreement, modified-instruction rejection, D3D disassembly
and WARP compute pipeline creation. GPU visual validation remains outstanding.
