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
with an explicit exit code. Revalidation is pending at this checkpoint.

The parser records serial/epoch changes, missing/partial data and observed heap
identity differences. It never grants a scene lease or native suppression, even
if every sample is quiet. The final host contract still needs code evidence and
actual consumer submission tied to native resource retirement. No live guided
DXR transaction, noise improvement or FPS gain is claimed.
