Control-FG Primary PT M12R25 - graphics composition and bounded transport validation

R24 did trace rays: about 32,200 of 32,400 sampled primary pixels passed material/depth matching. All integrated samples were rejected, while supported graphics draws failed the compute-context-based surface check. Full-resolution tracing therefore cost time without visible output.

R25 reserves a bounded block of 64 private surface tables through the verified lighting allocator. Graphics draws use their tracked root bindings and the retained camera, then revalidate at submission. No compute TLS context is required at a graphics draw. Native descriptor ranges are retained and no native table is overwritten.

Tracing starts with one pixel per 16x16 block. The validation pass keeps output alpha zero. Full-frame tracing starts only after GPU readback contains valid radiance; an all-invalid full frame returns to sparse validation. Detailed material/light/normal/transport rejection counters distinguish the still-unresolved shader fault. This build fixes composition routing and limits wasted tracing; it does not claim the universal shader rejection is fixed.

Replace all files. Enable PT Reflections through F10 for 10 seconds, report off/on FPS, then run Collect-PT-Primary-M12R25.cmd and send its ZIP. Look for PT_PRIMARY_SURFACE_RECORDED, PT_PRIMARY_FAULTS and PT_PRIMARY_TRANSPORT_READY. Windows/WARP tests do not establish in-game visuals or FPS recovery.
