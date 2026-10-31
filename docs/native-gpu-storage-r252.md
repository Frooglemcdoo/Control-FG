# R252: wider request coverage and an untrimmed session log

R247 storage behavior and R250 publication observation remain. R252 changes
measurement only. No demand, priority, budget, mip selection or rendering policy
is changed. The native selection routine itself is not instrumented by this build.

Install the whole package beside Control_DX12.exe. Run
Start-CPU-R252-NativeGPUStorage-4ms.cmd (4 ms, 16 partitions pinned).
Load the area, press F6 before approaching the three-sign board, and keep the
board visible while the delayed sign sharpens. Press F6 again exactly when the
third sign becomes sharp; within an active capture this adds a timestamp mark
without restarting. Wait at least 40 seconds from the first F6. Then run
Collect-Texture-Trace.cmd and send its Control-FG-Texture-Trace ZIP. Identify the
text on the delayed sign if readable. No recording software or Reference needed.
Use this new collector for this test: it preserves the full latest-session log,
including every capture, without the compact collector's middle truncation.
It snapshots the current file length; later appended bytes are not included.

The capture accepts requests for 30 seconds and allows 10 additional seconds
for completion, with 16,384 rows (32 times R251's limit). All intercepted texture
request attempts are eligible, including native fallback, not only accepted GPU
transfers. There is no sign-name filter. Names, texture numeric identity, archive
offset, byte size, native level and priority distinguish separate assets on one
board. Native levels may represent groups of mips: do not equate level numbers
with independently verified visible resolution. Repeated F6 presses within the
40-second capture log mark_us; after it closes a new press starts a new capture.

Row fields retain R251 timestamp semantics. request_us is native request-hook
entry, AFTER engine selection, not first visual demand. admitted_us is GPU
admission. submit_us precedes batch Submit; retired_us is observed transfer
retirement; publication_us is renderer consumer entry, not first visible use.
allocation_us is the preceding verified fresh allocation duration when available.
path=1 means GPU admitted; path=2 means native fallback or GPU failure retry;
path=0 means no route observed. result=1 means GPU transfer success, 2 native
retry, 3 terminal failure; 0 means no GPU retirement observed (normal for native).
Native publication is correlated only when the allocation hook supplied a fresh
container identity; other native requests are still listed with unknown completion.
The native observer has bounded 4096-slot/eight-probe matching and 30-second
identity expiry. A replacement identity supersedes the previous observation.
Zero times mean unobserved, not instantaneous. No extra resource ownership is held.

BEGIN reports rows, capacity, overflow, cumulative lock contention and cumulative
native tracking drops (including missing verified container identities). END
records delimit complete dumps. Check these and unmatched/zero fields before
claiming coverage. Names remain capped at 511 bytes with truncation reported.
Finite storage and try-lock observation cannot guarantee literally every event:
this removes the observed 512-row bottleneck and collector truncation, but loss
must be checked on the actual run. It does not see already-resident textures,
requests through unhooked engine paths, upstream selection or actual visible use.
F6 marks supply observation timing, not automatic object-to-texture identification.

Rows flush from the worker after 40 seconds. Snapshot copying and log writing
can add diagnostic overhead at that point; they are not normal loading costs.
The two fixed row buffers use roughly 20 MiB of CPU memory, not GPU VRAM.
Existing storage and graphics regressions remain; new coverage tests exercise
native/GPU/retry/full-queue requests and a 5 MiB untrimmed live-log copy.
