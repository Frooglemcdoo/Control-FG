# R177 — shorten probe basis lifetimes in serial SPEC shading

R176 remains the accepted baseline. Its matched-settings capture (Performance-R176-20261027-061420-2328-1.zip) recorded 41.9 FPS against R175's 40.6 FPS, with GI 6, SPEC 16, RR on and FG off. The SPEC-sensitive ray dispatch averaged 5.795 ms versus 6.297 ms; serial shading averaged 5.788 ms versus 6.085 ms. Each GPU mean contains seven samples. These separate runs support the user's observed 1–2 FPS gain but do not isolate every timing difference. The earlier R176 capture used GI Native and is excluded from the matched comparison.

R177 preserves R176's ray fallback reuse and R172's material-load movement. The new experiment moves two immutable probe SRV loads from before a nested lookup loop to immediately before their first use. Six probe basis components no longer need to remain live through 86 static instructions, including the loop. No instructions are removed or added; all arithmetic, sample counts, lookup decisions, branches, texture sampling, clamp and accumulation are unchanged.

Only the original exact-hash serial shader is eligible. The existing creation hooks and native fallback are unchanged. Runtime logs identify four moved loads (two retained material loads and two new probe loads). R174's rejected guards and R170's rejected parallel dispatch remain disabled. Skin/hair guides, shadows, GI, RR and reflection sample quality remain unchanged.

## Validation and limits

The independent audit checks binary load operands against the pinned disassembly, unchanged address and destination components, identical enclosing control scopes, and that crossed breaks only exit the nested loop. There are no crossed resource writes or calls. Reversing only the new move must exactly recover the accepted R172/R176 instruction sequence. Unknown shader input is rejected. CI validates compute PSO creation on WARP and runs existing DXIL, integration and package checks. This is an instruction-permutation proof, not a full-scene GPU image comparison.

Shorter live ranges may help register allocation, but delaying loads also reduces latency hiding. The driver may already schedule them efficiently. No GPU speedup or register-count reduction is claimed before measurement.

## Test

Restart after installing. Explicitly select GI 6 and SPEC 16 (settings can reset), NATIVE BLEND, RR on and FG off. Use the same view and resolution as R176, let the image settle, then run PERF CAPTURE. Send the ZIP and observed FPS/visual changes. Compare against R176, whose accepted result is 41.9 FPS in this scene. Restore R176 if R177 regresses.

Experimental branch only; no production or release update.
