# R358: preserve RR throughout character capture

R357 run130922-6280-0 captured only stable and RR-off. Stable recorded successful RR evaluations; RR-off recorded selected0,zero RR successes and status3(Stopped). At restoration, RR mode2 was requested but RR did not resume, so the suite ended present_timeout. Thirteen character PSOs and all material groups were active; this was not missing shader coverage. The precise terminal renderer fault is absent from the package and is not established.

R358 removes the automatic RR off/on comparison. The uploaded off comparison already shows remaining neck grain; the pending work is the four composition views. New sequence: stable,base/material lighting,rig diffuse,rig specular,material view,stable repeat. Two full-client JPEGs each. RR request is never written by capture, including finish/cancellation. External RR-mode changes cancel capture without restoring over user intent.

A stopped runtime when Full RR is requested now exits with rr_runtime_stopped_restart_required. It does not clear the renderer's terminal guard. Failure packages include at most65536 bytes of log data since capture start (tail selected if longer). Shader candidates are exported only for shader-not-active failures. Generic timeouts distinguish missing RR selection from missing presents.

No claim to fix the underlying RR toggle failure or neck noise. Gameplay shaders, composition diagnostics, near history and clamp60 are unchanged. A game restart is required to leave the previously stopped runtime.

Validation: static prohibition on RR mutation in the capture; six-phase coverage; actual CopyLog extracted into a Win32 fixture tests cap/session boundary/empty/legacy copy and failure cases. Existing4800 shader arithmetic cases and Windows WARP validity remain required.
