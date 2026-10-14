M12R48 - reuse geometry retirement fences per exact queue

RT-only remains the verified default from R47. This change targets PT-enabled
capture overhead: one persistent fence per submitted queue replaces a new
fence object for every nonempty geometry-transfer batch. Signals remain after
native ExecuteCommandLists. Each batch has its own increasing completion value.
Polling reads each queue fence once. Recorded-list ownership survives Reset
boundaries exactly as before, including replay. Failed proofs retain resources.

Test: close Control, extract over R47, run PT-Enable.cmd, restart.
Use the same scene/camera, RR setting and FG OFF. First measure DLAA and Quality
with the overlay PT toggle OFF (capture active), then with PT ON. Send those
four FPS readings plus Collect-PT-Primary-M12R48.cmd results.
Check moving Jesse/objects for new corruption. No shader or lighting changes.
PT_PRIMARY_RETIREMENT logs fences/signals/pending/failures; fence creation should
stay near queue count while signals grow. No FPS improvement is claimed yet.

Run RT-Only.cmd and restart to return to the validated RT-only baseline.
Live PT-off still retains capture. Full live shutdown/reactivation is not
implemented by this change. Shader, mirror, geometry-copy volume unchanged.
