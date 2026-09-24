# Epic Games Store compatibility work

This branch contains a **read-only compatibility collector** for adding Epic Games Store support without weakening Control FG's existing build lock.

## Why this is required

Control FG currently validates exact SHA-256 identities for:

- `Control_DX12.exe`
- `d3d_rmdwin10_f.dll`
- `renderer_rmdwin10_f.dll`

The runtime also uses build-locked internal RVAs in the D3D and renderer modules. Epic support therefore must be added as an explicit target profile, not by disabling the hash checks.

## Run the collector

Close Control, then from the repository root run:

```bat
Collect-Epic-Compatibility.cmd
```

The script auto-detects Epic and Steam installs when possible. To specify them directly:

```bat
Collect-Epic-Compatibility.cmd --epic "D:\Epic Games\Control" --steam "D:\SteamLibrary\steamapps\common\Control"
```

It creates `compatibility-output\Epic-<timestamp>.zip`.

The ZIP contains **metadata only**: hashes, PE layout, export/RVA comparisons, and a source literal inventory. It does **not** copy game binaries.

## Result classes

- `IDENTICAL_TO_VERIFIED_STEAM_TARGET` — all three locked files match.
- `SAME_RENDERER_AND_D3D_DIFFERENT_EXE` — likely a small storefront identity change; engine DLL/RVA contract remains identical.
- `STORE_BINARY_PROFILE_DIFFERS` — Epic needs a separate validated binary profile. Do not bypass the hash gate.
