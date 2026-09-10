# Troubleshooting — Control FG v1.0.0

## F10 does not open the overlay

- Confirm the game was launched using **`Control_DX12.exe`**. Control FG does not support the DX11 executable.
- Confirm `dxgi.dll` is directly beside `Control_DX12.exe`.
- Confirm the complete `ControlFGStreamline` folder from the release is beside the executable.
- Close overlays/wrappers that may also inject through a local `dxgi.dll` and retest.
- If the game was updated, the current Control build may no longer match the verified target.

## The game will not start after installing Control FG

1. Close Control.
2. Temporarily move Control FG's `dxgi.dll` and `ControlFGStreamline` folder out of the game directory.
3. Launch Control DX12 without the mod.
4. If vanilla Control also fails, repair/verify the game before troubleshooting Control FG.
5. If vanilla works, reinstall a clean copy of the current Control FG release.

Do not overwrite or delete another mod's `dxgi.dll`. The first Control FG release does not automatically chain another DXGI proxy.

## Frame Generation says WAITING / GPU MAX is `--`

Capability information appears only after NVIDIA Streamline has initialized and Control has entered a usable rendering path. Load into normal gameplay for a few seconds.

If it never resolves:

- update the NVIDIA driver;
- confirm the GPU supports DLSS Frame Generation;
- verify **Hardware-accelerated GPU scheduling (HAGS)** is enabled in Windows;
- restart Windows after changing HAGS or the display driver;
- make sure all files in `ControlFGStreamline` came from the same Control FG release.

## 3x–6x or Dynamic are greyed out

This is normally a hardware/capability restriction, not a UI problem.

- **GeForce RTX 40-series:** Control FG intentionally exposes **Off and 2x only**. Dynamic and 3x–6x are disabled.
- On newer hardware, the overlay follows the capability maximum reported by the NVIDIA runtime. Modes above that maximum remain disabled.
- Dynamic is enabled only when the NVIDIA runtime reports Dynamic MFG support.

## Dynamic stays at 1x or does not behave as expected

- Make sure **Dynamic** is selected rather than a fixed multiplier.
- Try **Auto** first. Auto targets the refresh rate of the monitor containing Control.
- In Manual mode, choose a sensible output target relative to the game's real rendered FPS.
- Very high targets cannot compensate for extremely low base frame rates or a fully saturated GPU.
- Update the NVIDIA driver if Dynamic capability is unexpectedly unavailable.

Control FG intentionally handles Dynamic pacing internally; do not expect Control's original frame limiter to configure the Dynamic target.

## FPS is below the requested Dynamic target

The target is a goal rather than a guarantee. Output is limited by base-game render rate, the maximum generated-frame count supported by the GPU/driver, GPU headroom, CPU limits, ray tracing load and other presentation constraints.

For best results, start with a base frame rate that already feels playable before enabling Frame Generation.

## HDR looks wrong / switching HDR causes problems

Control FG contains a dedicated HDR presentation bridge because Control changes its swap-chain format during live HDR transitions.

Try:

1. return FG to **Off**;
2. switch HDR to the desired state;
3. wait a few seconds;
4. re-enable the desired FG mode.

The validated branch supports SDR → HDR and HDR → SDR transitions without restarting the game, but monitor/Windows HDR configurations vary. If the image remains wrong, restart Control after setting the desired HDR mode and collect logs.

## Stutter or poor Frame Generation quality

Generated frames still depend on a healthy rendered-frame stream. Reduce settings that heavily lower the base frame rate, especially expensive ray-tracing options, then retest.

Also check:

- background GPU-heavy applications;
- NVIDIA driver updates;
- third-party frame limiters/overlays;
- whether a very high fixed multiplier is appropriate for the current base FPS.

## Settings are not being remembered

Control FG saves normal user settings here:

```text
%LOCALAPPDATA%\ControlFG\settings.ini
```

If the file cannot be written, check Windows permissions/security software. To reset Control FG preferences, close Control and remove `settings.ini`; the mod will recreate defaults on the next run.

## I previously used the experimental latency build

The abandoned v0.8.30 latency experiment used PresentMon. Current builds do **not** use it. If an old `ControlFGPresentMon` folder remains in the game directory, it can be removed after closing Control.

## Collecting logs for a bug report

Use the included:

```text
Collect-ControlFG-Logs.cmd
```

It gathers a small recent set of Control FG and Streamline logs plus `settings.ini` into a timestamped ZIP beside the script. Review the ZIP before uploading if you want to inspect what is being shared.

When reporting a problem, include:

- GPU model;
- NVIDIA driver version;
- Windows version;
- whether HAGS is enabled;
- SDR or HDR;
- selected FG mode;
- resolution and monitor refresh rate;
- ray tracing state;
- exact steps that reproduce the problem;
- the collected Control FG logs ZIP.

## Unsupported game build

v1.0.0 is verified against **Control Steam DX12 build 21225456**. GOG/Epic versions and future game patches are not automatically considered compatible. If the game receives an update, do not bypass build safeguards just to force installation; report the new build so it can be checked first.
