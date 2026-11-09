# R199: native free camera for window-light captures

This experimental build adds FREE CAMERA and NORMAL CAMERA buttons to F10.
The camera passes through walls using Control's built-in developer camera.
Jesse stays in place. Time keeps running. The accepted Native Blend, RR,
SPEC Native and hit-distance-OFF baseline is retained, along with R198's paired
window-blocker probes. This build is a diagnostic, not a lighting fix.

## Capture outside and inside

1. Launch normally with RR and Ray Traced Contact Shadows enabled. Keep Native
   Blend, SPEC Native and hit distance OFF.
2. At the affected window, F10 -> TRACE SHADOW. Hold still through completion.
3. F10 -> FREE CAMERA. Move through the window into the room. A controller is
   recommended: Y/A move up/down and X/B move slower/faster. Mouse wheel changes
   camera speed. These are the game's native controls, not new mod bindings.
4. Stop moving, then F10 -> TRACE SHADOW again. Keep the camera still. Send both
   Shadow-R199 folders (lighting.log, manifest.txt and three PNGs each).
5. F10 -> NORMAL CAMERA returns to Jesse and restores player controls through
   the native setter. Do not switch into Photo Mode or freeze time for this test.

The panel closes after a camera request. If nothing changes, reopen it to see
the camera status and use SAVE LOG. Finish/cancel captures before switching
cameras. F9 cancels an active light capture. A queued camera request expires
after three seconds without execution; it will not unexpectedly activate later.

Camera movement can change view-based culling. It does not necessarily move
player-driven streaming bounds because Jesse has not moved. The inside/outside
captures help separate these possibilities but cannot alone prove a culling fix.

## Implementation and limits

The uploaded Steam executable exactly matches target-manifest.json:
SHA-256 9441db3ae75b267abd989846ad0895e3fe24abcc0f06e58f93c34fc8d4736506.
Its camera-mode setter is RVA 0x384540. The unique signature from the old freecam
tool identified it; the implementation here was independently written after
inspecting this executable. No third-party tool source or game executable is
included in the repository or package.

One existing call at RVA 0x383c99 is redirected to a wrapper. It calls the native
camera input/update routine (0x384ba0) exactly once, then services a queued camera
request before the caller reads the active camera transform. The original
routine itself calls the same setter while handling native camera controls.
No setter is called on the UI, render-Present, or an arbitrary worker thread.
The wrapper services requests only on its first observed native update thread.

Installation runs during the existing hash-verified initialization before the
first DXGI factory returns. It checks exact instruction bytes and expected
imports. Only the verified Steam target enables this hook; other storefronts
retain the existing lighting behavior with camera controls unavailable.
Requests require normal/free mode, initialized cameras, an input manager, a
session, player controller and avatar. The native setter normally falls back
to free mode when no avatar exists, so requests are rejected in that condition.
After a switch, the mode and native free-camera/player-input flag are checked.
No direct camera-transform, time-scale, player-position, shader or culling writes
are added. Idle cost is one small CPU wrapper per native camera update; there
are no new per-frame GPU passes or resources.

Captures record camera mode (0 normal, 1 free, -1 unobserved) and generation.
Native Blend traces cancel on an observed camera-mode change. Within-mode
camera movement still requires the user to stop moving for the stationary test.
The camera stays where it is after capture completion; use NORMAL CAMERA to exit.

R198's original mask-1 ray and paired force-opaque triangle queries (mask 1/255)
are unchanged. See native-lighting-r198.md in the repository for interpretation
and capture-only resource costs. Those queries are not gameplay replacements.

Portable tests cover queued requests, expiry, gameplay/mode guards, explicit
normal-mode input restoration, verification failure and external mode changes.
Windows tests compile the integrated runtime and exercise its wrapper with
mock engine objects. These do not prove in-game movement or streaming behavior;
that remains the purpose of this experimental build.
