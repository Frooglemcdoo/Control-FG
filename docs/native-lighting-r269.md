# R269 — isolate spotlight GI shadow visibility

R267 user result: no-point mode did not help; no-spot mode cleared hair speckles and most plant speckles. R268 PCF candidate did not improve either object. The screenshot showed PCF TEST bound, but does not prove the lamp executes PCF rather than VSM. No shadow algorithm is yet established as the root cause.

R269 changes the final static/dynamic spotlight shadow visibility to 1 in all eight exact native GI shader variants, before the zero-visibility early-out. This covers PCF and VSM without needing to guess the lamp's shadow mode. Each patch replaces one nine-DWORD MOVC with MOV 1 and four NOPs. The native light contribution, geometric tracing, near-plane gate, attenuation, cookie, BRDF, ray count and normalization remain. Direct spotlight shadows are unchanged. Secondary shadow attenuation is intentionally removed: brighter bounce and light leaks are expected and are not acceptable shipping behavior.

Normal launch selects the bypass diagnostic. F10 must report `GI SHADOW: BYPASS` after gameplay. `ControlTOH/Diagnostics/Start-GI-Shadow-Test.cmd` offers bypass, native baseline, and no-spot reference. All choices are restart-scoped. No persistent setting is written. The R268 PCF change is off by default and is excluded whenever a source/visibility diagnostic is active.

Keep the lamp intact, RR and RT indirect ON, Native Blend, GI RAW BYPASS, the same GI ray count, and FG OFF. Check both stationary and moving views of the original plant and Jesse's hair. Compare speckles rather than brightness alone. CAPTURE RR and PACKAGE RR retain the existing capture behavior.

If bypass improves the artifacts, shadow visibility is implicated, although increased illumination could mask noise and must be considered. If it does not, spotlight contribution remains implicated but final shadow attenuation is less likely; investigate the light/BRDF/sample distribution rather than extending PCF filtering. A bound CPU pipeline does not prove any particular GPU branch executed.

Validation: exact native SHA guards; instruction/context guards locate the final visibility blend inside the spotlight loop; only intended instruction bytes and DXBC checksum may differ; inverse edit recovers the entire native container. All 32 source/visibility variants are checked by the Windows C++ patcher, disassembled, and passed to WARP PSO creation. This validates construction, not in-game visual quality. DirectStorage, native blend reflections, map guard, GI phase sequence, and RR filters are unchanged.
