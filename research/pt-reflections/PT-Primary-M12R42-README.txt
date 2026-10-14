Control PT M12R42 - descriptor tracking fast path

R41 captures show native single-descriptor copies repeatedly entering the
mod's global descriptor lock even with PT off. R42 avoids that transaction
only for one-slot copies proven irrelevant to PT's descriptor metadata.

INSTALL AND FIRST TEST
Close Control. Extract ALL files beside Control_DX12.exe, replacing the
previous experimental files. Keep existing Streamline/RR files. Run
PT-Capture-Restore.cmd before starting, to remove the startup bypass marker.
Keep dxgi.pdb beside dxgi.dll if a later trace is needed.

First compare WITHOUT WPR recording: same stationary scene, FG OFF, frame
cap/VSync OFF, unchanged RR and graphics settings. Let each mode settle.
1. PT OFF: DLAA, then Quality.
2. PT ON: DLAA, then Quality. Check that lighting/reflections remain intact.
3. Run Collect-PT-Primary-M12R42.cmd and send its small results ZIP with FPS.
No new large trace is needed for this first check.

WHAT CHANGED
A conservative atomic interest filter recognizes descriptors that may carry
AS identities, typed-UAV metadata, or cached source tokens. All writes into
mirrored pass tables remain tracked. Known source/destination descriptors,
bulk copies, and all descriptor creation hooks retain the existing observer.
The filter never clears its bits. Collisions, reused addresses, or saturation
can cause extra tracking, never permission to skip known metadata. Native
copies and legacy glass capture still run. Lock ordering on tracked copies,
PT scene ownership, and GPU shaders are unchanged.

The new PT_PRIMARY_HOT_CPU kind=descriptor_untracked counter reports sampled
calls taking the fast path. Existing descriptor counters cover tracked calls.
These are inclusive sampled wall-time totals, not frame time; do not add them
or multiply by 64 to infer the frame's critical path.

LIMITS
This targets measured CPU synchronization overhead; restored FPS is NOT yet
verified in Control. PT-off scene/geometry capture still runs for live PT
activation. PT lighting remains additive indirect lighting, and this build
adds no shadows or new lighting model. ETW/WPR recording can perturb timing,
so judge FPS outside recording first. PT-Profile.cmd remains available only
if follow-up profiling is needed.
