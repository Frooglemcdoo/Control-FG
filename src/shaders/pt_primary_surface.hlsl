// Development producer: complete standard opaque surface radiance.
// Requires a render-resolution u7 output and the reviewed pre-fog compositor.
// Do not load this as pt_primary_path.dxil: the legacy host allocates 1x1.
#define PT_PRIMARY_SURFACE_REPLACEMENT 1
#include "pt_primary_path.hlsl"
