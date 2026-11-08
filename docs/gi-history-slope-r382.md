# R382: slope-aware GI history validation

R381 was rejected by the user as visually worse. Its fixed relative-depth test
compared each history footprint tap with the receiver centre. Valid taps on a
sloped surface can fail that test as fractional reprojection changes.

The new fixture executes the exact shipped R381 bytecode and the candidate on
Windows D3D12 WARP. A constant sloped surface has current radiance .4 and valid
history .35; the intended temporal output is .365. Run 42431123309 produced:

| Fractional phase | R381 | R382 |
|---|---:|---:|
| .25 | .365 | .365 |
| .50 | .400 | .365 |
| .75 | .365 | .365 |

R382 estimates a limited current linear-depth gradient from four neighbouring
depth samples. Each axis uses the smaller same-sign one-sided slope, or zero
when signs disagree. History-tap residuals account for their UV offset along
this gradient before the existing relative-depth acceptance test. This is an
approximation, not full normal-based plane-distance reprojection or NRD.
Large view rotations and animated geometry remain limitations of this model.

History texel positions now use dimensions queried from the history SRV.
Current-depth gradient offsets similarly use the depth SRV's dimensions.
The native normalized previous-UV calculation is retained. Tests pass with
output constants 1.5x and 2x the history dimensions. An actual in-game mismatch
has not been established; this removes an assumption rather than proving the
cause of the user's scene regression.

Additional passing cases: diagonal slopes; invalid depths whose bilinear
average would match; valid bright-only history; continuous-surface interpolation;
diagonal boundary taps; camera cuts; previous-view depth offset; existing two-
and six-spatial-pass feedback fixtures. R380 and R381 archive hashes match the
previous generated shaders, so the controls are not modified approximations.

The production change adds four depth samples, two dimension queries and three
scratch registers compared with R381. No new passes, buffers, monitoring or
capture work. RAW, GI producer, spatial shaders, clamp defaults and F10 layout
are unchanged. Performance on the user's GPU and the visible desk/plant result
remain unvalidated. A synthetic passing test is not an in-game quality claim.

Candidate stable DXBC SHA-256:
`93254adac6ec54f2a4a1ec707aa0aa7844d853825e435cc5bbb47ce8c269bd1a`
