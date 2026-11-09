# R29: native GI sample comparison

R28 evidence places the horizontal blinds pattern in raw GI and scene color before RR. This motivates a producer sampling comparison; it does not prove that more samples will fix the residual RR pattern.

Audited renderer SHA256: `ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433`.

The option initialized at RVA 0x6280 is named `rt:Sample Count Diffuse` (string RVA 0x62ef68). Initial integer fields at +0xb0 are (1,1,16,1). The getter at 0x2dac0 reads +0xb0. The producer calls it at 0x12ba17 and stores its return in [rbp+0x408]. That same local is passed to hit-buffer allocation at 0x12ba70 and to the g_uRTDiffuseRayCount provider setter at 0x12bf07. Function 0xa2cd0 clamps the allocation count to at least one and writes it into the descriptors for material ID, normal/texcoord, position/texcoord, and optional additional hit buffers. Consequently a provider-only override would not be safe.

The experiment changes only that callsite's return, only when native count is one and full RR is selected and running. A packaged INI requests two samples; missing/invalid/zero configuration preserves baseline. Counts other than one, partial RR, and RR off are preserved. The registered option is never written. Existing verified-module checks, call-target verification, relay allocation, and rollback apply.

This increases ray work and hit-buffer storage. It is not a performance-neutral fix. Native shader implementation and in-game visual/performance results remain unverified; the engine exposes the count but that alone does not prove quality improvement. Compare in motion with Samples=2 versus Samples=0, restarting between changes. Logs report native/effective counts. No temporal smoothing, GI radius, guide shader, glass, or PT bridge changes are included.

Validation: portable policy test covers invalid/zero/high native counts, RR-off preservation and one-to-two selection. Existing clean-baseline and character matching tests remain required; Windows CI compiles the full hook and runs hair/character/capture tests. R28 raw-GI capture remains optional and retains its R28 schema because the byte layout is unchanged.
