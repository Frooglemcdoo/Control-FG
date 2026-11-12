# R427 scene writer and native epoch observation

R425's 24 GI frames establish separate ordered ray/shade submissions on the same
queue. R426 already established live root compatibility and RTX pipeline creation.
Neither contains native writer intervals, current scene fence values or descriptor
provider retirement epochs. This probe is specifically for that missing evidence.

Exact captured renderer/D3D identities are required. Startup patches only:
- Material call 12491D -> C7BC0.
- Preparation thunk E9 A8F84 -> A81B0 (native thunk has already added RCX+18).
- Recording thunk E9 A8F74 -> A89C0 (same argument adjustment).
- GI scene join call 128F3B -> A8BF0; RCX is scene from renderer owner+220.

All four targets are verified before any patch is installed. Native call results
are void at these call sites. No per-draw hooks, resource readback, added GPU work,
waits, QPC timing, descriptor registry or rendering selection change is introduced.
Hooked writers count before the first GI sample, so an already-running writer is
visible when the window opens. Counters and serial boundaries are process-wide;
a serial change does not by itself prove that this specific scene was modified.
RAII balances counters on C++ exception unwind. After the shared bounded window
stops, hooks only forward native calls.

## Schema

All event payloads contain 12 unsigned integer fields. Kind 1/2/3/4/5/6/7/8/9
retain R425 semantics. Extra snapshots are restricted to GI phase events and
submissions with GI recording markers, not every engine submission.

- 20: frame, writer(0 material/1 preparation/2 recording), begin, serial,
  native argument, thread.
- 21: frame, scene pointer, join finished, thread.
- 22: parent phase/submit event, frame, point(1 ray/2 shade/3 before Execute/
  8 after Execute), serial before/after, three active writer counts, valid mask,
  SEH fault code, thread, same-frame scene join completed.
- 23: sample22 id, scene, scene fence wrapper/value/COM fence, two job-handle
  words, device fence wrapper/value/COM fence, material manager generation,
  thread-local native descriptor state.
- 24: sample22 id, provider(0 CBV/SRV/UAV,1 sampler), record pointer, COM heap,
  raw retirement epoch, raw generation, 32-bit cursor.
- 25: revision427, installed, rejection(1 identity/2 prepare/3 exchange),
  additional detail (failed patch index or rollback health).

Mask bits: 1 scene fence, 2 job handle, 4 device fence, 8 material manager,
16 resource heap, 32 sampler heap. Values are raw observations; a successful
pointer read is not ownership. Sampler raw fields are not used for admission.
The scene context is TLS and valid only for the joined native engine frame.
Submission on another thread can lack that scene context; do not fabricate it.
Sequence IDs order reservation/publication, not simultaneous CPU execution.
Use the serial-before/after and active counts to detect unstable samples.

## Native evidence

The exact-binary audit now additionally checks graphics submission124130 and
signal124152, both before material/job preparation. The scene fence is scene+D0.
The joined async path waits atA8C45, submits cleanupA8C61, increments/signals the
same scene fenceA8C85/A8C8F, then flushes graphicsA8C95 and tail-waitsA8CB4.
Async flag7EC040 gates this path. This identifies the native synchronization
boundary; it does not prove there are no other/transitive writers.

D3D descriptor allocator1AA00 selects a reusable record only after completed
native device timeline >= record+20 (1AAC6..1AACD), stamps current timeline in
cold fragment1ACD0, then refreshes the reserved material prefix. Cursor+18 is
32-bit (1AA86), while retirement+20 and generation+30 are64-bit. The existing
native-fence contract must be respected by any eventual delayed consumer.

## Validation and limits

26a2da6 Windows core job130811444864 passed the new hook fixture, in-flight writer,
exception unwind, concurrent publication, SEH fault, stopped-window and E8/E9
forwarding tests; all existing WARP lifetime/constant/SBT tests passed too.
Collector fixtures passed but its CI wrapper incorrectly compared an unset
PowerShell LASTEXITCODE with0; the wrapper now invokes a child PowerShell process
with an explicit exit code. Revalidation passed: core43107062347/job130813037381.
The full DLL and install-package build43107062344/job130813085051 also passed,
including package layout and restart-helper validation. Both built source
f31481425462267c387064c9a8f1adee59f80c17.

The parser records serial/epoch changes, missing/partial data and observed heap
identity differences. It never grants a scene lease or native suppression, even
if every sample is quiet. The final host contract still needs code evidence and
actual consumer submission tied to native resource retirement. No live guided
DXR transaction, noise improvement or FPS gain is claimed.

## Ready for the one missing user run

Verified and saved ControlTOH-GI-Scene-Check-R427.zip (6,940,577 bytes).
Library identity libfile_8d49d7437b808191a134d1adfa76604b.
ZIP SHA256 72410d1126b282ba6cc89fc1269f3d8d6e9c37c0faa00ca48da4540abba61783.
DLL SHA256 4563f507ab643cd8a9dd88c0a8a4ba7860f1c694d9a043f0a3c519d641317b84.
All18 external shader files match the delivered R426-v2 package. New launcher,
collector and README match source. CI archive checksum and ZIP integrity pass.

Next: user runs Start-GI-Scene-Check.cmd, loads any level with RT indirect
lighting enabled, waits15seconds and exits. Collect automatic GI-Scene-R427 ZIP.
No video, camera pose, hair/jaw comparison, R425 repeat or R426 repeat is needed.
Analyze available rows with tools/analyze-gi-scene-check.py and existing phase
analyzer, correlate exact event IDs/serials/fences/heap identities, and inspect
startup rejection/fault metadata before asking for anything else. Global late
overflow need not invalidate an already complete first24-frame prefix.
Live guided producer/consumer integration is still disabled. Do not call this
a visual fix, a proven immutable scene lease or a performance improvement.


## User capture reviewed: diagnostic interference confirmed

Input GI-Scene-R427-20261111-221809-74416-f3c3b4ad.zip. All three files read;
JSONL checksum verified and all8192 sequence IDs contiguous. Only two complete
GI pairs (1863,1864), twelve snapshots, no read faults, active writers or serial
changes between phases. Scene/device fence values and material generation stay
constant within each pair. Descriptor cursors append; the saved prefix does not
change identity.65 complete native graphics/compute scene-fence handshakes have
successful API returns, along with65 balanced cycles of each tracked writer.
This is CPU submission evidence, not GPU completion or lease authorization.

CRITICAL: startup RT_RECORD_R411_INSTALL ready=0. R427 installed an E9 observer
atA8F74 before RTOwnedRecord.Install, which validates the original bytes at the
same site. The observer caused owned recording to fail admission and use the
native body. Prior claims that R427 left the recording selection unchanged were
incorrect. Do not extrapolate these two pairs to the normal owned recorder.

The frame window also counted absolute engine-frame deltas rather than actual
GI frames. After1864, logging continued through a GI gap until2044; subsequent
GI phases were excluded by the24-frame delta rule. Unrelated Execute/Fence
events later filled the buffer. This is our instrumentation defect, not a user
capture mistake. Do not ask to rerun the original R427.

Revision2 wraps the exact job vtable slot6281D8 instead, forwarding to the
original A8F70 thunk, which still addsRCX+18 and reaches the owned E9 recorder.
A regression fixture installs both patches, calls through the job slot and
checks owned dispatch plus the argument adjustment. Uninstalling the observer
leaves the owned tail patch active. The capture now counts24 observed GI frames
across gaps and emits Execute metadata only for lists carrying GI markers.
Install event25 adds revision2 in data[4]; the collector requires it.
Windows validation and the R427-v2 package are pending after these corrections.
No live guided rendering, noise fix or FPS gain is claimed.
