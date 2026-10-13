Control primary PT - M12R37 isolation test

R36 did not fix the reported FPS regression. Its cleanup fence reduction
worked (78,544 skipped signals), and total queue-mutex wait was only about
13 ms in the run. Neither explains the sustained loss by itself. PT GPU
samples fell from about 4 ms to 1.84 ms, confirming some GPU resolution scaling.

This is a targeted diagnostic build, NOT another claimed FPS/lighting fix.
It provides a restart-only PT infrastructure bypass while keeping RR and FG
available. No hook teardown or stale-scene reactivation occurs inside a run.

1. Close Control and install all files beside the game's dxgi.dll.
2. Run PT-Capture-Baseline.cmd from that directory, then start Control normally.
3. Keep RR/FG and scene fixed. Compare DLAA and Quality FPS. PT is unavailable
   in this baseline mode; its overlay toggle does not enable a renderer.
4. Run Collect-PT-Primary-M12R37.cmd and send its results ZIP with both FPS values.
5. Run PT-Capture-Restore.cmd and fully restart to restore normal PT testing.

The marker ControlFG-PT-Capture-Off.txt is read once beside dxgi.dll. Leaving
it there keeps PT capture disabled across launches, including later builds
that support this switch. Restore removes only this marker.

Disabled paths: experimental PT shader lookup/replacement installation,
early-device PT pipeline interception, legacy command-list capture, the PT
scene/geometry ownership and queue/descriptor trackers installed through that
route, and the five diagnostic DXR observer hooks. RR guide/evaluation and
Streamline FG initialization remain enabled. This is not a vanilla-game test.

The collector explicitly identifies the bypass; absence of PT session/GPU
records is expected in that mode. New Reflex sleep CPU timings are collected
in both startup modes without changing frame-pacing behavior. If the bypass
restores 80-85/125 FPS, the loss is in PT infrastructure; if it does not, the
next investigation must include shared RR/FG/frame-pacing work.

Normal PT remains additive indirect with native primary lighting/shadows.
Lighting replacement and new soft shadows are unfinished.
