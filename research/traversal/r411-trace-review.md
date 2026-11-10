# R411 traversal trace: first scheduling attribution

Source: ControlTraversal-68220-20261110-074124, PID 57552, R411 Clean.
Complete archive downloaded from user-provided Drive file; ZIP CRC verified.
ETL size 10666115072 bytes. Frame CSV identical to prior user attachment.
Run bounds QPC 2521412831390..2523216175479, frequency 10000000.

Scanned 94377852 records, including 26182642 context switches and 31816627
stack-walk records. Fast system-event decoding cross-checked against dissect.etl
for 1882 records from four separated buffers, including payload equality.
Narrow windows retained locally; raw trace and parsed large datasets not in git.

Thread 36148 is the startup/main game thread. Native renderer main-loop stacks
confirm its rendering work. Using ContextSwitch old/new thread IDs and clipped
switch-out/switch-in pairs, the three worst present intervals contain about
74.82, 74.49 and 63.59 ms off CPU respectively. Smooth 100 ms windows ending at
65 and 90 seconds after the beep contain 5.11 and 7.33 ms off CPU.
Most stall-window off-CPU time follows wait reason 27, state 5 (WrResource,
waiting): 818/680/642 short waits, totaling 74.28/74.10/62.66 ms. This is not a
single long wait, and is not a measurement of native preparation computation.
The window durations are present-to-present, not simulation/FG timings.

Exact kernel image retrieved from Microsoft symbol server using recorded
TimeDateStamp 3035B183 and SizeOfImage 1450000. Its CodeView identity:
ntkrnlmp.pdb A7EFEEC342CAE4CB37346409FADF4C6A age 1. Matching public symbols
resolve sampled stacks through EtwpLogContextSwapEvent, EtwpTraceStackWalk,
EtwpExpandedStackWalk, RtlWalkFrameChain and resource waiting via
ExAcquireFastResourceExclusive / ExpAcquireFastResourceExclusiveSlow /
ExpWaitForFastResource. Main-thread sample counts with an ETW stack in the
three stall windows: 14/51, 12/39, 11/37. These are sample counts, NOT time
fractions or proof that recording caused all stalls. Similar ETW stack activity
also appears in smooth windows. Kernel symbol identity and sample classification
are separate from determining which resource was contended.

User explicitly reports traversal dips without recording; recording worsened
performance. Keep those observations separate. This trace confirms observer
work but cannot establish its counterfactual cost. Do not call the underlying
game problem solved or dismiss it as a recorder artifact.

Disk completions in the worst windows mostly name System issuing threads.
Control-issued reads occur in smooth windows too. Without file-object mapping,
operation latency, resource ownership and dependency attribution, do not blame
texture streaming or the recorder's file writes from counts alone. Steam overlay,
XInput, Reflex and renderer frames occur in samples; mere presence is not cause.
The main-thread wait stack resolves at the exact switch-in timestamp, not the
switch-out timestamp. In the first worst interval all 844 switch-ins match
main-thread stack records. Do not use nearest timestamps or other threads.

Follow-up resolves the wait caller and compares sustained slow sections below. Do not modify lighting,
ray counts, native job scheduling or descriptor lifetime based on these results.
No new user run is needed for continued inspection of this trace.

Wait-reason definition:
https://learn.microsoft.com/en-us/dotnet/api/microsoft.windows.eventtracing.cpu.kernelwaitreason


## Follow-up: message waits and native preparation join

Matching Microsoft public symbols for win32u, user32, win32kbase and win32kfull
resolve the wait path through PeekMessageW/GetMessageW, NtUserPeekMessage,
win32kbase!EnterCrit and ExAcquireFastResourceExclusive. Steam's
GameOverlayRenderer64 hook is on this path. This establishes Windows UI resource
contention, not the lock owner or proof that Steam caused it.

Message-call waits account for 73.76, 73.79 and 61.89 ms of the three worst
present intervals (118.17, 113.98 and 101.44 ms). Whole-run 10-second bins show
about 0.8–1.2 ms of message waiting per present in several slow sections versus
about 0.1–0.4 ms in smoother sections. This explains only part of the sustained
frame-time difference; isolated worst hitches are not representative of all dips.

For bins 60–70 and 80–90 seconds, 2,147 sampled stacks include the native RT
preparation join at renderer+0x124053, across 1,363 presents / 20 seconds.
For bins 100–110, 120–130, 160–170 and 170–180 seconds, 6,074 such samples
occur across 2,085 presents / 40 seconds. That is about 1.58 versus 2.91 samples
per present, or 107 versus 152 samples per second. These are statistical CPU
samples, not milliseconds or directly recoverable FPS.

Inspected own IPs within inclusive join stacks frequently land in runtime TLS
thread-role lookup, getNextJob, and empty-queue checks. Native waitJob can execute
other jobs, so inclusive join counts alone must not be called pure idle time.
The observed queue checks motivate inspecting the outstanding preparation worker,
not changing spin behavior or blindly adding workers.

Next: attribute worker samples from the same trace to the preparation body and
its callees. A Steam-overlay-off run without the recorder is a useful reversible
isolation test, but neither overlay causality nor a performance fix is established.
No runtime patch has been made from these findings.


## Preparation worker follow-up

A second full-trace scan retained game CPU samples plus stacks containing the
renderer preparation/nearby helper range, then joined samples by exact timestamp
and thread ID. Nearby helpers also include unrelated renderer work; broad range
totals must not be labeled preparation totals. Exact renderer+0xA83DD return
frames isolate the object query.

The hottest own instruction in the selected worker stacks is dxgi+0xA613C
(198 smooth / 508 slow samples across 20 / 40 seconds). Disassembly of the
packaged R411 DLL maps this to the owned selection object's flag-filter loop,
immediately following the read of object+0xC. The native partial filter at
renderer+0x1660A8 also grows (54 / 217 samples); caller dxgi+0xA60DE corresponds
to the return from api.partial. Neither location is diagnostic logging.
Instruction sampling can skid, so this does not prove a specific cache miss or
quantify recoverable latency. The route/scene workload differs between windows.
Do not infer regression relative to native selection without a matched comparison.
This is evidence to prioritize the owned/native scene-query comparison, rather
than reduce lighting rays or change scheduler waits.


## User correction and R412 comparison

User reports Steam Overlay was already disabled and supplied its settings
screenshot. No overlay-toggle run is requested. The captured Steam hook does
not establish that the overlay was enabled or caused the Windows UI waits.

R412 isolates scene selection with two clean binaries from the same source.
Native omits the owned selection install entirely; Owned retains it. Other
rendering hooks and ray counts remain. This is a causal comparison, not a
performance fix. Earlier oracle tests established parity and allocation counts,
not FPS. Use a matched native/owned/native route without the heavy recorder.
