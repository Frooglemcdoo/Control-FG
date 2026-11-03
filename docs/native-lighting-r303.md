# R303 — scene-instance to submitted geometry metadata

Close Control and extract the package beside Control_DX12.exe, preserving folders.
Run ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd with RT indirect lighting on.
Move through several rooms for about two minutes, exit normally, and send the
automatic ZIP. No capture buttons, fixed camera, zoom or GI RAW setting are needed.

R302 completed four native instance-array copies and 40 paired geometry samples.
The first eight logged instance records retain their transforms while their BLAS
addresses change in the last snapshot. R303 therefore attaches a receipt to the
specific captured TLAS journal event. In submission order, it retains descriptor
metadata after that event and before later commands change the address table.
It handles earlier AS copies and replaces the receipt on command-list replay.
A failed submission clears the receipt. CPU instance mapping still waits for the
existing recording-seal/GPU-completion gate and metadata publication.

Every captured instance is checked against this retained descriptor table. Logs
report matched, missing, null BLAS, wrong AS type and absent geometry metadata.
All four bounded instance captures and existing geometry captures run together.
Existing GPU capture limits remain; each receipt can additionally retain up to
65,536 AS metadata records, shared immutable geometry descriptors, and no new GPU
resource ownership. These diagnostic CPU copies can add capture-time overhead.

This proves descriptor lookup at the captured event, not live AS allocation
identity, matching vertex/index bytes, accepted-hit identity or material mapping.
Historical address entries must not be mistaken for proven live geometry. Native
lighting remains unchanged and replacement GI remains disabled. The purpose is
to establish the next connection needed by the GI implementation.

Validation covers same-list writes after the captured event, copy lineage,
immutable earlier snapshots, replay, failed replay clearing and missing/null/type
classification. The Windows GPU fixture combines an actual instance-array copy
with a matching metadata submission and validates the result after fence/reset.
