# Troubleshooting — v2.1.1

For GOG input stalls or disappearing overlays, disable Galaxy's overlay for Control and restart the game; see [INSTALL.md](INSTALL.md#gog-galaxy-disable-the-overlay-for-control).

Start with KNOWN_ISSUES.md for the RR blinds artifact, overlay pacing report, and Windows security report.

For support, run `Collect-ControlFG-Compact-Logs.cmd` after reproducing the problem. Include storefront (Steam, Epic, or GOG), GPU, driver, resolution, HDR state, FG mode, RR model, and which ray tracing options are enabled.

When isolating a conflict, test Control FG without separate ReShade/RenoDX RR components. Keep the bundled runtimes inside `ControlFGStreamline`.

If HDR switching causes ghosting or corruption, restart Control with the desired setting. If RR or FG fails to activate, attach compact logs rather than replacing individual bundled DLLs.
