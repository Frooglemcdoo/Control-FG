# R101: wall and painting RR material evidence

R100 GI FILTER is the confirmed lighting baseline. R101 preserves that branch, character/hair replay, and all 14 exact R97 shadow shader assets. This is a capture build, not a claimed oversharpening fix.

Install with Control closed. Select GI FILTER in F10, keep RR ON, then close the panel. Face the affected wall/painting, stand still for 10 seconds, press F9 once, and wait 20 seconds. SAVE LOG, close the game, then run Collect-RR-Material-Capture.cmd. Send its desktop ZIP and the saved Lighting-R101 log. No mode switching is required.

## Evidence and scope

The current diffuse guide falls back to a decoded material value used as F0; textured replacement is restricted to Character and Hair. A native albedo technique exists for Standard materials, but its output semantics and vertex compatibility must be audited before admitting it to replay. R101 captures original and candidate VS/PS bytecode before the Character/Hair filter. It does not treat a valid MRT layout as proof of diffuse albedo.

F9 captures up to 16 distinct Standard pairs and 8 other pairs over 120 preparation calls. At most one pair (four shaders, 1 MiB maximum) is copied in a call. Four fixed slots hold CPU-owned bytecode; worker callbacks create and write files, never dereference engine objects, and publish a complete marker only after all four files close. The producer never waits for a slot or disk. Descriptor identity, stage, size and resident-copy success are checked before publication. Capture retains the existing F9 two-frame RR input/output capture; its pre-existing GPU readback cost still applies. These captures are explicitly requested, not continuous.

Shader files live in %LOCALAPPDATA%\ControlFGProbe\rr-material-shaders-PID-FRAME. The collector includes the latest shader session and available RR input captures for that process. The saved Lighting log may need to be attached separately. Missing Standard pairs are reported rather than presented as a successful wall-material capture. The bounded capture does not identify the material under the crosshair; the affected surface must be visible and its shader must appear among captured candidates.

## Validation

The production audit header is exercised with deterministic platform mocks: no writes on the producer, copy budget, duplicate suppression, slot exhaustion, worker-owned bytes, descriptor mutation, queue failure, export failure and frame expiry. Windows CI compiles the proxy and runs existing renderer, menu, GI, character and WARP checks. Actual in-game capture completeness and the cause of stationary oversharpening remain unverified until user evidence returns.
