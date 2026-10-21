# R69: diagnostic stall reduction

The user reports periodic pauses since the diagnostic overlay was introduced,
and one full system lockup. The code audit found synchronous WriteFile and
FlushFileBuffers calls in diagnostic Trace, reached from D3D/render hooks while
other locks may be held. It also found shader serialization/file writes on D3D
creation callbacks. These are stall risks; they do not prove the lockup cause.

Render-side Trace now only formats and attempts to enqueue a bounded line. It
never waits for the queue lock; contention/full capacity drops lines and reports
the count. The UI thread drains every 500 ms using buffered writes. SAVE LOG
explicitly flushes. Trace no longer duplicates messages into the main logger.
Shader blob capture is disabled, with that status recorded in the session log.
Shadow replacement hooks and all R68 shader assets remain unchanged.

The overlay caches its game window and hides/repositions only when needed.
There is no repeated window enumeration while a valid game window exists.
Buffered logs may lose their tail on a system crash; SAVE LOG forces a flush.

Install with the game closed. Check normal movement first with the panel closed,
then open it and compare the two shadow modes. Save the Lighting-R69 log if the
session remains responsive. This build is not a confirmed fix for the system lockup.
