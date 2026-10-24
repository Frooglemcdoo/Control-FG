# R133: authored light context for a general shadow correction

The R132 capture is valid. Repeated near-light meshes dominate the blocked rays,
but it lacks the authored light parameters needed to distinguish a native
exclusion plane from a guessed distance cutoff. The native shader uses both a
virtual cone position and a separate emitter position. Its original range uses
the virtual position and fNearDistance, while our centered full segment targets
the emitter without that cutoff. Restoring the old scalar cutoff blindly risks
reintroducing the close-shadow losses fixed in R96/R97.

R133 records the missing native context in the same fenced capture: light buffer
float4s at byte offsets 0, 16, 32 and 64, and sys_constants registers 6 through 9
used by the native view-to-world transformation. These contain cone/emitter
positions, near/far distances, direction, radius and the transform. The existing
48-byte ray and 80-byte hit record remain unchanged. Context uses a separate
128-byte-per-sample buffer tail and is paired with its result in the log.
No extra rays or rendering changes are introduced. Private GPU allocation and
readback each grow by 512 KiB; storage is bounded and retained for the session.

The intended analysis compares blocker positions against the authored light
frame across every sampled light. This is a hypothesis test, not proof that
fNearDistance is the rasterizer's exact shadow clipping plane or a proven fix.
No object IDs, room coordinates, arbitrary radii or global offsets are used.

## Test

Install beside Control_DX12.exe with the game closed, preserving NVIDIA runtimes
and existing ControlFGStreamline files. In the affected room, enable RT Contact
Shadows, samples 1, disable FG and glass enhancements. Face the problem light,
select BOTH in F10, close the overlay and hold the view briefly. Let the game run
for 70 seconds, then send Lighting-R133 from
%LOCALAPPDATA%/Control-FG/Diagnostics. One capture per process, automatic logging.

This collects additional fields absent from R132; it is not a request to repeat
the same capture data. Existing rendering sidecars and RR settings are unchanged.
