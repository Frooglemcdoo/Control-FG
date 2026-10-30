# R226: direct live material descriptor resolution — rejected

## Live-test result (2026-10-30)

The user reported no FPS gain and textures remaining hidden until they streamed in. The Resolver log (`probe-20261030-125344-260-80612.log`) confirms installed=1/ready=1, append bypass, and disabled R223/R224 diagnostics. Native baseline was reported as 69–73 FPS. Resolver material-update samples over presents 3600–5100 averaged (102.240−63.145)/(55−31) = 1.62896 ms; Native presents 7260–10140 averaged 1.43189 ms. Replay batches were 772 versus 770, and material counts differed, so this is not a controlled percentage regression. It supplies no evidence of a benefit.

Reject the candidate on visual correctness alone. The production startup path no longer calls its installer, even if the environment opts in; the old Resolver launcher now selects Native. Retain the implementation and fixture only for forensic investigation. The fixture did not establish equivalence during live streaming; the precise cause remains unproven. Previously distributed R226 binaries are unchanged: exit the game and use `Start-CPU-R226-Native.cmd` to restore the native loop without a new DLL. Preserve R217 visuals, Map Guard OFF and restored angled reflections.

The design and procedure below describe the rejected experiment.

R225's offline audit identified the changing R224 fields as emission intensity and wrinkle-animation weights. These affect material hashes and native sort/deduplication, so freezing values or writing only an old output slot is incorrect. R226 preserves those operations and targets the repeated descriptor helper calls inside each 35-slot material loop.

## Historical comparison procedure

Install over R224. Run `Start-CPU-R226-Native.cmd`, hold the same problem-area view for30–60seconds, and note FPS. Exit completely. Repeat with `Start-CPU-R226-Resolver.cmd`. Exit and run `Collect-ControlFG-Compact-Logs.cmd`; send the ZIP, FPS ranges and any visual difference. No video/profiler capture is needed.

Both launchers keep R218 stage timings/R222 scalar workloads, bypass append experiments and disable R223 writer/R224 field-diff diagnostics. Native leaves the original descriptor loop untouched. Resolver opts into its replacement with CONTROLFG_CPU_MATERIAL_RESOLVER=1. Normal launch does not opt in. Use these launchers to ensure comparable diagnostic settings.

## Change

The native block at renderer0xC80FC–0xC81F3 resolves35 live texture slots. The imported helper functions on this path only read native pointers/CPU descriptor handles, return a null handle, or store a descriptor and increment the global descriptor version. The candidate performs those same operations directly in one C++ loop, removing the repeated imported calls. It retains the resource's signed active-variant index, preferred/fallback container selection, null-container/native-pointer/zero-handle fallback, output order, all35 descriptor writes and all35 version increments, including unsigned wrap.

There is no cross-frame cache, retained resource pointer or suppression of texture callbacks. Every material update, sort/deduplication, output-ID rewrite and native upload still runs. R217 visual fixes, Map Guard OFF and angled reflections remain intact. Performance improvement is unconfirmed until tested in game.

## Installation and validation

Installation requires previously hash-verified renderer/D3D modules, the complete247-byte native loop, exact helper instruction bytes, original imported helper targets, and the original global descriptor-manager import. Any mismatch leaves native behavior. A15-byte startup patch calls an ABI-compliant relay then jumps to the native continuation. The relay preserves the native live nonvolatile outputs: rdi=output base, rbx=35, r13=0, and rsi set to output base only for nonnull slot tables. Remaining nonvolatile registers obey the Windows ABI. Native continuation overwrites volatile values before their next use. The relay supplies shadow space and registered unwind metadata, and remains allocated until process exit.

Windows differential tests execute the original247-byte machine-code block with relocated imports to original leaf-helper bytes, then execute the patched block using the candidate. They compare the entire descriptor buffer and native live register outputs over256 mixed resource cases, including null slot tables, missing resources/containers/native pointers, preferred/fallback variants, zero handles, and version wrap. They also test LastError, unwind and rollback. Runtime graphics behavior still needs the user's game test.

`NATIVE_CPU_R226 installed=1` confirms replacement. Native logs `installed=0 native_loop=1 reason=not_opted_in`. An enabled request with `NATIVE_CPU_R226_INSTALL ready=0` and no installed=1 line means installation failed; do not treat that run as a resolver comparison. No per-slot diagnostic counters are added.
