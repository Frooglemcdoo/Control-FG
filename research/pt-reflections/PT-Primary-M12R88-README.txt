M12R88 - recovery build; R87 terminal GI withdrawn

Restores the R86 primary runtime, integrator and path shader. Removes R87's GI
resource capture, descriptor remapping, constant uploads and terminal lookup
from production. This is a rollback, not another lighting or performance fix.
The previous missing-light, reflection and flicker limitations remain.

The reported R87 dialog says Device Removed, with CreateCommittedResource
reporting an already-removed device. That message does not identify the first
failing GPU operation or prove a VRAM shortage. The R87 log records device removal during private PT pipeline creation, before
any PT dispatch was submitted. The first GPU fault is still undetermined. The isolated GI test did not validate the live game's resource
states, lifetime or queue ownership; it was insufficient to ship R87 safely.
The GI implementation remains isolated reference code and is not included by the
production shader. It must not be re-enabled solely on the strength of WARP tests.

Close Control, replace ALL package files, and preserve your settings.
Do not rerun PT-Enable. Use Collect-PT-Primary-M12R88.cmd after gameplay.
