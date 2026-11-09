# R146 user capture review

The uploaded R146 motion trace completes six samples in approximately 24 seconds, with zero suite failures. All 24,576 records decode successfully, including same-present composition provenance. This validates the independent-submission capture correction in this run; it does not resolve the room-transition shadow bug.

Captured presents: 6355, 6583, 6817, 7038, 7266, 7495. The principal emitter geometries remain sampled across all six frames, despite light-index renumbering. Sample-grid coverage varies with movement, so frame-wide visibility averages are not a temporal stability measurement.

Exploratory comparison uses mutually nearest world-space receiver positions in adjacent samples, restricted to geometrically compatible selected lights. At distance below 0.01 engine units, 88 pairs match; their maximum visibility difference is approximately 0.0005. At distance below 0.03, 822 pairs match, with five dark-to-lit differences above 0.5, all between samples 3 and 4. No reverse changes above 0.5 occur in this comparison. These are nearby points, not proven identical surfaces or tracked scene objects. Shadow boundaries, moving geometry and different receivers remain confounders.

Conclusion: this trace does not show a broad loss of selected-spot visibility at closely matched receivers. It does not exclude local shadow failures, unsampled pixels, other lights, or changes between the four-second samples. The composition values are accumulated native diffuse/specular output before tone mapping, not isolated light contribution or final exposure. Next investigation should follow shadow contrast through lighting accumulation and downstream image processing, while checking the five local outliers. Do not change exposure or accepted shadow geometry on the basis of global averages.

## Input hashes

- `lighting(2).log`: `df9366e661f024244f2749d4d3d963b618572cc612d9c6fa42542a4890a5c85c`
- `manifest(2).txt`: `41f4e15f1904f490fec010e5bb16fec6df90f4169a252e94808838e24df09c0e`
