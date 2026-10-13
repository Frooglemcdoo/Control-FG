Control PT M12R41 - CPU/GPU attribution build

This is a profiling build, not a claimed performance fix. It retains R40's
rendering, resource lifetimes, synchronization and capture behavior. The held
R41 shared-lock experiment is NOT included. New CPU counters split sampled
wall time into lock waits, native descriptor calls, tracking, scene capture,
command-list reset, and primary preparation stages. They are NOT frame CPU
costs; nested categories overlap. ETW is the critical-path evidence.

INSTALL
Close Control. Extract the entire package beside Control_DX12.exe, replacing
the previous experimental files. Keep your existing Streamline/RR files.
Keep dxgi.pdb beside dxgi.dll: these matching full symbols are for profiling.
Compilation remains optimized (/O2); /DEBUG:FULL is symbol generation only.

FIRST TEST - NORMAL CAPTURE, PT OFF
1. Run PT-Capture-Restore.cmd, then start Control DX12.
2. Load a stationary scene. PT OFF, FG OFF, VSync/frame cap OFF. Keep RR and
   all other graphics settings unchanged throughout. Select DLAA and allow
   at least 15 seconds to settle. Keep the camera still.
3. Right-click PT-Profile.cmd -> Run as administrator. Select 2 (PT-Off)
   and 1 (DLAA). Press Enter when ready, then return to the game within
   10 seconds. It records 20 seconds and saves automatically. Stay in game
   until at least 35 seconds after pressing Enter; trace saving may take longer.
4. Select Quality in the game, wait at least 15 seconds, then repeat with
   2 (PT-Off) and 2 (Quality). Do not move the camera.
5. Upload the two ZIPs in Desktop\Control-PT-Profiles.

CONTROL COMPARISON - STARTUP BYPASS
Close Control. Run PT-Capture-Baseline.cmd and restart. Repeat the same two
captures in the same scene using mode 1 (Bypass). The script verifies the
running process's startup log, rather than relying on the marker on disk.
PT cannot work in this mode. Afterwards close Control, run
PT-Capture-Restore.cmd and restart to restore PT.

PT-ON COMPARISON
With normal capture restored, turn PT ON and repeat both resolutions using
mode 3 (PT-On). The requested PT/DLSS labels are user supplied; runtime logs
and GPU events must confirm them during analysis. Do not change settings
inside a capture. The startup bypass mode, unlike the live PT toggle, is
validated automatically against the running process.

CAPTURE DETAILS
Windows' built-in wpr.exe records GeneralProfile and GPU activity, including
CPU sampling and scheduler data. No PIX installation is required to RECORD.
Elevation is required by Windows ETW. The script checks available profiles,
refuses an existing/unrecognized recorder session, does not cancel any trace,
and attempts to save its own trace on errors. Leave the script running until
it finishes. Do not close its console while recording.
Each ZIP includes CPU-GPU.etl, the process log, exact build/hash metadata,
requested state, trace timestamps and recorder status (including lost events).
These are system performance traces, not gameplay videos.
If WPR status is localized and unrecognized, the script stops without recording;
send that message. No reboot, registry change or permanent tracing is used.

ANALYSIS
Load ETL in Windows Performance Analyzer; point symbols at the matching
package's dxgi.pdb and Microsoft symbols. Inspect sampled CPU stacks, precise
CPU/context-switch waits and GPU queues for Control_DX12.exe, comparing stable
windows. Check lost events first. Preserve FG, VSync, scene and resolution
labels. Do not multiply sampled hook totals by 64 and call that frame time.
The existing RR cadence log is evaluation-entry spacing, not average Present FPS.
Our GPU PT timer measures part of the GPU workload, not all capture overhead.
PT off still performs geometry/descriptor capture; that is intentionally
unchanged so the trace measures the shipped regression.
Primary PT remains additive indirect lighting; this build does not replace
native direct lighting or implement new shadows.

SOURCES
https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options
https://learn.microsoft.com/en-us/windows/win32/direct3d12/copying-descriptors
https://developer.nvidia.com/blog/advanced-api-performance-cpus/
