# Epic Games Store compatibility

Epic binary version `0.0.518.2177` is supported in v2.1.1. Startup, overlay, FG and RR were user-tested successfully. All three file hashes differ from Steam, while the validated engine contracts share the same layout. See [installation instructions](../INSTALL.md). The collector below remains available for investigating other builds.

## Why this is required

Control FG currently validates exact SHA-256 identities for:

- `Control_DX12.exe`
- `d3d_rmdwin10_f.dll`
- `renderer_rmdwin10_f.dll`

The runtime also uses build-locked internal RVAs in the D3D and renderer modules. Epic is admitted through an explicit target profile; the hash checks remain enabled.

## Run the collector

Close Control, then from the repository root run:

```bat
tools\diagnostics\Collect-Epic-Compatibility.cmd
```

The script auto-detects Epic and Steam installs when possible. To specify them directly:

```bat
tools\diagnostics\Collect-Epic-Compatibility.cmd --epic "D:\Epic Games\Control" --steam "D:\SteamLibrary\steamapps\common\Control"
```

It creates `compatibility-output\Epic-<timestamp>.zip`.

The ZIP contains **metadata only**: hashes, PE layout, export/RVA comparisons, and a source literal inventory. It does **not** copy game binaries.

## Result classes

- `IDENTICAL_TO_VERIFIED_STEAM_TARGET` — all three locked files match.
- `SAME_RENDERER_AND_D3D_DIFFERENT_EXE` — likely a small storefront identity change; engine DLL/RVA contract remains identical.
- `STORE_BINARY_PROFILE_DIFFERS` — Epic needs a separate validated binary profile. Do not bypass the hash gate.
