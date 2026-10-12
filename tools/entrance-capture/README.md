# E2 update: stop and diagnose failed captures

Extract E2 over the existing capture-tool folder; retain .venv and older Entrance folders.
Use Start-Entrance-Capture.cmd as before. Python/.NET do not need reinstalling.
Keep PIX_DIR set to the installed 2606.18-preview directory.

E2 prints CAPTURE STARTED, reports elapsed time every 10 seconds, and stops the
collector worker after 180 seconds inside the capture request (30 seconds for detach).
The external watchdog remains independent of Python/.NET/PIX calls in that worker.
It does not forcibly terminate Control or claim to cancel PIX cleanly. Close Control
manually if it remains frozen. A long capture can also reach this limit; timeout alone
is not proof of a deadlock. Keep any original PIX capture files.

Empty filenames are preserved as empty/null, never converted to '.'. A failed capture
stops the sequence. Get-method diagnostics exposed by the installed PIX capture-result
interface, exceptions/HRESULTs and progress state are retained. If PIX exposes no
underlying error, the report does not invent one.

On failure send Entrance-<timestamp>-Diagnostics.zip. Raw GPU captures remain separate
and are never discarded to make that archive smaller. This release fixes collector
failure handling; it has NOT established or fixed the underlying Control/PIX stall.

For Steam direct launches, retain the temporary steam_appid.txt containing 870780
beside Control_DX12.exe during both passes. Keep Steam running. Remove only the file
you created after finishing. This prevents SteamAPI_RestartAppIfNecessary's relaunch;
other launch mechanisms may still require investigation.

---

# Control Main Entrance Capture E2

One reusable acquisition tool; eight entrance views across two game launches. No new mod DLL
or shader build is needed. This is a diagnostic capture package, not the new PT renderer.

## Setup once

1. Install [Microsoft PIX Preview](https://devblogs.microsoft.com/pix/download/),
   a build newer than June 15, 2026 that contains `PixApiCsExt.dll`.
2. Install [64-bit Python 3.12](https://www.python.org/downloads/windows/), including
   its Windows `py` launcher, and the [.NET 10 x64 runtime](https://dotnet.microsoft.com/download/dotnet/10.0).
3. Enable Windows Developer Mode (Windows 10: Settings > Update & Security > For developers).
4. Extract this package into a writable folder on a disk with at least 40 GiB free.
   Captures may require substantially more space. Keep free space on the Windows TEMP drive too.

The launcher installs pythonnet 3.0.5 into a private `.venv` in this folder on first
run. PIX, .NET, Python and Developer Mode are checked/required, not silently installed
or enabled. For a custom PIX installation, set `PIX_DIR` to its version directory.

## Capture the entrance

1. Exit Control. Double-click **Start-Entrance-Capture.cmd** and select **Control_DX12.exe**.
2. Load the main entrance. Use borderless mode; keep native RT enabled. Disable frame
   generation. Start with PT on, glossy glass and RR on. Don't change resolution,
   exposure or other graphics options during the comparisons.
3. Follow the console's six pose prompts. Press Enter in the console to arm each
   capture, return to the game, then press **F8** once the view has settled.
   A higher beep means the capture is saved. Alt-Tab back for the next prompt.
   **F9** skips a view and reports it missing. F11 is deliberately not used.
4. After six captures, **exit the game** and press Enter. The tool temporarily renames
   the known R1/R2 `dxgi.dll`, then launches two baseline captures with the Control-FG
   proxy disabled. Follow the two prompts, **exit again**, and press Enter. The tool
   restores the proxy and runs local replay/export and packaging. Restore your
   preferred graphics settings when you next play.
5. Send the generated `Entrance-<timestamp>.zip`. It can be many GB. If upload is
   too large, send `REPORT.txt`, `session.json` and the `*-analysis` folders first,
   and retain every `.wpix` locally. Those raw files are the reusable evidence.

The first six captures cover overview, reflective floor, glass/wall reflection, reverse
view, overview with RR off, and overview with the current PT composite disabled.
Settings and pose labels describe what you were asked to select; they are not
automatically verified. The final two repeat overview and floor with the proxy disabled.
Keep the camera as close as possible for the comparisons.
If RR cannot be changed live, skip that pose; the missing status will be explicit.

## What is captured

PIX records the frame's D3D12 API calls and parameter data, including the resources
needed for its replay. The complete `.wpix` is kept, alongside binary hashes,
GPU/driver identity, current-session mod logs and relevant mod settings.
The exporter attempts local replay on your GPU and indexes **all queues/events**,
then queries every Draw/Dispatch for program/root-signature and resource-view data.
Resource dimensions, formats and object IDs are exported where supported.

The JSON indexes do not contain texture/buffer bytes or all DXR bindings. Those
remain in the raw PIX capture for inspection. Unsupported DXR generic-pipeline
queries and failed exports are recorded, never silently counted as complete.
No game assets are extracted from disk; installation executables/shaders are hashed.
Nothing is uploaded automatically.

## Limits we must keep visible

- Current **PT OFF is composite bypass, not untouched native RT**. Baseline views
  instead disable the known Control-FG proxy for a separate launch. Other game
  modifications are not removed, so this is not a certified pristine installation.
  Unknown proxy hashes cause a reported baseline skip and are left untouched.
- A capture can preserve GPU data without explaining its game-specific meaning.
  Light/material decoding, RR guide identification and DXR structure interpretation
  still need analysis. `requirements.json` lists those remaining validation tasks.
- One-frame captures include incoming temporal resources but do not prove temporal
  stability. Other rooms and content absent from these views are outside this pass.
- PIX capture/replay compatibility with this exact Control/Streamline/RR/driver
  combination must be verified on the game PC. The package has no in-game validation
  yet. Failed replay does not delete a capture. Each export has a 30-minute timeout.
- Replay may depend on GPU and driver. The automated local exports help, but do not
  guarantee the captures can be fully analyzed on a different machine.

## Reuse without recapturing or rebuilding

From this folder, after setup:

```powershell
.\.venv\Scripts\python.exe capture.py --analyze "D:\captures\Entrance-20261012-123456"
.\.venv\Scripts\python.exe capture.py --pack "D:\captures\Entrance-20261012-123456"
```

Analysis can be improved and rerun against the same raw captures. ZIP64 packaging
streams complete files with SHA-256 checksums. It does not use the compact-log
collector, drop large resources or truncate files. If interrupted, use the commands
above; successful captures and PIX's originals remain on disk. Do not run `--pack`
while acquisition/export is still writing the folder. Keep the game closed during replay.

The baseline rename is reversible and restored in a `finally` block. If the tool or
PC is forcibly terminated during that pass, the backup remains beside the game as
`dxgi.dll.entrance-e1-backup`. After exiting the game, restore it with:

```powershell
.\.venv\Scripts\python.exe capture.py --restore "D:\path\to\Control"
```

Restoration verifies the known proxy hash and refuses to overwrite an existing
`dxgi.dll`. Do not install another mod update while this capture routine is running.

If PIX cannot launch the game, stop and return the error plus the saved report;
do not repeatedly replace DLLs. Store launch arguments can be passed with
`Start-Entrance-Capture.ps1 -GameExe <path> -GameArguments <arguments>`.

## Sources and license

Microsoft PIX samples at commit `9dee7bddc088e65332c98c9c28495ab395972fd1`:
`api/_pix_bootstrap.py`, `api/preview/GpuCapture/python/main.py` and
`api/retail/ResourceViews/python/main.py`. Adapted API usage and view-description
helpers are covered by `MICROSOFT-LICENSE.txt`.

- https://github.com/microsoft/pix-samples/tree/9dee7bddc088e65332c98c9c28495ab395972fd1/api
- https://devblogs.microsoft.com/pix/gpu-captures/
- https://devblogs.microsoft.com/pix/directx-raytracing/
