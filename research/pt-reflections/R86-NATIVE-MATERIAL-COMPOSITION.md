# Native primary material composition

R85 evidence: 3,840 logged accumulated early reservations, 62,400 skipped draws
with unavailable early cameras, 600 missing sampler-table draws; these are not
frame counts or proof of which photographed pixels were affected. RR submissions
were admitted, but the recorded specular hit-distance bindings were null.

The nine reviewed surface shaders do not share one albedo formula. Captured
c2b522405006 blends material colors at DWORD617, 11f146abfdc0 applies detail at537,
77ee9ce88a8d blends/detail at733, and 3755ae5f574c/68b095104f50 use texture-only
color. The R85 PT decoder always multiplies texture0 by material words13..15.
Replacing their final RGB bypasses the true native material color.

R86 evaluates the primary diffuse lobe with unit texture color (kd=1-F0), while
retaining secondary materials. The estimator's proposal PDFs and RIS target both
use this same neutral primary material. Primary-lobe splitting retains separate
diffuse/specular contributions. The raster combine evaluates D*nativeColor+S.
This is linear primary recoloring with no division by zero or low albedo. The
native material operand is taken from each exact hash-reviewed MAD instruction.

Final packets grow from32 to64 bytes: summary/coverage0, geometry16, diffuse32,
specular48. All final/history strides and RR coverage accesses change together;
raw64 and temporal signal32 layouts remain. Fresh RR passes current lobes exactly;
standalone spatial reconstruction writes both resolved lobes. No extra rays/pass.
Extra memory is32 bytes per final pixel and existing capacity caps remain.

Validation gates: nine captured shader patches and checksums; optimized Windows
WARP compositor including colored/black material channels, separate specular,
coverage, bounds and guard isolation; reconstruction lobe-sum checks across fresh
and history paths; RR guide stride regression; full DLL/shader build. In-game
visual and performance validation remains required.

This does not correct secondary material evaluation or recover complete layered
albedo for RR guides. Those require additional material data plumbing. No claim
that the missing lights or reflection smearing is fixed. NVIDIA reflection guide
requirements: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
