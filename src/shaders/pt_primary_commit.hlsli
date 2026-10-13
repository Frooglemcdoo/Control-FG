#ifndef CONTROL_PT_PRIMARY_COMMIT
#define CONTROL_PT_PRIMARY_COMMIT
// Reviewed standard primary shader: u0 is diffuse irradiance, u1 is
// material-weighted specular radiance. Native surface PS combines
// diffuse * textured albedo + specular. PT already includes its material BSDF,
// so put its complete reflected radiance in the additive slot exactly once.
RWTexture2D<float4> PTNativeDiffuse : register(u0);
RWTexture2D<float4> PTNativeSpecular : register(u1);
bool PTCommitPrimary(uint2 pixel,uint2 extent,float3 radiance,uint admission){
    if(admission!=1||!all(isfinite(radiance))||any(radiance<0))return false;
    uint dw,dh,sw,sh;PTNativeDiffuse.GetDimensions(dw,dh);PTNativeSpecular.GetDimensions(sw,sh);
    if(any(uint2(dw,dh)!=extent)||any(uint2(sw,sh)!=extent)||any(pixel>=extent))return false;
    PTNativeDiffuse[pixel]=0;
    // Native primary stores RGBR, not opacity, in these light buffers.
    PTNativeSpecular[pixel]=float4(radiance,radiance.x);return true;
}
bool PTCommitPrimaryWithFill(uint2 pixel,uint2 extent,float3 radiance,float3 fill,uint admission){
    if(!all(isfinite(fill))||any(fill<0)||!all(isfinite(radiance))||any(radiance<0))return false;
    return PTCommitPrimary(pixel,extent,radiance+fill,admission);
}
#endif
