# R305 — Complete GI writer and scene capture

This diagnostic build captures four complete GI material-binding tables, ten
seconds apart, alongside the existing instance, geometry and submission evidence.
It follows the actual GI dispatch through the native scheduler and copies its CPU
inputs before the worker starts. It preserves GI's two-row geometry spacing,
including separate opaque and alpha-tested identifiers. File output runs on a
background thread. No native GPU shader table is mapped or rewritten.

## Run

Install the complete package beside Control_DX12.exe, preserving folders.
Run `ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd` and load the usual save.
Keep the problem plant and Jesse's hair visible for the first minute, then move
around for another minute and exit normally. No capture hotkey or special GI
mode is needed. Preserve your current lighting settings. The launcher packages
logs and the complete binding/table files automatically. Send the resulting ZIPs.

This run answers which exact GI material records and shader identifiers accompany
its dispatch. R303 already supplied dispatch dimensions, setup, geometry and
instance evidence; it did not include these matched full CPU writer inputs.

## Scope

This is not a speckle fix. Replacement GI and native resource retirement remain
disabled. CPU copies do not prove GPU completion, allocation ownership or alpha
visibility. Existing 1 GiB buffer / 512 MiB AS diagnostic limits remain. The writer
adds at most four pending snapshots (approximately 64 MiB CPU memory at the
maximum binding count), with a bounded retry budget. Normal launch leaves the
capture consumer disabled.

Each successful snapshot produces `GI-Writer-R305-<pid>-<serial>.json`,
`-bindings.bin` and `-table.bin`. The metadata identifies the dispatch, pipeline,
engine frame, row format and explicit evidence limits. The collector includes all
three files, plus the existing logs, automatically.

## Validated package

Code commit: `3d0bb1e1d4038c810514a02b0b468f478a302105` (remote),
`1a198d1` (local), matching tree `c2b228dce1712ae39d87c675761cb4de85faa4c1`.
Windows foundation run `41660764470` / job `126003014534`: PASS.
Full Windows build run `41660764126` / job `126003014033`: PASS.
Artifact `13241538060` contains the 74-file install ZIP, 4,224,836 bytes.
SHA-256: `dc49e2100f1ef85e58fa1c94a7808fbb1d0a46ad8f0c898ea11164208e4a9094`.
The nested checksum, ZIP integrity, R305 binary markers, updated launcher,
collector and packaged instructions were verified after download. Hardware DXR
routing and visual behavior still require the requested game run. RT indirect
lighting must be enabled for GI dispatches to occur.
