Control integrated primary PT - M12R30

Close Control and replace all package files in the game directory.
F10 opens the overlay; enable PT Reflections. F11 is unchanged.
Test the same room and repeatedly aim/zoom and release. Run
Collect-PT-Primary-M12R30.cmd afterwards and send its results ZIP.

R29 findings and R30 changes:
1. Unsupported cloth/hair/special BRDFs exceeded the global 5% fault threshold
   during aiming. This demoted the whole frame to RT. R30 excludes only this
   expected per-pixel fallback from readiness. Real transport faults, invalid
   counters and failed commits still prevent admission; no-sample frames cannot
   promote PT. Unsupported paths retain the original native pixel.
2. R28/R29 erased g_tFillSpecular when replacing the native light buffers.
   R30 restores this authenticated t20 reflection input with its exact native
   sampler and UV scale. To avoid double-counting, the first continuation is
   diffuse-only; subsequent hits retain diffuse/GGX transport. Primary direct
   lighting still evaluates both lobes. This is PT diffuse bounce lighting with
   native specular reflections, NOT a complete standalone PT replacement.
   Full reflected-material/transmission support remains unfinished. This build
   does not claim that every remaining lighting discrepancy is resolved.
3. Scene selection scanned the whole queue graph for every TLAS/BLAS lookup,
   in both preparation and late validation. R30 indexes exact write histories
   by allocation token/address. It retains all fence, unordered-writer,
   tombstone and generation checks, rebuilding after history compaction/rebase.
   A local 4,354-object synthetic sweep measured 41.746 ms before / 0.471 ms
   after; these are CPU benchmark results, not promised in-game frame times.

The existing CPU and GPU timers remain in the collector for comparison.
Windows/WARP and CPU tests verify mechanics; visual quality and FPS require
an in-game check on the RTX 5090.
