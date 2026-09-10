# Public release checklist — Control FG v1.0.0

## Required before publishing

- [ ] Build the final public source tree with `Build.cmd` on Windows.
- [ ] Confirm `Verify-Build.ps1` passes and `release\Control-FG-v1.0.0.zip` is created.
- [ ] Launch a clean Control Steam DX12 install with only Control FG's `dxgi.dll`/`ControlFGStreamline` present.
- [ ] Confirm overlay is hidden at startup and F10 opens/closes it.
- [ ] Confirm one fixed mode (recommended 3x/4x on RTX 50-series or 2x on RTX 40-series) produces the expected effective multiplier.
- [ ] Confirm Dynamic Auto works on supported hardware.
- [ ] Toggle HDR off/on once during gameplay and confirm FG recovers.
- [ ] Restart the game and confirm the saved mode/target is restored.
- [ ] Test uninstall/vanilla recovery.
- [ ] Capture at least one clean 16:9 in-game overlay screenshot for Nexus/GitHub.
- [ ] Publish `Control-FG-v1.0.0.zip` and its SHA-256 from `release\SHA256SUMS.txt`.
- [ ] Choose a license for the original Control FG source before granting source reuse/redistribution rights. If no license is chosen, leave the repository without a project LICENSE and do not describe it as open source.

## Nexus Mods fields

- Name: `Control FG - DLSS Frame Generation 2x-6x + Dynamic MFG`
- Version: `1.0.0`
- Category suggestion: Visuals and Graphics / Performance
- Main file: `Control-FG-v1.0.0.zip`
- Description: use `NEXUS_MODS.md` or `NEXUS_MODS_DESCRIPTION.txt`.
- Installation/troubleshooting: included in release ZIP and available as separate docs.

## GitHub

- Suggested repository name: `Control-FG`
- Suggested description: `DLSS Frame Generation, Multi Frame Generation and Dynamic MFG mod for Control DX12.`
- Release tag: `v1.0.0`
- Release body: `GITHUB_RELEASE.md`
- Attach the binary release ZIP and optionally a generated source archive.
