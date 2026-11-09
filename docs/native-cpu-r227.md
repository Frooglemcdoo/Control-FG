# R227: RT preparation measurement and DirectStorage foundation

**R228 correction:** the R227 process override is applied only at startup and mod-menu changes. Native graphics-settings application can subsequently overwrite the partition count (observed 16 → 2). The effective-value log is an application-time snapshot, not proof of persistent configuration. See native-cpu-r228.md for the opt-in lifecycle correction and phase-hooks-off comparison.

This is the first implementation milestone, not a claimed FPS optimization. R226 Resolver is disabled in startup code. Existing accepted lighting/GI/reflection changes remain intact. The shipped menu defaults are unchanged.

## Game comparison

Install the package over the previous experimental package. Fully exit the game between runs.

1. Run Start-CPU-R227-Baseline.cmd and observe the problem-area view and a short traversal.
2. Repeat with Start-CPU-R227-Phases.cmd using the same camera, route and settings.
3. Run Collect-ControlFG-Compact-Logs.cmd and send the ZIP plus FPS and texture observations.

Both launchers force the agreed 8 ms request budget and existing 16 scan partitions for this process, regardless of the saved menu setting. The override also stays in effect if the menu is changed during that process; the log's STREAMING_R227 effective values are authoritative. Overrides do not rewrite saved settings. Both disable append experiments and verbose material writer/diff probes. Both retain existing whole-stage timing and scalar workload sampling.

Only Phases enables three startup-only, exact-callsite-validated hooks into native readback map/unmap and TLAS prebuild-size estimation. They forward all arguments, return values and native work. Selected jobs (one in 64 on each registered worker) record five intervals: before readback; readback plus its CPU processing; scene selection/assembly; native prebuild sizing; buffer management/upload. With no readback path, the pre-readback interval is zero and the scene interval includes initial query work. Every complete sample partitions one job; incomplete timelines are rejected. The intervals include probe overhead and wall-clock scheduling, not just CPU execution. Use the Baseline run to detect material measurement overhead.

No per-object timing or logging, static cache, GPU offload, resource retention, worker-count change or job repartition is introduced. Buffer management includes conditional allocation, reuse checks, copy and upload; the marker does not assert every job allocates. Readback timing is not proof of a GPU stall.

Expected Phases startup: NATIVE_CPU_R227_INSTALL ready=1 and phases=1. Baseline ready=0 is expected. Compare cumulative NATIVE_CPU_R227_PHASE differences over stable workload windows, not startup averages. Aggregate rows can race an in-flight sample; select matching counts and account for skipped cells.

## Standalone DirectStorage validation

Exit Control, then run DirectStorage/Validate-DirectStorage.cmd. Send DirectStorage/DirectStorage-Validation.log separately; the existing compact collector does not gather it.

The executable creates a temporary synthetic payload, compresses sixteen independent 256 KiB chunks with GDeflate, reads them through DirectStorage 1.3 into a D3D12 buffer, checks completion status/fences, copies the result to CPU-visible memory and compares every byte. It exercises nonzero source offsets, mixed compression ratios and distinct destination ranges. It deletes its temporary file on successful completion. A failure can leave a small cfd*.tmp file in the Windows temp directory.

The validator chooses a high-performance hardware adapter when available, otherwise WARP, and reports the adapter IDs and DirectStorage compression-support flags. CPU fallback is possible and reported; PASS alone does not prove hardware GPU decompression or a game performance gain. It performs bounded waits; timeout exits the process without attempting unsafe resource teardown while work remains in flight.

The DLLs live beside the standalone validator in DirectStorage/, not beside the game's executable. Game textures are not redirected, archives are not modified, and the test does not install a texture cache. This is a correctness foundation for the future backend, not a representative streaming benchmark. SDK version and download SHA256 are pinned in CI; redistributable runtime DLLs and license notices are packaged together.

## Next integration gates

The recovered runtime exposes PackFile readFile/readFileAsync, physical-location lookup, and Loader partial-load/completion dispatch. The audited archive read wrappers translate entry offsets and forward to a lower filesystem interface; this does not prove that all texture payloads are uncompressed. LZ4 exports exist but their use for the targeted texture stream remains unproven. Map actual texture payload layout, native cancellation/completion dispatch, GPU upload publication and retirement before replacing reads. Preserve the responsiveness of 8 ms/16 partitions and use native fallback for unsupported cases.

RT retention is conditional on these measurements and stable identity/invalidation evidence. Do not cache texture pointers or material output IDs merely because an object is stationary.
