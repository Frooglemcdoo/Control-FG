M12R12 - remove incorrect descriptor-copy range cap

R11 showed the descriptor index permanently invalid before its first AS write.
The CopyDescriptors hook treated calls with more than 8192 ranges as invalid,
then called Invalidate with an empty span. This reproduces the exact reported
failure and stops all later AS descriptor tracking. R11 did not record the
failing call's range count, so this remains the identified candidate cause.
Large native descriptor copies must not be constrained by a per-pass limit.

R12 preserves every native copy range, including one-descriptor sources and
zero-sized ranges. Null range arrays remain rejected and are logged separately
from heap-span failures. Regression coverage includes 244000 source ranges,
a known AS at the end, and erasure of an older AS by an unknown source.
This fixes the incorrect cap; in-game PT dispatch is not yet confirmed.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files. Load the entrance save with native RT enabled.
Use F10 -> PT Reflections ON for 10 seconds. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R12.cmd, and upload the results ZIP.
No screenshot is needed.
