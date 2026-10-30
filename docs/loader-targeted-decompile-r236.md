# Targeted loader reconstruction after R236

## Decision from the live run

Prioritize native synchronous archive I/O and the scheduling/ownership contracts around it. Do not start with interleave policy, a larger texture budget, a replacement renderer, or full-game source reconstruction. Keep the accepted visual baseline and 8 ms / 16 pinned partitions.

Evidence: latest session `probe-20261030-195947-890-57940.log`, ending at report present 7740. The archive also contains a short earlier R236 session and an R235 session; neither is merged into the measurements below. The shorter R236 log ends around present 59 without enough evidence to classify why it ended.

| Scope / measurement | Result |
|---|---:|
| Native ReadFile calls | 16,558 |
| ReadFile bytes | 3,579,960,518 |
| ReadFile total / maximum | 36,230.185 / 204.642 ms |
| Complete PackFile read total | 36,269.301 ms |
| Buffer waits total / maximum | 823.074 / 267.756 ms |
| Interleave total | 1.341 ms |
| Native location lookup total / maximum | 74.146 / 22.972 ms |
| Observed range comparisons equal / unequal | 4,830 / 0 |
| Native read failures | 0 |
| Resolved candidate tracker contention drops / replacements | 0 / 19 |
| Entries excluded by per-batch observation cap | 7,995 |

ReadFile accounts for 99.892% of measured PackFile read duration; the latter includes the former. This identifies the synchronous API as the dominant measured archive-read duration, NOT physical SSD latency or CPU execution time. Scheduling, OS cache and storage-stack effects remain included. These durations are not additive frame costs, do not establish an FPS gain, and are not controlled comparisons to R235.

Buffer waits are secondary in total but can still cause individual stalls. The maximum buffer wait and maximum read need not belong to the same request. Neither event has been correlated to a displayed-frame hitch yet.

Large startup/backlog bursts dominate whole-run queue averages. In the final two report intervals (present 6840 to 7740), all 1,412 dispatched workers had matching range observations: read time averaged 0.546 ms and resolved lead averaged 3.313 ms. From present 4440 to 7740, resolved lead averaged 38.637 ms over 3,285 matched candidates, showing workload dependence. A mean does not guarantee a deadline for any individual request. Do not use the prior 15 ms admission mean as guaranteed usable I/O lead.

## Exact reconstruction boundary

All addresses are RVAs in the supplied Steam `rl_rmdwin10_f.dll`, SHA256 `a79234e4b34c2488deaac3459e8054be206b5af78a37723d834c1cbd826ce484`.

| Portion | Functions | Why needed |
|---|---|---|
| Admission and selection | requestFilePartialLoad FA2B0; enqueue FF080; selectNextFile_New FC5E0; processFileLoads FC9B0 | Request IDs, priorities, batching, dispatch order and safe handoff after locks |
| Archive mapping and reads | getFileLocationInfo 18F760; PackFileSystem::openFile 18D270; PackFileSystem::readFile 18E3F0; PackFile::readFile 196640; WinFileSystem::openFile 192630; WinFileSystem::readFile 193390 | Exact bytes, archive identity, handle access/position semantics, native fallback |
| Ownership and completion | loadFileToOtherThread FB440; acquireBuffers F9E80; processCompletedFileLoads FCFB0; releaseBuffers F9FE0 | Keep native buffer release, callback publication and completion-thread behavior correct |
| Cancellation and archive lifetime | cancelRequest(ID) FB1A0; unmountPackFile 18BDC0; unmountAllPackFiles 18BEE0; reloadPackFiles 18BF60 | Abandoned I/O, archive identity changes and lifetime boundaries |

R236 exercised the new selector; old_select remained zero. The old selector FBDC0 stays native and is outside the first replacement scope. Full-file and loader-thread request variants also stay native initially. RT preparation is a later independent target.

## Reconstructed behavior from disassembly

The following is manually reconstructed behavioral pseudocode, not recovered original source and not an executable replacement. Names below describe observed roles; unknown details are left explicit.

### Queue and resolution

1. FF080 assigns an ID and constructs a 0x188-byte request in a batch or pending list. Relevant fields: listener +138, user data +140, supplied buffer +148, requested bytes +150, partial offset +158, completion thread +160, priority +16C, ID +178.
2. FC5E0 holds the selection-list lock (+518), takes the pending-list lock (+4F8), moves new entries into the selection list (+508), then releases the pending-list lock.
3. FC853 calls 18F760 for new entries while the selection lock remains held. Lookup outputs numeric PackFile identity +120, sorting location +128, file size +130.
4. Sorting location is `archiveEntryOffset + packBias(+288)`. The actual read offset is `sortingLocation - packBias + partialOffset`; validate the partial range against file size and integer overflow. R236 verified equality for the observed subset only.
5. The selector sorts/selects one record, copies it to the caller, removes it from the list and decrements pending count. The selection lock is released before returning.
6. FC9B0 invokes pre-load listeners, executes FB440 (or FBAE0 for completion-thread value 6), then invokes post-load listeners and updates interleave clients.

The current safe observation point under the selection lock is NOT a place for file opens or DirectStorage submission. A future handoff must copy bounded owned metadata and perform expensive work after releasing that lock. Pre-load listeners run between selection and worker dispatch; their effects need preservation too.

### Native synchronous read

`PackFile::readFile` at 196640 obtains entry size +28 and byte offset +20 from the handle's entry. It limits requested bytes to the remaining entry length, adds the relative offset, selects a handle from the native thread-indexed array at pack+2A0, opens it if necessary, seeks if required, then dispatches the underlying read. The open call passes mode 0.

`WinFileSystem::openFile` at 192630 has a jump table at 192810. Mode 0 targets 192695, setting access 0x80000000, disposition 3, flags 0x80 and share mode 1 for CreateFileW at 1926FA. The observed native archive path is therefore not opened with an overlapped flag. Other modes must not be silently changed to emulate it.

`WinFileSystem::readFile` at 193390:

```text
if file is null: return 0
repeat:
    call ReadFile(file.HANDLE, destination, low32(requestedBytes), &received, null)
    if successful:
        file.logicalPosition += received
        return received
    if no native error handler, or handling is disabled: return 0
    recoverOrRetry = nativeErrorHandler(file, error, filename)
    if not recoverOrRetry: return 0
```

The ReadFile call at 1933F1 explicitly passes null OVERLAPPED. Preserve native returned-byte counts, retry/error policy and logical position. Do not repurpose a native synchronous handle for independent concurrent positional reads. A replacement should own its archive handle and buffers.

### Completion and buffer retirement

FB440 acquires or allocates the native destination, reads, closes the virtual file, and publishes a LoadedFile to the requested completion-thread list. It can wait on buffer availability or interleave clients before the read. Caller-supplied, pooled, heap-backed and new-buffer paths have distinct flags.

FCFB0 runs on the current native completion thread. It removes one completion record under the completion-list lock, releases that lock, then invokes a listener. New-buffer paths use vtable slots +18/+20. Memory-file paths use +8/+10 and then call releaseBuffers at FD1BA. All paths decrement outstanding completions at loader+818. Callback names cannot be resolved just by the base vtable: several exported empty methods share address BF10. Preserve slot signatures and flags until concrete listener overrides are mapped.

F9FE0 checks whether the completion's data pointer belongs to the loader pool. For recognized pool buffers it clears occupied entries under the pool lock. Otherwise it frees heap memory only when the native flag at completion+158 is set, then decrements heap outstanding count at loader+54C. After releasing buffers it increments the available-buffer signal counter under an SRW lock and calls WakeConditionVariable at FA16D. This connects completion retirement to the producer-side buffer waits measured by R236.

Caution for automated decompilation: the PE exception/unwind entry beginning at F9FE0 ends before the whole source-level function. Its executable continuation reaches FA187. Treating the first unwind record as the complete function truncates the ownership analysis.

### Cancellation

FB1A0 takes the selection and pending locks, searches both queued lists by request ID, removes/destroys a found record, decrements pending count and returns true. If neither list contains it, it returns false. This establishes queued removal only, not cancellation of in-flight reads or ownership of a submitted replacement request.

## First replacement design to investigate

Implement a bounded owned request stage between resolved native work and native consumption. Copy identity/range metadata while valid, hand it off without blocking native selection, submit batches outside native locks, and retain each independent buffer until its I/O completion is confirmed. Native dispatch may consume a ready result only after request/range/archive identity validation. Native decoding, callbacks and upload remain intact.

For late or missing results, preserve immediate native fallback. Account explicitly for wasted in-flight reads; do not hide demand-triggered duplicate I/O behind cache-hit statistics. Admission needs byte and request caps plus priority/deadline handling. Large queue backlogs do not justify reading the entire backlog into memory. Cancelled/unmounted requests must retire their owned I/O without writing to game buffers.

Before implementing: map concrete archive reload/unmount synchronization; find a safe post-lock batch handoff; establish request retirement and duplicate suppression; preserve native error/retry semantics. GPU decompression and texture upload replacement are outside this first scope.

## Deliverable boundary

R236 now supplies live range evidence and stage timing. This document selects the targeted reconstruction and records the first read/completion/cancellation control-flow findings. No additional game build or performance improvement is claimed here. The next implementation should follow the ownership audit, not introduce another broad cache-tuning experiment.
