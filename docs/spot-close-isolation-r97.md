# R97: isolate the remaining close-shadow loss

The R96 user test reports no visible improvement. Its log records the corrected
ray path in both filtered and raw modes, with per_sample_endpoint=1 and
near_distance_cutoff=0. It records no R94-baseline switch. These are CPU submission
observations, not GPU readback proof. The screenshot shows RT CLOSE COVERAGE
selected and weak close shadows. Further cutoff tuning is not justified by this
result; preserve R94 selection and isolate the next cause.

## Controls and purpose

Install all package files while Control is closed. Enable native RT Contact
Shadows. Keep the camera and RR setting fixed during comparison. F10 switches
live; SAVE LOG writes one Lighting-R97 log.

1. RT R94 BASELINE retains the shipped R94 selected-light rendering.
2. CENTER RAW uses a zero-radius emitter sample and bypasses spotlight temporal
   and spatial filters. This intentionally makes the shadow test hard-edged.
3. SELECTED LIGHT OFF omits only the selected spotlight's final diffuse and
   specular additions. Other lights, including point lights, retain their path.

CENTER FILTERED is optional: it uses the same center rays with R94 filtering.
Send the same camera view in the three numbered modes and one log. These controls
are diagnostics, not proposed replacements for soft shadows. No restart between
modes, PIX capture, GPU readback or additional ray dispatch is required.

## Interpretation, not predetermined conclusions

- A shadow returning in CENTER RAW points toward disk sampling or filtering.
  CENTER FILTERED helps separate those two effects.
- A shadow still absent in CENTER RAW, but substantial darkening in SELECTED
  LIGHT OFF, points toward ray geometry/coverage or later shadow processing.
- Little effect from SELECTED LIGHT OFF points toward other lighting dominating
  that area, or selection/composition not targeting the expected contributor.

Exposure and other downstream processing can affect those comparisons. The
remaining signal views still include downstream material/lighting processing;
they are not final-screen pure buffer dumps. Use RR and FG off for signal views.

## Implementation

R94 selector, original baseline ray, light radius data, normal composition,
spatial/temporal filters and other features remain byte-identical. The diagnostic
ray uses R96 emitter-bounded segments with all four disk offsets zero. Four rays
are retained to avoid changing the aggregation contract; they target the same
center. Zero-length segments retain a finite fallback direction.

The selected-off composition replaces exactly two spotlight multiply-adds with
moves of their existing accumulated-light operands. This avoids using zero times
a potentially invalid value. The remaining shader instructions, native point
branch and resource layout are unchanged. The variant is prepared with the other
composition pipelines and selected only inside the verified full RT pass.

State objects and ray records are prepared during creation. Switching selects
existing records/pipelines. No added render-time allocation, file I/O, GPU waits
or readbacks. Existing queued diagnostic logs record the mode and center probe.

## Validation and limitations

Signed DXIL validation; captured-IR point/hit-handler preservation; diagnostic
instruction and hash checks; Windows WARP pipeline creation and routing tests;
package verification. These tests do not establish real-GPU visual correctness
or explain the earlier reported green screen. Do not reproduce that crash.
