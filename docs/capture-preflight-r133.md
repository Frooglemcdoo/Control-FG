# R133 standalone preflight

This optional standalone tool validates startup of the R133 pipelines, including
the extended light-context output. It does not collect game-scene data.
Close Control, extract outside the game directory and run Run-Capture-Preflight.cmd.
The log is Capture-R133 in %LOCALAPPDATA%/Control-FG/Diagnostics.
Each pipeline stage is isolated in a child process. No game injection or ray
traversal is performed. WARP CI is not hardware verification.

R131 passed on the RTX 5090 and R132 completed an in-game hit capture. R133 retains
the ray/payload ABI and appends 128 bytes of native light metadata per sample to
a private buffer tail. The main R133 game package is required for that metadata.
