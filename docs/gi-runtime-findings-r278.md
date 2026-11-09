# R278 in-game accounting result

Verified the package manifest, ZIP CRCs and all four reconstructed file hashes.
Trace SHA256: `f8ff6d4066412218d374211b3ddec65844e84bdf197f1e90f2fb07b6677f08ab`.

Process 48968 accepted F7 at present 9796 with right mouse held. The countdown
armed the probe at present 10281. One GI6 dispatch recorded generation 1332,
groups 320x90x1. Submission succeeded; export at present 10311 confirmed GPU
completion, 25,200 retained records, zero overflow and runtime accounting enabled.
Native root/PSO restoration was logged. Nearby logs show RR selected and six GI
rays requested/effective. Reflection clamp bypass was requested at present 7693.

The complete trace contains 3,600 sampled pixels and 21,600 rays. Every pixel has
the new accounting marker and passes comparisons of its surface and miss-fallback
terms against ray contributions, and their sum against native final GI.

| Check | Maximum absolute error |
| --- | ---: |
| Runtime surface/fallback comparison, including their sum versus native | 1.769513e-8 |
| Recorded ray contributions versus native accumulation | 7.363269e-8 |
| Native normalization versus final output | 9.934107e-9 |
| Depth scale | 0 |

These differences are consistent with float32 rounding. The integration passes
this sampled dispatch; no repeat is needed for this check. It does not prove
correct cross-pixel sampling weights, connection visibility, temporal convergence,
or a speckle improvement. Native image output was intentionally unchanged.

12,141 rays are miss/invalid records. Miss fallback contributes about 85.21% of
this sampled grid's luminance sum and must remain in the estimator. Twenty pixels
have one ray contributing over 90% of their GI luminance. These are single-frame
sample statistics, not full-image energy or identification of plant/hair pixels.
Differences from R277 are not a quality comparison between identical views.

The boundary report says no RR capture pairs because this was a diagnostics-only
session; that does not invalidate the separately confirmed GI binary export.
The trace establishes upstream GI accounting, not the final RR guide/filter state.

Next: preserve this verified split while establishing destination evaluation,
proposal weighting and fresh connection visibility for any sample reuse. Do not
replace that work with neighboring-color averaging or brightness suppression.
