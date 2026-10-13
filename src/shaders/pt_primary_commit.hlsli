#ifndef CONTROL_PT_PRIMARY_COMMIT
#define CONTROL_PT_PRIMARY_COMMIT
// Native u0/u1 remain the output pair. Preserve u0 for native diffuse material
// and scattering consumers; indirect radiance is additive through u1.
RWTexture2D<float4> PTNativeDiffuse : register(u0);
RWTexture2D<float4> PTNativeSpecular : register(u1);
// R31 retains native direct diffuse/specular/SSS. Only indirect diffuse-first
// transport is added, so direct lights and specular fill are not counted twice.
// Host proves typed-UAV-load support for this exact native descriptor format.
bool PTCommitIndirect(uint2 pixel,uint2 extent,float3 radiance,uint admission){
    if(admission!=1||!all(isfinite(radiance))||any(radiance<0))return false;
    uint dw,dh,sw,sh;PTNativeDiffuse.GetDimensions(dw,dh);PTNativeSpecular.GetDimensions(sw,sh);
    if(any(uint2(dw,dh)!=extent)||any(uint2(sw,sh)!=extent)||any(pixel>=extent))return false;
    float4 native=PTNativeSpecular[pixel];
    float4 combined=native+float4(radiance,radiance.x);
    if(!all(isfinite(native))||any(native<0)||!all(isfinite(combined)))return false;
    PTNativeSpecular[pixel]=combined;return true;
}
#endif
