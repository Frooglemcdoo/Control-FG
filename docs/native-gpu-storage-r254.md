# R254: admit higher-detail upgrades through the native budget gate

R253 recorded 2,951 eligible upgrade evaluations whose delta differed from the
native global minimum, across 98 tracked sign-related textures. 2,865 still had
time available in the 4 ms budget. All 35 observed budget-gate calls succeeded.
The affected examples included Luck & Probability, Central Research,
Parapsychology Division, Parakinesiology and Cafeteria. Observations were incomplete
because try-lock contention drops were reported; these are sampled evaluations,
not measured visible-delay totals. Individual signs were reevaluated about every
0.53 seconds with 16 partitions in that run.

## Change

With CONTROLFG_TEXTURE_UPGRADE_GATE=1, replace the aligned eight-byte sequence at
Steam renderer RVA 15AA90:

- Original: CMP EAX,[global minimum]; JNE skip (3B 05 72 81 6A 00 75 2F).
- Replacement: TEST EAX,EAX; four NOPs; JNS skip (85 C0 90 90 90 90 79 2F).

This applies only on the existing upgrade-direction, non-bypass branch. A negative
native delta (wanted level below resident level) can reach native request routine
15AB30 without equaling the single global minimum. Zero and positive deltas skip.
The budget gate still validates level state and available time before allocating
or requesting anything. The prior 25 MiB pending-byte comparison remains native.
Detail targets, native ownership, unloading, scan partitions, resource states,
GPU transfer lanes and native completion remain unchanged. No signs are hardcoded
into the policy: eligible texture upgrades generally can benefit.

This changes upgrade ordering within the existing scan/budget. A less urgent
upgrade can now use budget before a later more urgent one. Therefore faster
arrival and frame pacing require live validation; this is not a guarantee against
all starvation or a guarantee of unchanged CPU/VRAM use. Loaded textures may
become resident sooner; no additional GPU staging arena is introduced.

The existing exact renderer/runtime/D3D hashes are required before installation.
The eight-byte site must match exactly and be aligned. Exchange uses an aligned
compare/exchange, restores page protection and flushes the instruction cache.
Failed post-write validation attempts rollback. This is startup-only, not a live
menu toggle. Set CONTROLFG_TEXTURE_UPGRADE_GATE=0 before launch to retain the
original gate. The prewarm/transfer baseline remains R247.

## Test

Install the full package beside Control_DX12.exe. Launch
Start-CPU-R254-NativeGPUStorage-4ms.cmd. The launcher enables the gate and retains
4 ms / 16 pinned partitions. Approach the affected signs and other slow textures.
Press F6 before approaching; press again when the delayed sign sharpens. Wait
40 seconds from the first press, then run Collect-Texture-Trace.cmd and send its
ZIP along with texture-arrival and stutter observations. No Reference run or video
is required. F6 measurement remains optional for the policy itself.

Expect TEXTURE_UPGRADE_R254 installed=1, negative_delta=eligible, budget=native,
pending_limit=native. The R253 request and selection traces remain with R254
prefixes for checking whether previously blocked upgrades now issue requests.
The collector now resolves default paths inside the script body; its CMD wrapper
also passes its directory explicitly. The full session log is preserved.

## Validation

Exact-image static audit includes the original eight-byte gate and surrounding
selection evidence. An executable Windows fixture runs the real original and
replacement instruction sequences over negative, zero and positive deltas, checks
exact-byte/alignment rejection, duplicate exchange rejection and rollback. Existing
selection-wrapper, ownership, prewarm, 72-mip GPU content and graphics tests remain.
Collector tests now include a fresh Windows PowerShell -File process with omitted
OutputDirectory and a script path containing spaces, in addition to full-copy
byte equality with an open writer. No live-game improvement is claimed by CI.
