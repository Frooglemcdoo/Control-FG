# R128: bounded native GPU hit capture

R127 succeeded on the RTX 5090: the independent pipeline linked and all 12,802
native table records translated on the CPU. R128 adds the GPU capture itself.
This is a diagnostic build, not a lighting fix.

## Test

Close Control to install beside Control_DX12.exe. Preserve NVIDIA runtimes and
ControlFGStreamline. Load the affected room with RT Contact Shadows on, samples 1,
FG and glass enhancements off. Face the affected light/table/floor. Select BOTH,
keep that view for a few seconds, then run for 70 seconds. SAVE LOG and send the
Lighting-R128 log. No screenshots or restart between modes are required.

There is one capture per game session. Changing modes afterward still works,
but selecting BOTH again does not overwrite or reallocate the capture resources.

## What is recorded

A 64-by-64 grid samples the native image. The input shader is derived from the
hash-verified R97 centered shader. It retains the native ray-construction
instructions, biased origin, selected spotlight ID and endpoint arithmetic.
It writes only a private 48-byte input record, never the game's output textures.
Unselected pixels produce invalid records.

The GPU reads the currently bound native shader table and translates identifiers
into a private table, retaining all 96-byte record positions and all 64 material
argument bytes. Null records retain their argument bytes. Unknown identifiers
or invalid layouts set a status flag that prevents diagnostic ray traversal.
The independent pipeline then traces at most 4,096 rays against the same bound
TLAS, using the native alpha-test handler and original material/sampler tables.

The recording changes root table 0 and pipeline state locally, then restores
both. It preserves native descriptor heaps, root constants, material tables,
root signature and gameplay outputs. All private tables are fresh allocations
from the same verified engine descriptor heap.

Readback occurs only after an explicit fence on the queue that actually submits
the matching command-list generation. The asynchronous log worker exports the
80-byte records in bounded batches. There are no render-thread GPU waits.
Reset before submission and replay before readback are rejected. Resources are
retained for the process lifetime, including failure paths, with one fixed
8 MiB table and less than 1 MiB of additional buffers (plus allocation alignment).

## Log results

- `rt_hit_gpu_pipelines ready=1`: native input and translation pipelines prepared.
- `rt_hit_gpu_recorded`: one bounded capture was recorded; not completion proof.
- `rt_hit_gpu_complete`: the exact submission fence completed, with GPU table counts.
- `rt_hit_gpu_record`: integer identity, barycentrics, hit kind, distance and ray inputs.
- `rt_hit_gpu_export_complete`: all result slots examined and valid records logged.
- `rt_hit_gpu_pending` / `rt_hit_gpu_failed`: explains why a capture could not finish.

A successful build and WARP compute tests do not prove live DXR traversal on the
user's GPU. The R128 log must establish that. Existing centered shadows, GI
filtering, material guides, glass and the 60% clamp remain unchanged.
