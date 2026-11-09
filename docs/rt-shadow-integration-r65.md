# R65: capture the missing RT lighting shaders

This is an integration diagnostic, **not a shadow replacement or visual fix**.
Native rendering remains active. The next replacement still needs the verified
per-light composition path; changing only ray distance would be incomplete.

## What the R64 capture established

The dedicated main-view shadow library has SHA256
`d5efe2c2219257e2a7b51bbd65a7fa3b80cdb56fb7e6444223f8b4d7ee45a1a9`.
Its closest-hit handler returns a float distance; its miss handler leaves the
initialized distance alone. Its alpha-test handler uses native material and
geometry bindings. These differ from the reflection library's shadow handlers.

The ray-generation shader selects one spot and one point light per pixel.
Spot ray distance is limited to the smaller of the receiver/light distance
minus the light's near distance, and a randomized 0.5–1.5 engine-unit limit.
Point rays have a similar contact limit. The ray radius also depends on this
limit. Extending it blindly would change softness as well as reach.

However, R64 saved only the known **non-RT** direct-light shader
`bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff`.
That shader has no selected-light RT visibility input. The log recorded hashes
and lengths of 11 compute shaders first bound after RT was enabled, but not
those shaders' bytes. Their individual roles are not yet established. The
previous claim that no further capture would be necessary was premature.

## What changed

R65 captures those 11 exact size/hash pairs at compute-pipeline observation,
including the existing pipeline-library load route. Equal-size shaders remain
distinct. Shader candidates live in one shared catalog; they are not selected
by size alone. Unknown bytecode is neither captured by this catalog nor patched.

The single session log reports independent masks for creation, successful byte
persistence, and observed command-list binding. Expected mask is 2047. Missing
saved bits are explicit. A saved bit is set only after a successful file write
or identification of an already-persisted identical blob. Candidate bindings
are recorded separately from the general 512-pipeline logging limit. Binding
observation does not prove GPU execution.

The capture remains bounded to 2 MiB per blob, 8 MiB total and 128 unique blobs.
No GPU readback is added. New shaders add at most 271,160 raw bytes for this
catalog. The extractor accepts `compute_cs` records and verifies their SHA256.
Root/state/scene/SBT capture remains available. Pipeline-state streams and
AddToStateObject are still not observed; shaders created before hook installation
can also be missing. This is why missing bits remain visible in the log.

## One-session run

1. Close Control to install R65, then launch once in Borderless.
2. Load the affected room with FG, RR and glass enhancement off.
3. Start with RT off, return to gameplay, then enable RT Contact Shadows and
   RT Reflections. Play for 10 seconds and move briefly.
4. Toggle RT off and on again, returning to gameplay after each change.
5. Press F10, choose SAVE LOG and send the single Lighting-R65 log shown.

No restart between settings is required if the game applies them live. Installing
the DLL requires closing the game. There is no visual replacement A/B mode yet.

## Replacement boundary

The planned first replacement targets selected spotlights, retaining native
alpha testing and denoising where compatible. It must replace the matching
light's old visibility before diffuse/specular accumulation, preserve native
light color/intensity/falloff, and leave unselected lights on their native path.
It must also preserve shader-table identifiers and resource lifetimes during
live switching. Merely extending contact rays or removing all shadow maps does
not satisfy this boundary. Full scene/light coverage remains unverified.
