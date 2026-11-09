# R93: locate where shadow coverage breaks

Normal RT FILTERED and RT RAW rendering remain R92. This build adds three live
views to F10. They replace selected-light composition with signal RGB, bypassing
its material shading, light color, shadow maps and point-light accumulation.
They do not change rays, sampling, caster selection or normal-mode filters.

- RAW VISIBILITY: spotlight visibility with both spotlight filters bypassed.
  Black = blocked; white = visible; gray = partially blocked across four rays.
- FILTERED VISIBILITY: the same signal after spotlight temporal/spatial filtering.
- LIGHT ID COLORS: pseudocolor of the spotlight ID selected for each pixel.
  Colors are identifiers, not light intensity or fixture color. Similar colors
  can recur; this is not an exact numeric ID readout.
- Magenta in all three views means no selected spotlight (ID 65535). It is not
  a ray miss. A ray miss with a selected light appears white in visibility views.

These are signal views at the direct-light composition boundary, not a final
swapchain dump. Later lighting/postprocessing may still affect the display.
Disable RR and FG for the test; native resolution with DLSS off is preferred
so reconstruction does not disguise pixel steps. If normal textures or lighting
remain strongly visible, report that rather than treating the view as pure data.

Close Control to install every file. Enable RT Contact Shadows. At the affected
blob/staircase, hold the camera still and capture RT FILTERED, RAW VISIBILITY,
FILTERED VISIBILITY, then LIGHT ID COLORS. Wait a few seconds in each mode.
Repeat a short camera movement if the boundary pops. Use F10 or Ctrl+Shift+1..6
without restarting. Save one Lighting-R93 log after all views. Return to RT
FILTERED and restore normal graphics settings when finished.

Interpretation:
- Artifact already in RAW VISIBILITY: investigate rays, caster coverage and
  selected-light boundaries; adding filter blur is not a demonstrated repair.
- Appears only in FILTERED VISIBILITY: investigate filter radius, rejection,
  history and resolution assumptions.
- Tracks a LIGHT ID COLORS boundary: investigate per-pixel light assignment and
  how visibility is associated with that light; correlation is not proof.
- Both visibility views clean but rendered RT bad: investigate composition or
  subsequent lighting/reconstruction.

The shader uses the native t15.x visibility and t14.x light ID with unchanged
pixel coordinates. Synthetic WARP execution checks all pixels/channels, absent
light marking, and zero specular output. Runtime routing tests cover all modes.
PSOs are prepared during shader creation; no frame-time GPU waits, allocations,
readbacks or file writes are added. No new game capture is required for this test.
