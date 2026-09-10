# Building Control FG from source

## Requirements

- Windows x64.
- Visual Studio with **Desktop development with C++** and a Windows SDK. The build script uses `vswhere.exe` and `vcvars64.bat`; Visual Studio 2022/2026 toolchains are suitable when the required C++ components are installed.
- Internet access on the first build to retrieve the pinned official NVIDIA Streamline SDK 2.14.1 release.

## Build

From an extracted source checkout:

```bat
Build.cmd
```

`Build.cmd` performs the following:

1. stages NVIDIA Streamline 2.14.1 from the official NVIDIA-RTX/Streamline GitHub release;
2. verifies the pinned SDK archive and staged file hashes;
3. checks the C++ ABI used by Control's imported DLSS symbol;
4. compiles the x64 DXGI proxy;
5. checks proxy exports/imports and runtime identity markers;
6. runs a DXGI smoke test;
7. creates the binary drop-in and public-release ZIPs.

Generated artifacts are ignored by Git and appear under `build\` and `release\`.

## Supported Control build

The current source is build-locked to Steam build `21225456`. `target-manifest.json` records the tested hashes. The source installer refuses to install against changed required game binaries. A manual binary drop-in does not replace this requirement; unsupported game versions are simply not claimed compatible.

## Streamline pin

- Version: `2.14.1`
- Official release archive SHA-256: `92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B`

Do not silently replace the pinned Streamline binaries when reproducing a release build.

## Runtime validation baseline

The core runtime baseline is v0.8.26, where fixed 2x–6x, native Dynamic MFG, live HDR transitions, and live multiplier switching were exercised successfully. v0.8.27–v0.8.35 were release/UI/policy revisions that did not intentionally change the proven generation core. v1.0.0 promotes that stabilized branch to the first public release.

## Source licensing

Third-party components retain their own licenses. See `legal/`. The project owner has not yet selected a license for original Control FG source code; choose and add one before granting third parties reuse/redistribution rights.
