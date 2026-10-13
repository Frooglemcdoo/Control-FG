M12R14 - preserve ready scene cuts across later queued waits

R13 restored native RT performance in the reported test, but logged no PT
submissions or GPU coverage. Its ambiguous/mismatch message was a pending
scene-ordering result, not evidence that PT rendered.

The queue graph now uses proven same-queue FIFO and baked completion edges
before asking a future operation to resolve all its waits. An unrelated later
signal, or an ordered later AS overwrite, cannot block an already-ready scene
solely because its own future wait has not been signalled yet. Pending waits
at the selected cut, competing AS writes and unordered cross-queue fence
signals still block admission. No GPU wait or invented completion is added.

Regression tests reproduce the false pending result and verify that genuine
pending dependencies and competing writes remain rejected. Pending diagnostics
now distinguish submission, dispatch waits, competing writes and fence order.
This fixes a reproduced ordering bug; in-game PT rendering remains unconfirmed.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R14.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
