# R296 — bounded native index snapshots

Extract beside Control_DX12.exe preserving folders. Close the game first and run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd. Keep RT indirect lighting on,
load the usual save, move around for about one minute, exit normally, and send
the automatically packaged diagnostics ZIP. No fixed camera or capture hotkeys.

R295 reported bindings for all 93,481 observed AS builds, zero missing geometry
ranges and zero lifetime quarantine/fence failures. It rejected 17,352 of 391,339
index range checks (4.434%) under the old DWORD-copy planner. Those aggregate
counts alone did not identify padding versus dispatch-limit rejection.

R296 adds an exact uint16 GPU read path. It reads the last two-byte element without
reading past the resource boundary; only the private destination is DWORD padded.
Two-dimensional dispatch supports ranges up to 256 MiB in the standalone kernel,
subject to separate ownership/allocation limits. It requires shader model 6.2 and
native 16-bit shader operations, checked before pipeline creation. Descriptor
heaps are never switched; native compute roots/arguments/PSO are restored.

The live probe copies at most 32 samples: up to 16 ordinary ranges and 16 rejected
by the legacy planner. Each sample is at most 256 KiB; larger native index ranges
use a tail window, explicitly labelled in the log. At most one sample is attached
to a command-list recording. The session reservation cap is 128 MiB, charging
actual allocation sizes for the retained committed source, private output and
readback. Reservation counters include unsuccessful attempts and are not live
VRAM measurements. Pipeline/driver bookkeeping and the existing owner inventory
have separate costs. Placed/reserved sources are not copied by this probe.

Copies are inserted immediately before the native AS build. Source state is not
changed. Unknown local state relies on the native DXR input contract requiring
NON_PIXEL_SHADER_RESOURCE; pending/invalid/conflicting local state is rejected.
The source, output, readback, native bindings and copy pipeline remain owned until
recording Reset/destruction and private-fence completion. Future base-device queue
creation is intercepted; loss of observed coverage quarantines retained owners.
Batch overflow also quarantines owners. Native execution is never suppressed.

After completion and sealing, up to two samples per poll are mapped and summarized
outside the observation lock. There is no CPU wait for GPU completion. Logs contain
hashes, maximum index, indices outside the declared vertex range and output-padding
checks, rather than large buffer dumps. This validates transport and integration,
not reconstruction of the entire scene. No replacement GI or speckle fix is enabled.

New log markers:
- GI_INDEX_INIT_R296: capability/pipeline initialization.
- GI_INDEX_PLAN_R296: separate padding, dispatch-limit and invalid-range counts.
- GI_INDEX_STATUS_R296: recorded/read samples, failures and reservation counters.
- GI_INDEX_COPY_R296: completed sample summary. raw_reject: 0=none, 1=range,
  2=overflow, 3=padding outside source, 4=old dispatch limit.

Normal launch leaves the probe off. The launcher enables GI_METADATA, GI_BINDINGS
and GI_INDEX_SNAPSHOTS; existing lighting and DirectStorage settings are preserved.
This diagnostic is not a normal-launch performance benchmark.
