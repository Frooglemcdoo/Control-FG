# R415 Balanced RELAX validation

Runtime commit: 00e85584c1f24a202530114cc80c04966872d1f6.
Build: https://github.com/Frooglemcdoo/Control-FG/actions/runs/42890347227
All 60 job steps succeeded. All four NRD tests passed, including Balanced D3D12 pipeline and profile-switch history reset / one fewer spatial dispatch.

Final Balanced settings: prepass radius 20 instead of 30 pixels, four instead of five spatial iterations, original luminance tolerance 2. Temporal accumulation and anti-firefly unchanged. Default Balanced; F10 cycles Balanced, Original, Off; session-only.

Rejected candidates: 12-pixel radius / three iterations failed unchanged moving-foreground stale-illumination threshold, both with luminance tolerance 1 and 2. The latter measured 0.15528488 at frame 101 against <0.14. Do not ship either candidate or weaken threshold.

Local portable cycle test and 132-file baseline verification passed. Artifact SHA256 5e87109fd94f5e6be4fea19c0745339ac344800a6cbe35f8e7db18404e0e349b. Package SHA256 50ce983ff4387ad3efa003c36b61bb93cb0c2e8bba3d93d690a697ca7eb76af4 (6809522 bytes). DLL SHA256 be67d9f136423f83d987eff22f12cd5c2d2aba4c6e2a52f158ea54ba8c295fdb. Archive CRCs, x64 PE and R415 RELAX labels verified.

Package saved as ControlTOH-R415.zip, Library libfile_69728224c3448191ae192a7403355eb2. Live game detail preservation, motion stability and FPS improvement are not yet verified. Compare identical camera then slow pan; no new large capture required for initial quality evaluation.
