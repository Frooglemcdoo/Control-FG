Control primary PT - M12R35

Close Control and replace all package files. Run Collect-PT-Primary-M12R35.cmd
after comparing RT and PT in the same scene. F10/F11 controls are unchanged.

R34's GPU dispatch fell from about 3.9 ms to 1.1-1.9 ms later in the capture,
while CPU preparation remained roughly 6.5-7.9 ms. That is consistent with the
reported weak resolution scaling, but is not a complete frame-time profile.
PT-off still preserved geometry updates, so PT-off was not a zero-cost bypass.

R35 removes unused graphics interception for the compute-only primary route:
graphics root signatures, tables, CBVs and both draw calls remain native on
newly watched interfaces. Earlier hooked interfaces stop collecting unused
graphics arguments. Compute bindings, resource transitions and queue ordering
remain observed in both modes. Full graphics consumers keep their old behavior.
Resource barriers no longer repeat QueryInterface just to register an already
known command list; the existing setter path still registers unseen aliases.
Dispatch leases reuse the live token index instead of rebuilding a map of all
resource owners on each frame. Windows tests verify skipped graphics hooks,
retained compute/barrier hooks, native forwarding and existing alias behavior.

These are CPU overhead fixes, not a claim that RT-off overhead is eliminated
or that the whole frame is now CPU-bound. In-game FPS must be measured.
Geometry updates are still preserved while PT is off: removing those copies
without a replacement would allow stale geometry after PT is re-enabled.

PT LIGHTING REPLACEMENT IS STILL UNFINISHED. Primary direct light and shadows
remain native; indirect PT is additive. This build does not claim improved
lighting, soft shadows, native indirect replacement or a PT denoiser. Lighting
replacement remains the next rendering goal before new shadow work.
