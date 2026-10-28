# R191: F9 reflection attribution sequence

Install the inner ZIP beside Control_DX12.exe. Keep existing NVIDIA runtimes. Launch normally, enable RR and disable FG. Let gameplay load, face the painting, then press F9 once and keep the camera still. The sequence changes SPEC automatically and takes at least a minute, plus disk export time. Do not open menus or switch windows during capture. Each stage waits five consecutive seconds of evaluated gameplay after reset before recording two consecutive frames.

At completion, rendering settings are restored and a packaging console opens automatically. Wait until it opens the SEND folder. Send every ZIP in that folder, including capture-manifest.zip. Return to the game, move to the elevator, and press F9 once again. Location sessions are numbered 1 and 2, not inferred from screenshots. Python 3 is needed for packaging, as with the prior capture helper.

## Stages

1. Full R184 lighting, SPEC4, normal RR-boundary clamp.
2. Full SPEC4 with the RR-boundary clamp bypassed through the existing raw-copy path.
3. Secondary local point/spot **specular** contribution.
4. Secondary environment-volume specular contribution.
5. Secondary cubemap specular contribution.
6. Secondary emissive contribution.
7. Reflected diffuse contribution (includes local and environment diffuse).
8. Full SPEC16 reference.

Every stage uses gain 1. Primary scene lighting remains present. Isolated secondary signals are not additive after RR; compare their saved pre-RR lighting buffers. All stages retain the original-total per-hit resolve cap. Stage 2 disables only the later RR-boundary clamp. It is a separate settled-stage comparison, not a same-frame pre/post-clamp capture. Two-frame pairs reveal short-term differences, not full temporal convergence statistics.

Each pair includes RR input/output, depth, motion, normal/roughness, diffuse/specular albedo, G-buffers, raw GI, accumulated diffuse/specular lighting and available distance guides. Metadata records the stage, sample count, clamp request, reset/result and binding information. Recording requires a matching same-frame diagnostic dispatch and observed producer sample count. CPU dispatch submission is not proof of the driver executing a specific shader; captured output copies are fence-retired before export.

R190 compact hit distance remains enabled by default and is held consistent throughout. R189 roughness blend remains disabled. This build adds capture automation, not a proposed quality fix. Readback/allocation and SPEC16 stages are intrusive; do not use their FPS as performance measurements.

F9 again cancels. Focus loss, RR/FG changes, unsupported capture, or a timeout also stop the suite. Your prior lighting mode and sample selection are restored; no persistent settings are written. Cancelled/incomplete sessions require restarting the game before another sequence. In-flight resources are retained until their existing fence retirement or process exit; no forced GPU waits or guessed resource releases are added. PACKAGE RR can rerun collection if Python was missing. CAPTURE RR starts the same automated sequence as F9.

A location can produce several GB of raw files and many ZIP parts at 4K. Packaging retains originals. Keep enough free disk space. No automatic deletion or upload occurs.
