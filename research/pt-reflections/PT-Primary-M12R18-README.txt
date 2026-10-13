M12R18 - preserve the last geometry update while PT is off

R17 identifies the actual blocking cause: scene_update_while_pt_off. No PT
dispatch was submitted. Required BLAS metadata had been deliberately discarded
when its update arrived with PT disabled. If the object then stopped updating,
enabling PT could never recover its geometry.

The observer now freezes those updates too, using R16's bounded vertex-range
copies. It preserves the current deformation rather than reusing stale data or
waiting for an update the game may never issue. PT rendering remains disabled
until requested, and strict scene/ownership/submission checks remain required.
Active copies retain their 512 MiB budget and fence-based lifetime rules.

A WARP regression exercises an initial build, a final update while PT is off,
and enabling PT with no later geometry build. It requires the updated frozen
geometry to be selectable and executes the capture's GPU copies. Existing
snapshot tests separately verify copied bytes, overwrite/release and retirement.
Logs count off_updates_preserved. In-game PT and off-mode FPS need verification;
preserving updates adds bounded copy work while PT is off.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R18.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
