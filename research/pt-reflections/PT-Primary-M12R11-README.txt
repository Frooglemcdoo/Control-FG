M12R11 - exact scene-descriptor rejection diagnostic

R10 kept capture healthy but submitted no PT work. Every detailed handoff
failure stopped at scene descriptor selection. That combined error did not
identify a missing descriptor, invalid index, lost owner, or changed token.

R11 preserves the exact descriptor rejection and records its slot handle,
allocation address/token/size, current owner token, and descriptor-index health,
first failure reason and counters. It makes no speculative rendering change.
This is a diagnostic build, not a confirmed PT fix. All safety checks remain.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files. Load the entrance save with native RT enabled.
Use F10 -> PT Reflections ON for 10 seconds. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R11.cmd, and upload the results ZIP.
No screenshot is needed.
