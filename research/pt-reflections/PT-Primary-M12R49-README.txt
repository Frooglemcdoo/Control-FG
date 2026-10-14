M12R49 - avoid global descriptor locking for untracked CPU CBV writes

R48 matched CPU/GPU traces show a slower frame cadence with capture enabled
and lower queue-derived GPU occupancy. This build removes exclusive descriptor
locking and observer work only for constant-buffer descriptor addresses with
no tracked metadata and outside all retained mirror ranges. Native CBV creation
always runs. Tracked AS/source/UAV addresses and mirror destinations retain
observation. The monotonic interest filter is conservative across address reuse.

Close Control, extract this package over R48, run PT-Enable.cmd, then restart.
Keep the same scene/camera and RR setting. Turn FG OFF; leave tracing OFF.
Measure DLAA and Quality first with PT OFF, then with PT ON. Check moving Jesse
and reflections for corruption. Send the four FPS readings and the ZIP produced
by Collect-PT-Primary-M12R49.cmd. No large ETL is needed for this test.

RT-Only.cmd plus restart returns to the RT-only baseline. PT-off in a PT-enabled
session still captures geometry. This is a targeted performance test, not a
claim that the entire performance issue is fixed. GPU copies and shaders are
unchanged. The existing R48 GPU capture script intentionally requires R48.
