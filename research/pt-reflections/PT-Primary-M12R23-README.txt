Control-FG Primary PT M12R23 - late copy-queue admission

Records PT with a zero GPU admission word. If the native graphics submission still waits for a future copy-queue signal, retains the immutable scene proof and revalidates it before that signal is forwarded. Only an exact unchanged scene can enqueue admission on the copy queue before the releasing signal. Rejected scenes keep native fallback. Replay cancels admission. Scene lifetime protection starts after admission; reserving retirement does not block the copy producer. No CPU drain or blocking wait is added.

Windows/WARP tests cover a graphics consumer submitted before the copy producer, two fence values, zero-guard rejection, admission, replay cancellation, and scene version changes. These tests do not establish in-game visual quality or performance.

Replace all files. Use F10 to enable PT Reflections, test for 10 seconds, report off/on FPS, then run Collect-PT-Primary-M12R23.cmd and send its ZIP. F11 is unchanged. PT_PRIMARY_SUBMITTED late_signal=1 and nonzero PT_PRIMARY_GPU coverage are the first in-game activation checks.
