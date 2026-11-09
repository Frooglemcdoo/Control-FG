# R144 — selected-light records during movement

R143's video shows changes in raw spotlight signal and ID colors while walking
through the room. This is not proof of incorrect selection or exposure causing
shadow loss. ID colors may change when the light buffer is renumbered; moving
Jesse also changes valid shadow geometry. Keep exposure enabled and preserve the
accepted R142 default while gathering actual light records.

## Test

Close Control to install. Retain NVIDIA runtimes. Keep RT Contact Shadows on,
game RT samples at 1, FG/glass enhancements off. Load the affected room and allow
Default (Fixed) to become ready. Begin recording a video if practical.

Press **Shift+F9 once**, then repeatedly walk through the troublesome transition
and back. The trace stays in Default (Fixed); do not change modes. It collects six
64x64 selected-spotlight samples, at least four seconds between requests, with no
screenshots or mode cycling. Usually this takes around 25–40 seconds, but capture
completion depends on the game. A 60-second deadline prevents endless capture.

A completion dialog shows a `Shadow-R144-...` folder. Zip and send that folder
with the video. It contains `manifest.txt` and a single `lighting.log`; you do not
need to run Python or manually save another log. F9 or Shift+F9 cancels; focus
loss cancels. Your prior mode is restored. Plain F9 retains the stationary suite
of 28 comparisons / up to 84 PNGs.

A failed/pending capture is reported, not silently retried indefinitely. In-flight
GPU work retains its resources; cancellation only cancels requests not yet
recorded. No render-thread fence waits or readback polling loops are introduced.
This is opt-in diagnostic work, not background gameplay telemetry. Extra capture
work can affect frame times; this run is not a performance benchmark.

## Captured evidence and limits

The established capture supplies the selected spotlight index, its cone/emitter
positions and direction in view coordinates, near/far/radius/shape fields, the
camera's view-to-world matrix, sampled receiver origin and reference ray hits.
Default now permits the same capture path previously restricted to Both. The
producer remains the corrected default, so selection reflects that mode.

The diagnostic input/hit rays are still the existing **unclipped reference rays**.
They do not measure the final authored-clipped visibility or exposure. No game
shader binary changes. The useful new evidence is actual selected light geometry
and its world-space relationship to sampled receivers across movement.

The six samples reuse the fixed allocation only after export, fence completion
and native command-list reset are confirmed. Requests are spaced and bounded.
The existing 64-record export batches and asynchronous log queue remain in use.
There is no additional continuous capture and no changing selections between
samples. Normal F9 behavior and all accepted render policies are retained.

## Offline analysis

Maintainer command: `python tools/analyze-light-transition.py lighting.log`.
No dependency beyond Python's standard library. The tool requires a complete,
zero-failure trace with six ordered requests in one mode sequence, monotonically
increasing captured presents, and validated GPU completion/export provenance.
It reconstructs world-space light geometry with the native matrix convention.

Matching uses emitter and cone positions (default tolerance 0.02 engine units),
direction and shape. A unique match in both directions is a geometric candidate;
it is **not a persistent engine object identity**. Co-located matching lights are
reported ambiguous. Lights not matched in the previous sample may have moved or
may simply not have been selected by that 64x64 grid. Do not call these culled.
Camera/grid motion changes sampled receiving surfaces, so coverage counts alone
also do not prove missing shadows. The report retains receiver bounds and actual
light geometry for further analysis against the video.

Validation includes camera translation/rotation invariance, renumbering and
ambiguous matches, finite/rigid transform checks, complete six-sample logs,
replayed/stale/partial exports, readiness and bounded timing policy. In-game
capture behavior and the reported shadow cause still need user evidence.


### Upload-sized capture packaging

Extract the separate Capture-Packager helper ZIP and drag the completed capture
folder onto `Package-Lighting-Capture.cmd`. Or double-click it and paste the folder
path. This uses installed Python 3.8+ with no additional packages or .NET runtime.
Place any video to include in the completed capture folder before packaging.
Send **all ZIP files**, including `capture-manifest.zip`, from the new sibling
`Shadow-...-SEND-...` folder. Every ZIP is verified below 200,000,000 bytes.
The originals are retained. Packaging is manual after capture completion and runs
outside the game. Do not package a folder while capture is still writing to it.

The helper splits even a single oversized file, streams bounded buffers, includes
per-chunk SHA-256 metadata, checks ZIP CRCs and actual final sizes, and inventories
all ZIP parts with hashes. Failed outputs are marked INCOMPLETE-DO-NOT-SEND.
To reconstruct, keep every ZIP together and run:
`python package-lighting-capture.py SEND_FOLDER --restore-to NEW_FOLDER`.
Tests cover incompressible data, multi-part files, Unicode paths, empty files,
exact reconstruction and missing-part rejection. A 201 MB random-file test
produced parts of 180,055,310 and 21,006,810 bytes plus the manifest ZIP.
