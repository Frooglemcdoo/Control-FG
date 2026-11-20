# Control PIX local capture kit

Use with the existing R490 mod. This kit does not replace the game DLL.
Everything stays on your PC; no automatic upload, ZIP, deletion, registry edit,
or keyboard injection is performed.

## Capture
1. Close Control. Extract this kit anywhere on your PC.
2. Run `Capture-PIX.cmd`. Select a folder on the drive where you want the data,
   then enter the full path to `Control_DX12.exe`. PIX must already be installed.
3. The helper launches Control through PIX. Load the problematic scene with RR ON,
   FG OFF, and your normal GI settings. Do not use the custom F10 capture or the
   old motion-capture launcher alongside PIX.
4. Return to the console and press Enter. You have seven seconds to return to
   the game and move Jesse through the affected light. The helper captures the
   next **16 presented frames**, not sixteen seconds. Keep moving; do not use the
   custom recorder's STOP instructions. Capture can stall or slow the game.
5. Wait for the console to print the saved `.wpix` path. Close the game.

The helper inspects the installed pixtool help before launch and refuses unsupported
capture options. It chooses the newest installed standard PIX executable by file
time. To choose a particular version, call Control-PIX.ps1 with -PixTool.
A capture can still be large; PIX size and replay compatibility are not guaranteed.
If injection or access is denied, keep the logs; run the helper with the same
privilege level as the game. Do not repeatedly recapture an identical failure.

## Local inspection
Run `Analyze-PIX.cmd`, enter an output folder and the saved `.wpix` path.
It independently attempts:
- The capture-time screenshot (not a screenshot sequence).
- The GPU event list CSV.
- One replay pass.
- A replayed render target 0 image (not guaranteed to be the final output).

Each step has a log and an entry in `manifest.json`. Failures preserve all originals
and successful partial exports. A nonzero exit or absent image is not treated as a
clean capture. No automatic image-equality or speckle-detection claim is made.
Full buffer/shader inspection still uses PIX UI or the installed PIX API; these
basic exports do not automatically contain every RR input or per-ray intermediate.

Open the original `.wpix` in PIX on the capture PC. Check that the affected frame
and speckle appear during replay before attributing it to a rendering pass.
The GPU and driver should match capture time. Do not update the driver between
capture and analysis. If replay fails or changes the artifact, investigate that
limitation before accepting buffer comparisons as evidence.

## Local Codex handoff
Open the Control-FG source in local Codex. Give it the `.wpix` path, the output
folder, and `CODEX-HANDOFF.md`. The large capture stays local. Do not send it here.
The scripts export a starting point, not a completed root-cause analysis.

Advanced invocation (PowerShell):
```powershell
.\Control-PIX.ps1 -Mode Capture -GameExe 'D:\Games\Control\Control_DX12.exe' -OutputRoot 'E:\Control-Captures' -Frames 16 -PixTool 'C:\Program Files\Microsoft PIX\2603.25\pixtool.exe'
.\Control-PIX.ps1 -Mode Analyze -CapturePath 'E:\Control-Captures\run\motion.wpix' -OutputRoot 'E:\Control-Captures'
```

Implementation references:
- https://learn.microsoft.com/en-us/windows/pix/gpu-captures/pixtool
- https://learn.microsoft.com/en-us/windows/pix/gpu-captures/directx-raytracing
- https://learn.microsoft.com/en-us/windows/pix/gpu-captures/gpu-captures-overview

Validation: Windows PowerShell parsing and mocked command/error handling are tested.
Real Control + RTX 5090 capture and DLSS RR replay cannot be validated by this kit's
CI. The first run must establish that compatibility, not assume it.
