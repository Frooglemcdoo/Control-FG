# R110: Specular test startup repair

R108's log records GI counts 2, 8, 12 and 16 reaching the native producer with full RR selected, while the native specular and shadow options stayed at one. The user saw no improvement. This does not prove GI can never contribute to noise, but it makes GI count a poor explanation for this remaining problem. Raising the native RT slider affects several paths; R109 isolates specular ray count.

## Test

Install with Control closed. Keep RR on, FG and enhanced glass off. Set the game's **Ray Tracing Samples Count to 1**. Keep the mod's Global Illumination Samples at the same saved value throughout (the reported session used six). Keep resolution, camera position and exposure unchanged.

Open F10 and compare **SPEC 1, SPEC 4, SPEC 8**, then **SPEC 12 / SPEC 16** if useful. SPEC 2 is also available. Wait about ten seconds per mode; observe both stationary quality and FPS. All SPEC buttons preserve centered filtered spotlight shadows, GI filtering, textured world albedo, the 60% clamp and the existing skin/hair guides. They do not enable the rejected specular-filter experiment. No restart between modes.

Save one Lighting-R110 log. Report which count, if any, improves the painting/doorway and its FPS. BASELINE restores native specular sampling; no setting is persisted by these buttons. GI controls in the graphics menu are unchanged. RR-off, partial RR, initialization and recovery frames use native specular counts.

## Implementation

The supported renderer is hash-verified before initialization. The exact seven-byte load at RVA 0x128FF4 reads `rt:Sample Count Specular` into R9D. The untouched instruction at 0x128FFB saves that count into `[rbp+0x100]`. R9D feeds the hit-array allocation at 0x12902F; the same local later feeds `g_uRTReflectionRayCount` at 0x129300. The reproducible native audit includes both routes.

R109 made this diagnostic a required step of RR initialization. Its relay used RIP-relative data loads, but a relay allocated near the lower edge of the site's ±2 GiB range could be more than 2 GiB from the native option. The encoder then rejected preparation and RR never installed. The supplied log confirms zero installed frame hooks and zero RR attempts; it does not contain the failed address. The address-layout failure is reproduced by the new test.

R110 uses absolute data addresses through saved/restored RAX. It also makes diagnostic preparation/installation optional: failure leaves baseline RR enabled, blocks SPEC selection and logs the precise setup stage.

An initialization-only relay intercepts that one load, reads the native option, and substitutes a frame-latched request only for valid 1..16 native/request counts. Zero request means native passthrough. It changes no registered option, allocation call, shader constant independently, or global game RT setting. The relay makes no calls and preserves RAX, flags and every register except the intended R9D destination. Code is RX; request/observation storage is a separate RW page. Changing a button does not patch executable memory.

Frame selection resets RR history on a specular request change. The override is cleared before native reset and enabled only after a successful full-RR selection. The producer records native/effective counts in private memory; the existing reflection callback reports these through the bounded diagnostic queue, on changes and periodic snapshots. No GPU readback, wait, shader recompilation or render-thread file writing is added. Native higher-count allocation work and additional ray cost still apply.

`specular_sample_producer` records the actual CPU producer value used by allocation and shader-provider binding. `gi_sample_producer` records the independent GI count; `rt_sample_options` records unchanged native option values. These logs are CPU evidence, not GPU-completion proof. Quality and performance require the in-game comparison.

## Validation

The emitted relay executes in a synthetic harness on Linux and Windows. It executes 126 native/request combinations across near and far virtual-memory layouts, including native/state addresses beyond the relay's signed RIP range, RAX/flag preservation, unchanged native storage, exact opcode/target checks, out-of-range address rejection and invalid-count passthrough. Live mode tests cover preserved GI/guide/shadow policies and six-bit selection storage. Windows CI also runs existing WARP, menu and baseline checks. Packaging verifies unchanged R97 shadow/glass assets and the production guide/clamp shaders.
