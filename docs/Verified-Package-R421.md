# Verified package R421

R420 did not fix either the GI-lit jaw/neck noise or red hair speckles. R421 is diagnostic only; no visual fix is claimed.

- Runtime commit: 84d6df15c5f5b90854f31174baf418e9576807ef
- Workflow: https://github.com/Frooglemcdoo/Control-FG/actions/runs/42940738311
- Job: 130261660659; all 60 steps succeeded.
- Artifact: 13821111328
- Outer artifact SHA256: 5cda2e3c4965c55011ebbd16cf9d12e5b0111df0714f21423b491becc44eeba3
- Package: ControlTOH-R421.zip; 6,798,329 bytes; 83 entries.
- Package SHA256: 2a685a2c413ac504d288ef821dca55025e0187e0497642f04a75d4b4620eece4
- DLL SHA256: 1606985623e80a2e3aac6e432905c81590bbd116406d4155faa89199233af56b
- Download: https://chatgpt.com/library?file_id=libfile_d522ca89fe188191812ced40a454110a&account_id=personal

Verified outer and inner checksums, ZIP integrity, x64 PE, R421 UI/startup markers, CAPTURE MOTION label, preserved character guide/history markers and packaged instructions. Windows motion-mode wiring test passed. No in-game capture has yet been tested.

Install with game closed. Use Native Blend, RR ON, FG OFF, RELAX OFF, no lighting override. Press CAPTURE MOTION and keep reproducing the artifact by moving camera/Jesse under GI illumination. One beep arms; two beeps indicate ZIP packaging completed. Send that ZIP.

This captures one consecutive pair at full resolution with lighting unchanged, excluding the large per-ray GI hit dump. The prior R378 exports were stride-four samples and switched Raw/Stable conditions. Full-size readback may hitch; this is not a performance benchmark. Native packed character reconstruction maps and effective fragment coverage remain uncaptured evidence gaps.
