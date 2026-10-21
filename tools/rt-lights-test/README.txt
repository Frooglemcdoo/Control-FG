Control per-light RT shadow backend - hardware test 1

This is a standalone GPU test, not an installable game mod.
Extract anywhere, then double-click Run-Per-Light-Test.cmd.
No administrator privileges are needed. Send Per-Light-DXR-Test.log.
It does not read or change Control, renderer.ini, or your installed DLLs.

The new backend traces separate receiver/light pairs and applies their visibility
to separate diffuse/specular contributions. The test creates its own geometry,
lights, shader tables and acceleration structures on your DXR-capable GPU.

Checks: overlapping blocked/visible lights, alpha-test rejection, geometry beyond
the light, instance masks, partial visibility at a soft edge, and rejection of
invalid/stale/incomplete lighting results. It does not validate Control's native
alpha materials, scene coverage, native light extraction, temporal filtering or
the game's final appearance. Those integration steps are still required.

Current game builds still use the older selected-light path. This test does not
enable all-spotlight shadows or claim to fix the blotch in the game.
