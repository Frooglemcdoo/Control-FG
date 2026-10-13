Control-FG Primary PT M12R19

R18 confirmed off-mode geometry preservation works (2759 updates), but PT still did not submit: scene_row_index_owner blocked reachable 16-bit index buffers.
R19 captures native ownership using logical index bytes, and rounds private snapshot capacity up to a DWORD without reading beyond the native resource. Preflight separately verifies logical copied bytes and private raw-load capacity. The unused upper halfword is masked by the existing 16-bit decoder. Native RT fallback and generation checks remain active.

Install all files over R18. Launch Control DX12. Note FPS with PT off, then use F10 > PT Reflections for at least 10 seconds. Leave F11 unchanged. Run Collect-PT-Primary-M12R19.cmd and send the results ZIP. Windows/WARP validation does not prove in-game PT rendering.
