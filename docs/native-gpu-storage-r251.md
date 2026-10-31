# R251: targeted texture asset timing

R251 preserves the accepted R247 storage behavior and R250 publication observer.
It adds an optional, user-triggered metadata capture; it does not change demand,
priorities, budgets, transfer lanes, resource states or native completion ordering.

Copy the full package beside Control_DX12.exe, replacing previous mod files.
Use Start-CPU-R251-NativeGPUStorage-4ms.cmd (4 ms, 16 partitions pinned).
Load the area first. Press F6 just before approaching a blurry painting, sign or
picture. Approach and watch it sharpen. Capture accepts requests for 15 seconds,
then allows another 5 seconds for completion. Wait at least 20 seconds after F6,
then collect compact logs promptly and identify the location/object. No video,
Reference run or ETW capture is required. F8 is a different diagnostic: use F6.
Another F6 capture is possible after the previous one flushes; pressing it during
an active capture does not restart the window.

CONTROLFG_STORAGE_ASSET_TRACE=1 enables F6; setting it to 0 disables this feature.
The supplied launchers also enable latency and publication observation.
TEXTURE_ASSET_R251_ARM reports whether capture started. BEGIN/ROW/END records
are emitted by the storage worker after the 20-second window. At most 512
accepted requests are captured, with names limited to 511 bytes. Control
characters are sanitized. Overflow and lock-contention counts report lost
observations. Callbacks use try-locks and drop observations rather than wait.
The snapshot copy and deferred logging still add diagnostic overhead; a hitch
at flush time should not be attributed to ordinary storage operation.

Each row identifies the archive asset name, native level, priority, byte count
and correlation token. Timestamps are monotonic CPU-observed microseconds:
request_us is entry to the intercepted native request after engine selection;
admitted_us is successful admission; submit_us precedes batch Submit;
retired_us is observed transfer retirement before native finish/retry;
publication_us is matched native renderer consumer entry, before its state updates.
Result 1 means transfer success, 2 native retry, 3 terminal failure; 0 means no
retirement observed. Zero timestamps mean unobserved, not zero delay. Requests
still pending after the grace period have incomplete rows. Old capture tokens
cannot update subsequent captures. Publication matching retains R250 identity,
expiry and coverage limitations; see native-gpu-storage-r250.md in the repository.

This trace excludes native-only fallback requests, already-resident textures,
upstream demand decisions and actual first visible use. Asset names narrow the
candidates but do not automatically map a request to an object on screen.
No payload data or additional resource ownership is captured. This build is for
measurement; it does not claim faster texture arrival or improved FPS.

Tests cover capture bounds, boundary times, completion grace, name truncation,
control-character sanitization, event correlation, stale generation rejection
and publication token forwarding, alongside existing GPU/lifetime tests.
