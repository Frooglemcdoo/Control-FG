# Changelog

## v1.0.0 — first public release

First public release of Control FG.

- Fixed DLSS Frame Generation modes from 2x through 6x on supported hardware.
- Native Dynamic Multi Frame Generation with Auto monitor-refresh targeting and a Manual 30–1000 FPS output target.
- Working Dynamic MFG pacing path using NVIDIA Reflex and PCL `SimulationStart`.
- Live SDR/HDR switching, including the RGB10 presentation bridge used by Control's HDR lifecycle.
- Persistent settings stored in `%LOCALAPPDATA%\ControlFG\settings.ini`.
- F10 overlay that starts hidden.
- Control-inspired black/white/red UI, embedded CONTROL FG branding, current FPS, effective multiplier, HDR state and GPU maximum.
- RTX 40-series policy: Off + 2x only; Dynamic and 3x–6x are disabled.
- Build-locked installer and runtime safeguards for the verified Steam DX12 target.
- Public build scripts now fetch and verify the pinned official NVIDIA Streamline 2.14.1 SDK.
- Release packaging, checksums, troubleshooting documentation and log collector added for public support.

### Runtime baseline

The generation core is based on the runtime-proven v0.8.26 implementation. Later pre-release revisions focused on persistence, UI and hardware-policy polish rather than changing the stabilized FG core.
