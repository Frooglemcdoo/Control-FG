CONTROL TRAVERSAL RECORDER R1

Keep R411 Clean installed. This recorder does not replace game files.
Extract this folder somewhere with at least 10 GB free space, preferably on
another SSD from the game if available. PresentMon is bundled and verified.

1. Start Control DX12 and load the save where traversal causes dips.
2. Right-click Record-Traversal.cmd and choose Run as administrator.
3. Return to the game. One beep begins a three-minute window. Traverse normally.
4. Two beeps end the window. Let saving/packaging finish, then send the ZIP in Runs.

Keep graphics settings and FG/RR unchanged; don't press F9/F10 or run PIX/Nsight
at the same time. Menu/loading/alt-tab frames remain in the raw record, so tell
us if these occur. Closing Control stops early. Do not close the recorder window.
For a longer run, from elevated PowerShell use:
  .\Record-Traversal.ps1 -Seconds 600
Duration supports 15-900 seconds. Captures are complete file-mode recordings,
not circular histories. Disk pressure ends the run early and is reported.

Contents: per-frame PresentMon CPU/GPU/present/display timings with raw QPC;
light CPU/GPU/disk ETL; one-second process/recorder CPU and memory counters;
profile definitions, event-loss status, matching mod startup log and run bounds.
No game hooks, per-object timers, resource readbacks, screenshots, heap traces,
or per-frame disk logging added by the mod. ETW and PresentMon still have cost;
report if recording itself visibly changes performance. All compression happens
after the timed window. Saving can cause a temporary hitch outside that window.

This is broad performance evidence, not a full function/shader trace. Light
profiles may lack stacks for exact function attribution. CPU frame timing is
not CPU utilization. Cached reads may not show in disk tracing. FG labels depend
on driver/provider support; unknown frames are not asserted to be native FPS.
Event loss and provider support must be checked before calling a run complete.
The tool marks missing frames/ETL as incomplete and retains raw files on failure.
The ZIP can be large: no trace history is deliberately discarded to make it fit.

PresentMon 2.3.1: https://github.com/GameTechDev/PresentMon/releases/tag/v2.3.1
SHA256: 364e5d98d4d134bd54dd25c22ed2ca2f4883f8bc3ed6502bee0c151e3436d30c
License included. WPR uses the Windows installation's CPU.light, GPU.light,
and DiskIO.light definitions and exports those definitions with each run.
