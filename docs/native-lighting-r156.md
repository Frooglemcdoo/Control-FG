# R156 — capture preparation and overlay focus

Verified the three R155 PID 5688 diagnostic files against capture-index SHA256. Both nine-buffer slots prepared successfully. Neither frame was recorded or exported. Request at 122794.887 s; preparations completed at 122801.524 and 122806.547 s; overlay reopened at 122811.468 s. No GPU/export failure is logged. The collector correctly reported an incomplete pair.

The settling clock was evaluated only when a slot was selectable and the worker lock could be taken. Allocation gaps therefore restarted the five-second delay, repeatedly. R156 observes evaluated frames before the nonblocking lock attempt, prepares slots immediately, and gates only recording on five seconds of settled gameplay. Actual evaluation gaps/resets still restart settling. No worker accesses the settling clock.

The diagnostic panel now opens without activation and preserves Control foreground focus to avoid causing the focus-loss pause in the first place. Clickable owner-drawn static controls avoid button focus changes. CAPTURE LIGHT hides the panel; reopen F10 afterward to check saved pairs. Cursor visibility and clipping are restored on closing. This does not dismiss a native pause menu deliberately opened before F10; start from gameplay. No synthetic keys or native pause-state writes.

Native RR Off capture, source-state locks, exact barriers/restoration, consecutive-frame validation, rendering shaders, exposure and package size limits remain unchanged. Windows validation cannot prove Control-specific mouse/raw-input interaction; in-game confirmation is required.
