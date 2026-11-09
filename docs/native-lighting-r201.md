# R201: raw and filtered spotlight capture

R200's map guard helps with leaks, but cannot recover light blocked by RT.
The supplied R200 capture has 511 samples where native map visibility exceeds
filtered RT by more than 0.25. Forty-eight of those reference rays miss. A
reference ray alone cannot distinguish gameplay ray occlusion from filtering.

## Capture

1. Keep RR ON, SPEC Native, hit distance OFF and the accepted RT settings,
   including Ray Traced Contact Shadows. Native Blend selects MAP GUARD ON.
2. Stand at the outside-window view where native lighting is missing.
3. F10 -> TRACE SHADOW. Keep the same view through settling and completion.
4. Send the Shadow-R201 folder's lighting.log, manifest and three PNGs.

One capture is enough for this measurement. No camera trip inside, OFF retest,
SPEC change, raw-visibility display, or diagnostic colors are required. The
capture preserves the chosen map-guard state and RR. F9 cancels it.

## What changed

Only capture/analysis changes; gameplay shaders and rendering policies remain
R200. After the actual gameplay rays finish, a small observer reads both native
outputs at the same 4096 grid pixels: spot/point visibility and their blocker
distances. It does not trace replacement rays or write native textures. The
private record includes pixel, frame tag and selected light from the existing
input extractor. Filtered visibility and native maps are captured as before.

The hash-verified ray library binds hit distances to u0 and visibility to u1.
Both are two-channel textures. The observer checks both texture extents and
identity; invalid observations are rejected, not interpreted as dark pixels.
The existing ray command list, submission ownership and fence cover this copy.
There is one extra 64x64 compute dispatch only during a requested Native Blend
trace, with no CPU/GPU completion wait. The private input and readback buffers
increase by 128 KiB each. Typed UAV load support is required; pipeline failure
makes capture unavailable rather than changing the gameplay path.

Raw visibility is the aggregate of the gameplay rays, not an individual-ray
identity trace. Raw lit / filtered dark points toward filtering or neighboring
samples. Raw already dark places the disagreement upstream of filtering, but
neither result alone proves which native map/RT result is physically correct.
The native map also includes authored fade/filtering. No minimum brightness,
mesh exclusions or arbitrary shadow-bias adjustment is added.

`tools/analyze-missing-light.py` reports raw-versus-filtered categories when
present, and still accepts older captures without inventing raw evidence.
The strict decoder rejects missing/duplicate records, pixel/light/frame
mismatch, invalid visibility and nonfinite/negative distances.

## Validation

The emitted observer is checked for two native texture reads, no texture stores
or rays, and private buffer writes only. WARP executes the shipping observer
against patterned two-channel textures, checks all 4096 values and unchanged
input identities, and checks extent-mismatch rejection. Decoder tests exercise
malformed/stale records; existing submission, repeat-capture and queue-retry
checks remain. In-game capture remains to be verified on the user's system.

Experimental branch only. R200's accepted reflection fixes, free camera,
Native Blend guard and R199 comparison are retained.
