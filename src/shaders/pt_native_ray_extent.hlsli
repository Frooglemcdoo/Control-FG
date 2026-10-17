#ifndef CONTROL_PT_NATIVE_RAY_EXTENT
#define CONTROL_PT_NATIVE_RAY_EXTENT
// Archived native reflection-000.ll: %134=1-GBuffer1.b; %348/%349 and
// %334..%336 yield 179.2 / exp2(6*sqrt(roughness)) * (1.5 + noise).
// The PT extension uses this bounded coverage policy at each continuation;
// it does not claim native random-number or multi-bounce equivalence.
float PTNativeRayExtent(float roughness,float noise){
    return 179.2*exp2(-6*sqrt(saturate(roughness)))*(1.5+noise);
}
#endif
