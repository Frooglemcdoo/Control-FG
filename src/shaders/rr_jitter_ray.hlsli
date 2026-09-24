#ifndef CONTROL_RR_JITTER_RAY
#define CONTROL_RR_JITTER_RAY
// Native projection offsets shift raster coordinates by NGX pixel jitter.
// Invert that shift at the integer texel's center; do not resample guides.
float ControlRayX(float center,float width,float scale,float jitter) {
    return (2.0f*(center-jitter)/width-1.0f)/scale;
}
float ControlRayY(float center,float height,float scale,float jitter) {
    return (1.0f-2.0f*(center-jitter)/height)/scale;
}
#endif
