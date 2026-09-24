# GOG compatibility

The GOG EXE identity beginning `57A8912F` is supported in v2.1.1. Its D3D and renderer DLLs match the verified Steam files byte-for-byte. The unified R3 build was user-tested on GOG. **Disable Galaxy's in-game overlay for Control**; see [installation instructions](../INSTALL.md#gog-galaxy-disable-the-overlay-for-control). The collector below remains available for investigating other builds.

Run:

```bat
tools\diagnostics\Collect-GOG-Compatibility.cmd
```

The collector attempts to find the GOG install from GOG registry entries and common GOG/Galaxy install paths. If detection fails:

```bat
tools\diagnostics\Collect-GOG-Compatibility.cmd --gog "D:\GOG Games\Control"
```

If Steam or Epic is installed locally, the collector also compares the GOG renderer/D3D binaries directly against that already-validated reference.

Output:

`compatibility-output\GOG-<timestamp>.zip`

The report contains hashes, PE metadata, exports and exact byte-window/RVA comparisons. It does not copy game binaries.

A GOG target is added to the runtime only after a complete three-file identity is known and the build-locked renderer/D3D contract is validated.
