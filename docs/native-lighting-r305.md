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
