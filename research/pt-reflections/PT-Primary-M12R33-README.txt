Control primary PT - M12R33

Close Control and replace all package files. F10 overlay and F11 are unchanged.
Compare the same room with PT off/on, then run Collect-PT-Primary-M12R33.cmd.

R32 screenshots show 49 FPS off and 37 FPS on, with mainly a brightness increase.
Its log confirms the validation fast path is active, but PT still adds diffuse
indirect to native lighting. This is not a completed PT replacement. R33 does
not claim to fix that composition or the unfinished denoising/material coverage.

R33 targets geometry-copy overhead shared by RT and PT. Capture previously
copied entire index allocations for each BLAS. It now copies the needed index
span, rebases private hit-table roots/offsets, and narrows private raw-load bounds.
Original allocation bounds, exact AS versions, retained geometry and late queue
admission checks remain enforced. Off-mode updates remain current for PT toggles.
New copy-byte counters allow the next capture to measure actual copy volume.

Regression coverage includes shared index/position resources, R16 halfword
alignment, private padding, truncated ranges, source overwrite/release, budget
rejection and fence retirement. Windows/WARP testing is not an in-game FPS result.
