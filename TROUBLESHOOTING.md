# Troubleshooting — v2.1.0

Start with KNOWN_ISSUES.md for the RR blinds artifact, overlay pacing report, and Windows security report.

For support, run `Collect-ControlFG-Compact-Logs.cmd` after reproducing the problem. Include GPU, driver, resolution, HDR state, FG mode, RR model, and which ray tracing options are enabled.

When isolating a conflict, test Control FG without separate ReShade/RenoDX RR components. Keep the bundled runtimes inside `ControlFGStreamline`.

If HDR switching causes ghosting or corruption, restart Control with the desired setting. If RR or FG fails to activate, attach compact logs rather than replacing individual bundled DLLs.
