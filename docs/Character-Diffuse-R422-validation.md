# R422 character diffuse reconstruction

**REJECTED after gameplay testing:** user reports substantially worse artifacts. See R422-Rejected-Character-Regression.md. Build/test success below is historical and does not establish visual quality.

R421's two moving captures show strong diffuse-light fluctuations on the affected character surfaces. This change removes the current filter's radiance-based neighbor rejection while retaining its surface-guide checks and native remapping. It does not change the GI producer, sample count, RR guides, albedo, specular shading, or temporal history.

## Reason for the intervention

The old gate measures the difference between a noisy center and a noisy neighbor. Bright isolated centers therefore reject ordinary neighbors and survive unchanged. The existing emitted-instruction reproduction demonstrates this exactly: center 1 with neighbors 0.1 stays at 1 under R421. R422 returns 0.325, the existing 4:12 center/neighborhood weighted average. Constant signals remain constant.

No samples are discarded or hard-clipped by this change. Local normalized averaging does not guarantee global conservation at boundaries or with asymmetric native remaps. On flat periodic surfaces with identical guides, emitted-code tests preserve integrated energy for peaks 1, 30 and 173. This scoped result must not be presented as universal energy conservation.

## Capture-based approximation

`research/motion-r421/filter_trial_r422.py` reads the two existing captures using `CONTROL_R421_CAPTURE_ROOT`. It applies the surface weights at identity source coordinates, using captured normals and material/mask bits. The native packed reconstruction map was not captured, so this is an approximate input-buffer experiment, not an exact replay of the character pass and not an RR output prediction.

| ROI/frame | Mean lighting ratio | Local residual RMS ratio | Maximum luminance ratio |
|---|---:|---:|---:|
| Hair 19346 | 1.000612 | 0.562917 | 0.519461 |
| Hair 19347 | 1.000774 | 0.563856 | 0.490227 |
| Jaw 8685 | 0.996508 | 0.317418 | 0.705973 |
| Jaw 8686 | 0.997476 | 0.319453 | 0.609812 |

Residual RMS is relative to a 5x5 local median and can include real lighting variation. Ratios do not quantify pure stochastic noise or guarantee image quality. These results motivated testing the actual shader change rather than another global denoiser adjustment.

## Tests

- 384 emitted-instruction reconstruction cases across 12 character families pass: native remap, borders, constants, material/normal/mask checks, finite-value handling, MSAA fallback and original-register preservation.
- 36 selected-coverage cases pass across 12 reversible transforms with unchanged resource bindings.
- Emitted impulse/periodic-energy fixtures pass; the existing missing-destination-guide limitation remains explicitly tested and unresolved.
- Motion-capture contract and 132 clean-baseline checks pass.
- Filter instruction count drops from 562 to 498. Texture reads, pass count and resource count do not increase. This is not a measured FPS gain.
- Windows build/GPU results are recorded separately in the verified-package document once complete.

## Scope and risks

The existing character diffuse filter has a small footprint, but averaging across a legitimate lighting discontinuity can soften it. Material/normal/mask checks protect some boundaries; they are not depth or object-identity checks. The coverage guard preserves native behavior on unsupported selections. No proof that R421's native reconstruction map selects wrong surfaces has been established.

Keep RELAX OFF and native GI rays. Evaluate moving jaw/hair artifacts, skin brightness, strand detail and character lighting edges. Projector/desk/plant shader code is unchanged, but in-game appearance remains to be checked. R422 is a candidate correction, not a verified complete visual fix.

## Rejected prototype

A temporary 2x2 spatial ray-placement prototype reduced an idealized rare-angular-hit block error by only about 4% on average, with regressions for some placements. It was not integrated. Production sampling and weights remain unchanged.

NVIDIA notes that screen-space temporal sampling benefits can be lost under motion; this was one reason not to substitute an untested temporal-only sequence for this moving-character issue:
https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
