# R279: native GI geometry descriptor inventory

This diagnostic records the CPU descriptors passed to native ray-tracing
acceleration-structure build and copy calls. It does not change lighting,
geometry, native build parameters or the GI estimator. It is not a speckle fix.

## Run

1. Install all package contents beside Control_DX12.exe, preserving folders.
2. With the game closed, run ControlTOH/Diagnostics/Start-GI-Geometry-Probe.cmd.
3. Load the original plant/hair area with the lamp intact, RT indirect and RR ON,
   Native Blend selected, FG OFF, GI RAW BYPASS and REF CLAMP: BYPASS.
4. Press F7 once and wait for ARMED, then SAVED. Mouse zoom may remain held.
5. Exit normally and send the launcher-generated ZIPs. Do not use CAPTURE RR.

The inherited trace launcher and binary still identify their R278 accounting
format. R279 adds GI_GEOMETRY_R279 records to the normal collected lighting log.
The previous R278 capture remains valid; this run adds geometry bindings.

## Scope and limits

Records include vertex/index GPU addresses, formats, stride, counts, geometry
flags, optional transform address, AS source/destination addresses and copy mode.
TLAS instance addresses are logged but instance bytes are not read. There is no
GPU readback for this inventory. Native calls are forwarded unchanged once.

The inventory is capped at 512 build calls, 4096 geometry entries and 512 copy
calls. A limit explicitly marks the inventory incomplete. This is format and
binding evidence, not a complete resource-lifetime or primitive identity map.
It does not yet recover triangle bytes, associate instances with captured rays,
verify proposal support, or enable sample reuse. Normal launch leaves it off.
