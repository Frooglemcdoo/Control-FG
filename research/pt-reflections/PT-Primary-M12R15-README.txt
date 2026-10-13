M12R15 - prepare disarmed PT before final scene admission

R14 logged no PT submissions or GPU coverage. Its new diagnostics identify
scene_dispatch_wait_pending: the native ray pass's submitted queue cut still
depends on a signal that has not arrived when lighting is being recorded.

Recording now uses a provisional CPU scene preview built from known queue
edges. Unresolved waits stay unresolved; no completion or signal is invented.
Unknown geometry and competing writes still reject preparation. The private
dispatch is recorded with its existing GPU traversal guard disabled.

Immediately before lighting submission, strict scene selection must resolve
all dependencies and match the exact prepared TLAS, BLAS versions, shader
table, allocations, descriptors and view. Only then can the output be armed.
Pending or changed scenes retain native output and cannot traverse the AS.
Preparation alone never counts as a PT submission or successful rendering.

Tests cover preview/strict separation, late signals, changed BLAS versions,
unknown writes and competing producers. Submission failures now have explicit
diagnostics included in the collector. In-game PT remains unconfirmed.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R15.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
