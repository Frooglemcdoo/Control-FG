Control FG - Integrated Primary PT M12R5

Experimental test build. Windows validation does not establish in-game PT coverage.

R4 evidence: zero submitted primary PT dispatches, 7215 copy failures,
26.2 seconds accumulated in scene-history checkpoints, 63 ms worst checkpoint.

Changes:
- Replace per-key/per-root history scan with interval queries; preserve exact
  predecessor/successor writes, tombstones and queue ordering. Cleanup is batched.
- Do not copy repeated dynamic BLAS updates with PT OFF. Record unknown versions
  so old deformation cannot be reused. Initial static builds are still preserved
  because the engine may not rebuild them when PT is enabled later.
- Native default read flags no longer override buffer decay when no local
  transition was recorded. Explicit transitions and pending splits still win.
- Keep the pass's bound render extent rather than replacing it with a TLS extent
  that may describe output resolution. Compare only consumed affine matrix lanes.
- Skip PT surface setup when no producer exists. Report precise view mismatch
  offsets and split copy rejection reasons rather than one combined message.

Install:
1. Close Control. Back up your current dxgi.dll and pt_*.dxil / pt_*.dxbc files.
2. Extract this package beside Control_DX12.exe, replacing included files only.
   Keep the rest of your mod and Streamline installation.
3. Launch DX12 and load the entrance with native ray tracing enabled.
4. F10 opens the overlay. Use its PT Reflections toggle: OFF for about 20 seconds,
   then ON for about 20 seconds. Note both FPS readings and any visible change.
   Leave F11 in normal mode. If performance is unusable, stop without extending
   the test; existing logs will still show the failure.
5. Close Control, run Collect-PT-Primary-M12R5.cmd, and upload its results ZIP.

The collector distinguishes recorded/submitted work from actual GPU-covered
samples. A successful build or recovered FPS alone does not prove working PT.
No old M6-M11 diagnostic scripts are needed.
