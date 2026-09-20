# Troubleshooting — Control FG v2.0.0

## F10 does not open the overlay

- Confirm you launched `Control_DX12.exe`.
- Confirm `dxgi.dll` is beside `Control_DX12.exe`.
- Confirm the complete `ControlFGStreamline` folder is installed.
- Temporarily remove other wrappers/injectors that may also use `dxgi.dll` and retest.

## Ray Reconstruction does not enable

- Make sure ray tracing is enabled in Control.
- Toggle RR off/on once from the F10 overlay.
- Try both RR Model **F** and Model **E**.
- Make sure the bundled RR runtime files are present inside `ControlFGStreamline`.
- If another graphics injection or rendering modification is installed, test Control FG by itself before reporting the issue.

If the problem continues, run `Collect-ControlFG-Compact-Logs.cmd` immediately after reproducing it and attach the generated ZIP.

## RR image quality looks wrong

If RR suddenly looks unusually plastic, noisy, unstable, or very different from the normal v2.0.0 result:

1. Compare Model F and Model E.
2. Toggle RR off/on once.
3. Confirm another injector is not replacing Control's temporal or NGX path.
4. Reproduce the issue once and collect compact logs.

## HDR + Frame Generation issue

v2.0.0 includes the final HDR/SDR Frame Generation recovery path. If switching Windows HDR on/off still corrupts generated frames or FG fails to recover:

1. Leave FG enabled.
2. Reproduce the problem once.
3. Run `Collect-ControlFG-Compact-Logs.cmd`.
4. Attach the resulting ZIP with a short description of whether the transition was SDR→HDR or HDR→SDR.

## Performance

The largest expected RR performance cost is at **4K + DLAA**, where testing showed roughly a **20–30%** reduction with RR enabled.

At **1440p and 1080p**, the difference is usually much smaller, generally around **2–10%** and sometimes difficult to notice.

## Verbose support logging

Normal release logs intentionally suppress high-volume diagnostics.

For a specific support investigation, you can enable verbose logging before launching the game:

```powershell
setx CONTROLFG_VERBOSE_LOG 1
```

After collecting the requested run, disable it again:

```powershell
setx CONTROLFG_VERBOSE_LOG 0
```

A new terminal session is required after changing the environment variable.
