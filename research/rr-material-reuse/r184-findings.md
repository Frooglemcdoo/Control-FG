# R184 capture feedback

User reports a small improvement inside the elevator and no improvement on the
painting. Treat the secondary-hit view correction as a partial result, not a
solution to low-sample stability. No performance result was supplied.

The uploaded manifest (7) and all nine parts passed archive and chunk SHA-256
checks and reconstructed 67 complete files. Runtime PID 42760 confirms
`SPEC_HIT_VIEW build=R184 applied=1 native_fallback=0`. Both capture requests
specify SPEC4. Painting frames 50093/50094 and elevator frames 56810/56811
contain 14 resources each, successful RR, reset=0, 2560x1440 input and
3840x2160 output, with pre-exposure 5.55555534.

Inspection of the captured buffers shows dense grain on the painting in
lighting-specular and input-color before RR; reconstructed output retains
coarser unevenness. The elevator also retains grain in its pre-RR specular
lighting. R184 therefore has not removed the noisy input that RR receives.
This does not establish the precise sampling or shading stage responsible.

Camera placement differs between runs. Do not infer a numerical improvement
from image subtraction or compare absolute brightness as a controlled result.
The elevator improvement is the user's in-game assessment.

The R183 component captures already prioritize non-emissive reflected shading
for the painting. They do not distinguish secondary diffuse, local specular,
and environment terms. The next investigation should trace their sampling and
weighting before choosing a new low-sample intervention. Repeating SPEC8/16,
increasing history weight, or broad spatial blur does not answer this result.

This record changes no rendering code and produces no new build.
