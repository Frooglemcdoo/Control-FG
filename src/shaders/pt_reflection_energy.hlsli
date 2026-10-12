// Scalar functions shared with the CPU regression test. Native luminance uses
// equal RGB weights; the caller supplies that exact luminance convention.
float PTReflectionNativeScale(float nativeLuminance,float limit) {
    float denominator=max(nativeLuminance,0.00001f);
    return limit>=0.0f?min(1.0f,limit/denominator):1.0f;
}
float PTReflectionReplacementScale(float retainedLuminance,float bounceLuminance,
                                   float nativeScale,float limit) {
    if(limit<0.0f)return nativeScale;
    float remaining=max(limit-retainedLuminance*nativeScale,0.0f);
    float proposed=max(bounceLuminance,0.0f)*nativeScale;
    return proposed>remaining&&proposed>0.0f?nativeScale*(remaining/proposed):nativeScale;
}
