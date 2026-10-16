# R87 device-removal report and recovery

The supplied image image(20261016-183958).png shows:
Device Removed / CreateCommittedResource: due to removed device.
It reports a device that was already removed. The subsequently supplied
PT-Primary-M12R87-Results-20261016-134310-918.zip confirms the R87 startup marker
and logs PT_PRIMARY_PIPELINE_FAILED hr=0x887A0005 at frame3156. Cache capture was
admitted for frames3155/3156 (11 volumes, scale1), but no PT dispatch was submitted
and no completed GPU coverage exists. Pipeline Initialize returned device-removed
before the private PT producer was recorded. Therefore the log does not support
claiming the GI traversal timed out or dereferenced stale resources on the GPU:
that work was never submitted. DXR pipeline creation/driver compilation or an
earlier native GPU failure remains to be distinguished. The pipeline helper
combines multiple creation calls and does not log the first removed-device reason.
No DRED page-fault address or breadcrumb is available. Do not claim out-of-memory,
a specific stale descriptor, driver failure or shader timeout from this evidence.

R87's newly introduced GI integration is withdrawn. R88 restores all three
production files from R86 commit3850e349e2064d11853eaa3876acddc9750d3095:
pt_primary_runtime.in.h (version marker only differs), pt_primary_path.hlsl and
pt_path_integrator.hlsli. Therefore no GI capture, new CBV/SRV remapping, extra
upload region, GPU lookup or metric is active. R86 material composition, RR guide
routing and settings behavior are retained. This is containment, not a claim
that the original missing lighting or the precise GPU fault has been solved.

The new cache tested arithmetic and synthetic resources on WARP. It did not
establish real native-resource retention, state transitions, aliasing and queue
ownership for the newly borrowed CBV, tree and brick resources. Descriptor-value
snapshots and version checks do not establish those properties. These are open
integration requirements, not a proven diagnosis of this particular crash.

Before any cache re-enable: obtain the first device-removal reason/DRED data where
available; trace each actual resource and CBV range through submission and
completion; prove states/lifetimes and live binding validity. Do not turn on an
unproven path simply because the shader compiled or a synthetic GPU test passed.
Microsoft reference: https://learn.microsoft.com/en-us/windows/win32/direct3d12/use-dred

CI retains the isolated shader test for reference but production has no include
of the cache. Windows command gates use explicit nonzero exit checks, including
negative process exits, so assertion aborts cannot appear as successful steps.
Package verification compares primary DXIL bytes with the R86 package, in addition
to checksums, build identity and DLL/PDB matching. No in-game success is asserted.
