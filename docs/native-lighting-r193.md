# R193: bounded primary reflection sequence

Experimental quality test following the R192 four-frame CPU-provider cycle.
No quality or performance improvement is established yet.

Install the experimental DLL beside Control_DX12.exe, preserving NVIDIA runtime
files. Enable RR, choose NATIVE BLEND and SPEC 4, disable frame generation.
Hold still at the painting for about 30 seconds. Send a screenshot and the
Lighting-R193-*.log, and report whether the stationary splatter improves or
turns into moving shimmer. Check the elevator afterward if the painting helps.
No F9 suite or large image archive is required. Exact cross-run camera matching
is unnecessary; do not infer a numeric quality/performance gain from unmatched
screenshots. Restore the R192 DLL to revert this rendering experiment.

## Exact rendering change

Only the SHA-matched native reflection ray library is substituted. The primary
sample index changes from g_uTemporalFrame * SPEC + sample to
(g_uCurrentFrame & 255) * SPEC + sample. The latter field already occupies
sys_constants register 69.z, byte 1112; no resource binding or constant-buffer
layout changes. If the GPU counter advances normally, the primary sequence has
256 frame phases, or 1024 primary indices at SPEC4. These are index support
counts, not independent rays or SPEC16-equivalent quality.

The maximum index at SPEC16 is 4095, avoiding an indefinitely growing float
phase input. Original native float arithmetic, blue-noise lookup, GGX mapping,
radial truncation, rejection test, fallback sequence and memoization, ray count,
resolve lighting, clamp and RR guides remain. No R185 modular phase rewrite is
enabled. No shared provider is written. The first change can compile to an
additional integer mask and extraction from the existing constant-buffer load;
actual GPU cost must be measured, not assumed zero.

The narrow source transformation is reversible exactly to the delivered R176
ray-library optimization. Tests cover rejection of unexpected source, index
coverage across starts and uint32 wrap, and exact runtime shader selection,
association rebasing and native fallback. DXIL assembly/validation verifies the
actual replacement. These tests are not an in-game DXR quality evaluation.

## Evidence limits and logging

R192 observed g_uTemporalFrame cycling 3,0,1,2 for 1024 consecutive CPU-provider
reads at SPEC4. This build is a causal sampling experiment, not a claim that
R192 measured GPU constants. Its bounded probe reads both g_uTemporalFrame and
g_uCurrentFrame near reflection filtering, authenticates and rechecks them,
and logs at most 1024 eligible SPEC4 frames. It adds no GPU readback or waits.
The analyzer reports whether current-frame CPU values advance continuously.
A progressing CPU provider still does not prove the raygen GPU field is filled
or consumed as expected. A failed match or replacement creates the native
library and is reported, rather than guessing another variant.

Look for SPEC_LONG_SEQUENCE build=R193 applied=1 native_fallback=0 in the log.
Other unchanged paths can retain older build labels. The visible build/log name
is R193. Periodicity, changing noise texels and RR history may still limit
convergence. If quality worsens, retain the log and revert; do not raise SPEC to
hide the result. The original R191 F9 suite remains available but is not needed
for this run.
