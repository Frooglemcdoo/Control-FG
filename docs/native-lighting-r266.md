# R266: second verified foliage variant

Install the complete package beside Control_DX12.exe. Keep RR ON, Native Blend,
GI RAW BYPASS, GI 6, FG OFF. Inspect both the original failing plant and the
Central Research broad-leaf planters, stationary and moving. Recheck the floor.
Send visual results and compact logs. Admission marker: RR_FOLIAGE_R266.

Adds the exact 12956-byte native foliage lighting shader captured in session
24923, sharing the audited 7692-byte vertex and 1072-byte native albedo replay
with the previously supported 17648-byte lighting shader. Original pixel size
selects which evidence to compare; full byte equality remains mandatory.
Vertex equality, pixel bindings/outputs, native depth, draw-range checks and
unknown-shader fallback remain intact. No material IDs are hardcoded.

This targets the measured Central Research leaf-guide gap. It is NOT a confirmed
fix for the original plant: that plant already uses the supported variant and
still shows speckles. Batch admission does not prove pixel guide correctness.
No additional filtering, hair changes or speculative GI modifications included.
R264 sampling, ray count, DirectStorage and reflection settings remain unchanged.

Local audit verifies both native base-color dataflows and embedded bytecode.
Windows validation covers both admitted variants, cache reuse and rejection of
modified shaders. Runtime image quality and performance need the in-game check.
