# Building Control FG v2.1.1

Extract the source to a fresh directory on Windows. Install Visual Studio C++ build tools and a Windows SDK (Visual Studio 2026 is supported).

1. Run `Build.cmd`. It fetches pinned dependencies, verifies source, compiles the proxy and RTX 40-series sidecar, runs the existing validation suite and proxy smoke test, and packages the result.
2. Outputs include `build/dxgi.dll`, `build/ControlFG.RTX40MFG.dll`, `build/build-validation.json`, `release/Control-FG-v2.1.1.zip`, and `release/SHA256SUMS.txt`.
3. `Make-DropIn.cmd` can regenerate the deployment ZIP after a successful build.
4. Install into a supported Steam, Epic, or GOG DX12 folder using [INSTALL.md](INSTALL.md). Disable Galaxy's overlay for GOG.

The v2.1.1 preparation workflow builds the committed source on Windows, verifies deployment contents, and creates a source ZIP from that same commit. It uploads build artifacts without publishing a GitHub release. Use `RELEASE_NOTES.md` for v2.1.1; the older `GITHUB_RELEASE*.md` files and release-note workflows refer to historical releases.

The runtime baseline is the validated unified R3 source. The only changes under `src/` for v2.1.1 are version labels and the storefront description comment. Hotkey/slider experiments and GI investigation changes are excluded.

The retained `RR_NATIVE_G12_R20P.md` and milestone validation records are historical test inputs, not current build instructions.

## Support and developer tools

The source root keeps only `Collect-ControlFG-Compact-Logs.cmd` and its `.ps1` implementation for normal support reports. The duplicate `Collect-Logs.cmd` and `Collect-ControlFG-Logs.cmd` shortcuts were removed.

Advanced collectors and storefront probes are in [tools/diagnostics](tools/diagnostics/README.md). They are excluded from the deployment ZIP. The alignment collector belongs to the historical R12 diagnostic build and cannot collect a normal v2.1.1 run.
