# R138 — remaining local maps and bent normals

R137 completed 18 modes and 54 screenshots with zero capture failures. Both
contact-visibility and direct-light screen-occlusion variants were recorded;
the visible dark region remains in those comparisons and with RR off. The
capture does not establish which remaining contribution causes it.

## Compare live

- **Default (Clip):** unchanged authored-clip lighting and GI filtering.
- **Local Maps Off:** neutralizes the producer's nonselected spotlight map weight,
  producer point-light map weight, and selected point-light map weight. Default
  already bypasses the selected spotlight map. RT visibility, sunlight shadows,
  screen occlusion, light cookies and material shading remain. This intentionally
  removes some legitimate shadows as a diagnostic; it is not a proposed fix.
- **Bent Normal Off:** replaces bent-normal minus surface-normal deltas with zero
  in both direct-light passes. The existing normal blends therefore use the
  decoded surface normal. Scalar screen occlusion and all shadow visibility
  remain. This does not disable every indirect-light bent-normal use in the game.

Native, Both, Default, Light Off, Contact Off, Screen AO Off and Light IDs remain
in the overlay. RR-off, history and raw comparisons remain available through F9.

## Capture

Install with Control closed. Keep RT Contact Shadows enabled, game RT sample
count 1 and existing RR/GI settings. Return to the problem location and press F9.
Keep the camera still until completion (roughly 90–110 seconds). The suite runs
20 modes, up to 60 PNGs and the existing GPU hit/light-context capture. Zip and
send the displayed `Shadow-R138-...` folder. It restores the original mode;
F9 again or focus loss cancels. No restart between modes is needed.

Both new modes require successful preparation and observed bindings in **both**
the producer and composition before screenshots are accepted. Unavailable modes
are explicitly skipped. Local-map bypass can change native point-light ranking;
it does not promise to freeze every selected light ID.

## Implementation and checks

Four optional embedded shader variants use full SHA256 and exact instruction
word checks. Edits replace existing instructions with constant moves plus NOP
padding, without shifting any offset or changing resource declarations. Bent
normal edits change five subtraction sites; local-map edits change three weight
sites. Every byte outside these instructions and the container checksum must
remain identical. Existing R137 generated variants remain byte-identical.

Default's authored ray shader, all shipped spotlight sidecars, sample counts,
material guides and skin/glass behavior remain unchanged. New modes reuse the
same authored-clip ray record. Mode storage grows from six to seven bits; shared
shift constants move observation/sequence fields together, with boundary and
stale-observation tests. Optional PSO creation uses the existing shared helper;
failure cannot disable the working Default route.

Validation includes decoded operand/consumer checks, unsupported-input rejection,
mode/capture readiness tests, WARP shader creation and live routing. In-game
results are still needed to identify the shadow source.
