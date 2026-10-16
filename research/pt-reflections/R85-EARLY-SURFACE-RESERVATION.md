# R85 early surface reservation

R84 exposed surface_matching_frame_unavailable (82,200 accumulated draws) and
missing sampler argument 16 (at least 1,800 draws). These observations do not
identify how many misses are visible main-view draws versus auxiliary views.

Drawing previously required Job(recorded) or Dispatched to reserve FrameFor first.
R85 permits Drawing to reserve an empty frame only when no retained output exists
for that engine frame. Provider Read(surface) authenticates the bound view and
rereads complete TLS camera/frame state; Same uses full camera and render extent.
An opaque producer can reuse this reservation regardless of the originating pass.
No matching-frame or submission-order checks are relaxed. Output coverage starts
cleared; producerRecorded plus armed/deferred is still required at submission.
The draw strongly owns the reservation through existing recording/fence retirement.

The existing eight-frame and byte limits remain. An unmatched early view can
consume capacity until its recording retires. Missing TLS authentication stays
native. This is a bounded ordering correction, not proof of the flicker root cause.

Root 16 is the native global sampler table; reviewed surface shaders consume it.
Do not remove this requirement or retain a stale table across descriptor-heap
changes. Microsoft documents table state as undefined after heap changes:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/setting-descriptor-heaps

Validation: native view regression exercises pass-independent full identity and
rejects changed projection, extent and engine frame. Existing Windows GPU guard,
producer admission, output retirement and binding tests remain required. No game
runtime is available here; visual outcome and early reservation frequency await
capture. Missing sampler bindings, per-pixel correspondence and RR guide limits
remain separate unresolved issues.
