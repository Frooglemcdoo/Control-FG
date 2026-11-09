# R334 Owned GI

The live pass combines reservoir validation and final resolve. Each output thread now loads a shaded ray once for both calculations. The two intermediate buffers and their dispatch are removed. A UAV barrier orders the native writer before the first tile and another orders all tiles before the native consumer. Tile scratch still transitions at each dependency.

The resolve inherits u0 (raw GI RGB, alpha one) and u1 (mean hit distance) from the captured GI6 table. All six rays, full render resolution, and per-pixel fallback are retained. Native GI6 still executes first. Install over R333 and use the existing Owned launcher. The F10 recording is optional.
