M12R55 - reuse retired PT shader-table and camera upload allocations

R54 measured 7,560 completed PT samples. DLAA / Quality averages:
PT ray GPU: 3.970 / 1.887 ms. CPU preparation: 7.199 / 7.161 ms.
Scene preparation: 3.877 / 3.880 ms; shader-table creation: 1.319 / 1.293 ms;
camera upload: 0.401 / 0.407 ms; metrics allocation: 1.424 / 1.412 ms.
These overlap other work and do not sum to total frame time or prove the entire
40 FPS limit. They do establish substantial resolution-independent CPU cost.

This build reuses shader-table and camera UPLOAD buffers after their last
recording owner is gone AND the Output retirement fence has completed. Existing
ownership retains them while submitted or replayable. Unknown completion keeps
existing quarantine behavior. No live buffer is resized or overwritten for reuse.
The pool matches device identity and 64 KiB capacity, capped at 32 MiB / 16 idle
buffers. Each reuse overwrites the consumed SBT/camera ranges. Scene validation,
resource retention, metrics buffers, ray count and shader output are unchanged.
R54 timing windows remain and new pool counters show whether reuse takes effect.

Close Control and extract all package files beside dxgi.dll. Start with PT on.
Use the same scene/camera and keep native RT, RR and FG unchanged. Test DLAA for
30 seconds, Quality for 30 seconds, then DLAA for 30 seconds. Note FPS. Close
Control and run Collect-PT-Primary-M12R55.cmd; send the results ZIP.
PT selection remains persistent and restart-only. No enable script needed.
This targets measured allocation overhead; FPS improvement remains unverified.
