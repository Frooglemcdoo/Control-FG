# M8 results — 12 October 2026

Source: PT-Scene-M8-Results-20261012-150109.zip. The capture has no end record.
The probe log ends roughly 160 seconds after observer startup, before its
180-second deadline. Treat this as partial evidence, not a completed test.

Retained records: 55,708 geometry rows, 8 complete binding samples / 37,207
bindings, 4,318 creations, 6,301 releases, zero reported read errors.
Historical geometry candidates: 35,334 unique, 1,817 ambiguous, 56 missing.
Geometry quota reached; end counters absent, so dropped/call totals unknown.

A chronological range-coverage analysis using the observed creation metadata
found no missing allocation ranges for sampled geometry indices/positions or
binding indices/attributes. This does NOT establish current resource lifetime.
Wrapper-keyed observed ranges gave:

| Range | One candidate | Multiple candidates |
|---|---:|---:|
| Geometry index | 55,678 | 30 |
| Geometry position | 38,235 | 17,473 |
| Binding index | 36,837 | 370 |
| Binding attributes | 36,778 | 429 |

There were 25 wrapper-matched releases and 1,980 nonnull releases without a
tracked wrapper; many other releases were empty. A resource-pointer keyed
alternative still left 1,970 nonnull release events without a tracked resource.
This means the observed creation/release stream is not a complete live registry.
Do not use its apparent range coverage as proof of ownership or pick overlapping
candidates arbitrarily.

M9 therefore tests a bounded owned COM reference obtained while the native
owner still holds its resource, then validates and drops that reference after
the original release helper returns. No GPU commands or fence tracking added.
M9 is not a production scene resource registry. Queue/fence integration and
unambiguous scene mappings remain required before rendering.
