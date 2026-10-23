# R121: dispatch-correlated shader-table audit

R120 found an 8 MiB DEFAULT-heap shader-table buffer populated by UPLOAD copies.
That buffer is reused for different ray-tracing passes. R120's samples only
covered prefixes, with differing identifiers; they were not full dispatch tables.

R121 mirrors existing upload copies into a fixed 8 MiB CPU buffer, including tail
chunks. It tracks command-list reset generations, resource base/extent, and exact
byte coverage. A new offset-zero upload invalidates prior coverage. Missing copy
observations, unreadable sources and unstable reads invalidate evidence. Copies
from different command lists are never joined; cross-list queue tracking remains
unsupported and will be reported as incomplete rather than guessed.

At the recognized native spotlight DispatchRays, it checks coverage for that
exact table size and counts identifiers against the bound state's two native hit
groups. Null identifiers are counted separately, preserved, and are not proof of
an invalid shader table. Up to four records from a fully covered, identifier-matched
table are logged. This is CPU recording evidence, not proof of GPU execution or
actual ray-hit geometry. No render shaders, descriptors, or GPU commands change.

Work stops after the first validated table, 64 MiB of CPU copy traffic, or the
existing audit timeout. There are no added GPU waits or readbacks. Only one
command-list generation is mirrored at a time, with an explicit fail-closed policy.
The existing shadow/glass shaders, GI filter, material guides and clamp are retained.

## Test

Close Control to install. Copy the package beside Control_DX12.exe, preserving
NVIDIA runtimes and ControlFGStreamline. Load an affected area with RT Contact
Shadows on, RT samples 1, FG and glass enhancements off. Select BOTH, play for
70 seconds, then SAVE LOG. Send the Lighting-R121 log. No new screenshots needed.

Validation covers partial/split copies, coverage gaps, command-list resets,
unknown/null identifiers, lost observations, CPU budgets, and native copy forwarding
with unchanged upload and GPU destination bytes on Windows WARP.
