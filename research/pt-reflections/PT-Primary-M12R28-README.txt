Control-FG Primary PT M12R28

Close Control and replace every supplied file. Use F10 > PT Reflections.
After testing run Collect-PT-Primary-M12R28.cmd and send the results ZIP.

R27 produced valid private PT samples, but the user saw no visible change.
Its private GPU dispatch measured 3.10-3.30 ms; the larger total frame-time
increase cannot be attributed to that dispatch alone. Prior coverage counters
proved a private texture write, not that its pixels reached the final image.

R28 replaces the separate surface-PS composition route. After the native standard
primary lighting dispatch, validated full-mode PT pixels write the native u0/u1
light-buffer pair directly. Native surfaces consume diffuse * albedo + specular;
PT already includes material response, so it writes zero diffuse irradiance and
its complete reflected radiance into the additive slot. RGBR encoding matches
the original stores. Native surface emission, fog and later processing remain.
Rejected, unsupported, sparse-probe and invalid pixels leave both native outputs
unchanged. Full-mode coverage now counts commits to the native lighting buffers.
UAV barriers order the original lighting, private overwrite and later consumers.

Per-draw replacement callbacks and their descriptor reservations are disabled.
The retained completion owner uses a 1x1 private texture instead of a 4K output.
The separate single-spot-output primary variant currently retains native lighting;
its auxiliary u2 composition needs its own validated replacement contract.

GPU timing remains. New CPU timers separate scene readiness, retained scene/SBT
preparation and late submission validation. These will identify remaining CPU
costs; no FPS recovery or final-image correctness is claimed before in-game test.

Validation: Windows build, DXIL compilation, WARP native-buffer commit fixture
covering accepted/rejected/probe/NaN/negative/dimension-mismatch/zero pixels, and
existing resource lifetime and scene-order tests. Four-hit early roulette remains.
