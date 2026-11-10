# R417: extend an existing skin-guide correction to audited native variants

R416 user report: projector, desk and plant good; hair speckles, red face speckles
at the end of video 20261110-2124-06.5896476.mp4, persistent jaw noise. The inspected
27.8–29.55s sequence shows a short red/orange burst during the turn, then settling.
This is a visual observation, not attribution to a particular renderer pass.

## Reopened existing evidence

Both archives were CRC-checked and their manifests/images inspected:
- Character-Surface-20261106-173040-2368-0.zip (R362): 12 images, complete,
  failures=0, active RR. Contact filtering was already present in baseline
  (301/301 temporal/spatial calls); contact-only also preserves it (441/441).
  Therefore contact-only was not an independent policy change. GI removal and
  steady GI did execute their intended replacements. Jaw grain looks reduced in
  stabilized images, but poses/illumination differ and JPEGs cannot quantify it.
- Character-Surface-20261106-160616-83136-0.zip (R360): 12 images, complete,
  failures=0, all groups bound. Diffuse reconstruction executed; grain remains
  visible in the full-lighting repeat. Do not repeat this as a new isolation test.

## Concrete source finding

The R19 base-albedo correction accepted only native skin-basic SHA
0cd8a4b8144f1add1eac28edb815477ad7e3b42291735aa361e2f8e77b20bb96.
Three later captured skin shaders use the same diffuse reflectance expression:
skin-live, skin-live-noise-a, skin-live-noise-b. All four multiply the base
texture at materialIndex*35 by Part3 bytes52..67 immediately before multiplying
the diffuse light. None applies the selected albedo shader's extra detail term.
The genuine skin-detail family does apply that term and must remain excluded.

`test-rr-character-base-evidence.py` decodes the actual captured shader bytes,
checks the binding identifiers, native tint load, texture-index origin and final
material multiplication, then compiles/executes the production size+SHA matcher.
It checks all12 recorded families, hash mutations, mismatched sizes and null hash.
The existing selected-albedo FNV/SHA regression remains unchanged and passes.

## Change and limits

Extend only the native-side size+SHA allowlist to those four proven families.
The selected replay shader still needs the original exact size/FNV/SHA checks,
and existing vertex/linkage validation still runs before replacement. Unknown
pairs retain their original path. New once-per-record logs include native size
and SHA so activation can be distinguished from merely having the source fix.

Native rendering, hair guides, GI, RR responsivity, filtering and sample counts
are unchanged. RELAX stays off by default. No pass or history is added.
This closes a demonstrable coverage gap if those native shaders are paired with
the audited selected albedo shader. It does not prove that those pairs occur in
the latest video, or that this fixes jaw/hair noise. Hair may need separate work.
No new capture is requested. A build is not a live-game quality result.

## Delivered validation

Runtime commit `c4ee7b48c77183f5352d1eb217fc4f48d8568fc6`; Windows run 42906420410, job 130145457304: all 60 steps successful. Logs confirm all four captured native reflectance audits, production allowlist rejection cases, original selected-albedo hash check, four NRD tests and 132-file baseline check passed. Downloaded archive CRC, published SHA, x64 DLL, R417 labels and new native shader hash strings verified. Live-game result pending.

ZIP SHA-256: `4ca53e06a83382b35065bfe6bdc39ebabd74a732120a5ca1117f51316b7509be`
DLL SHA-256: `28280b2a7c64b427e73a92e2a7254d9a13ac0a1bf52e759f02b91707b68cf01c`
Package: 6797007 bytes, 83 entries.

