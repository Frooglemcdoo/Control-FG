Control Entrance Offline Export E3

Extract this update into C:\Dev\Control-Entrance-Capture.
It adds export_saved.py; it does not replace your capture launcher or captures.
Close Control, then run in PowerShell:

$env:PIX_DIR = "C:\Program Files\Microsoft PIX Preview\2606.18-preview"
cd C:\Dev\Control-Entrance-Capture
.\.venv\Scripts\python.exe export_saved.py "C:\Dev\Control-Entrance-Capture\Entrance-20261012-100912-390040" --native-baseline

Upload the printed *-Offline-E3-Review.zip when finished.

WHAT THIS DOES
Uses the installed pixtool.exe open-capture / export-to-cpp commands to export
all six existing frames, including captured D3D12 setup/commands and the resource
payload. It bypasses the old Python exporter's invalid DXR-to-generic cast.
No game launch, DLL rename, injection, new capture, build, or execution of the
exported C++ occurs. Only PIX reads/replays the saved capture as needed.
The old *-analysis folders and original .wpix files are untouched.

Heartbeat every ten seconds reports time and output size, including while PIX
is busy and its own log is quiet. There is a 30-minute limit per offline export;
--timeout 3600 increases it if needed. Timeout stops only this offline pixtool
process. It does not contact a running game; the tool requires Control closed.

DISK SPACE
Each frame produces an independent full export under SESSION\offline-E3.
Allow tens of GB free for six frames. A minimum check before each export requires
the larger of 8 GiB or four times that capture's size; this is a heuristic, not a
guaranteed upper bound. Partial outputs are retained if disk space is exhausted.
Repeated runs skip finished exports when the capture fingerprint and output sizes
match. Failed attempts go into new directories so prior output remains intact.
Do not run two copies. If the computer shuts down mid-run, remove only the stale
SESSION\offline-E3\.running file after confirming no exporter is running.

RESULTS AND LIMITS
A zero PIX exit code alone is insufficient. E3 also checks for generated C++, a
nonempty resources.bin, and code references to DispatchRays, CreateStateObject,
DXIL libraries, and acceleration-structure builds. These are structural checks;
references may include helper declarations. Status 'exported_unvalidated' does
NOT mean the project was compiled/replayed or all full-PT requirements decoded.
Unexpected layouts are retained as incomplete for inspection, not discarded.

The upload review ZIP contains generated source, small supporting files, logs,
and inventories (up to 256 MiB uncompressed). Large resources.bin and any files
excluded by that limit stay local and are listed in review-contents.json. Keep
both offline-E3 and the original .wpix files for subsequent targeted extraction.
The review ZIP alone is not a full replay project.

All six frames in this session were user-confirmed proxy-disabled native frames.
The filenames rr-off/composite-off do not prove PT or RR comparisons.
Capture pauses do not invalidate a frame merely by happening. Successful replay
supports using these captures for frame inspection, but independent still frames
do not establish continuous motion, temporal stability, or normal frame pacing.

VALIDATION
CLI syntax checked against the installed help strings from Microsoft's official
PIX 2606.18-preview installer. Automated tests cover failure reporting, timeout,
resume checks and payload preservation. Actual Control export must run on the
user's Windows/PIX machine; no Windows GPU validation was performed here.

References:
https://devblogs.microsoft.com/pix/pix-2203-30/
https://devblogs.microsoft.com/pix/download/
