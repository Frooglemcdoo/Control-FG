# GOG compatibility validation

GOG is validated as a separate storefront identity before being admitted by Control FG.

Run:

```bat
Collect-GOG-Compatibility.cmd
```

The collector attempts to find the GOG install from GOG registry entries and common GOG/Galaxy install paths. If detection fails:

```bat
Collect-GOG-Compatibility.cmd --gog "D:\GOG Games\Control"
```

If Steam or Epic is installed locally, the collector also compares the GOG renderer/D3D binaries directly against that already-validated reference.

Output:

`compatibility-output\GOG-<timestamp>.zip`

The report contains hashes, PE metadata, exports and exact byte-window/RVA comparisons. It does not copy game binaries.

A GOG target is added to the runtime only after a complete three-file identity is known and the build-locked renderer/D3D contract is validated.
