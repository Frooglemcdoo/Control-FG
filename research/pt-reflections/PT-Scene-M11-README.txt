Control PT M11 - INSTANCE / SHADER-RECORD ROUTING

Still diagnostic: this build does not enable full PT or change the image.
M10 exercised resource ownership successfully, but hit its retention cap.

1. Close Control and back up dxgi.dll.
2. Replace only dxgi.dll beside Control_DX12.exe; keep the shader files.
3. Load the entrance with RT ON and PT OFF. Move briefly, then leave the game
   running THREE MINUTES before closing it.
4. Run Collect-PT-Scene-M11.cmd after closing. Send its results ZIP.

M11 connects native CPU instance uploads, BLAS builds and compaction, TLAS builds,
and sampled shader bindings. It checks record routing and reports missing or
conflicting relationships. It also corrects padded R16 index bounds and raises
retention to 1 GiB (64 MiB per resource), released after the 120-second capture.
CPU tracking and retention may affect loading/frame times during capture.

No GPU submissions, resource-state changes or shader-table substitutions occur.
CPU matches do not prove same-dispatch identity or GPU submission lifetime.
Requires the verified Steam native DLLs and your existing mod installation.
Rollback: close Control and restore the backed-up dxgi.dll.
