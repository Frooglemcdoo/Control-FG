#ifndef CONTROL_PT_SURFACE_COMMIT
#define CONTROL_PT_SURFACE_COMMIT
// Material-weighted radiance belongs AFTER diffuse*albedo+specular, BEFORE
// primary emission/fog/media. Alpha is coverage, never opacity or luminance.
// u7 is the existing private descriptor slot 48; never alias native u0/u1/u2.
RWByteAddressBuffer PTSurfaceRadiance : register(u7);
bool PTSurfaceExtent(uint2 pixel,uint2 extent){
    uint bytes;PTSurfaceRadiance.GetDimensions(bytes);
    return all(extent>0)&&all(extent<=16384)&&extent.x*extent.y<=16777216&&bytes==extent.x*extent.y*16&&all(pixel<extent);
}
bool PTClearSurface(uint2 pixel,uint2 extent){
    if(!PTSurfaceExtent(pixel,extent))return false;
    PTSurfaceRadiance.Store4((pixel.y*extent.x+pixel.x)*16,uint4(0,0,0,0));return true;
}
bool PTCommitSurface(uint2 pixel,uint2 extent,float3 radiance,uint admission){
    if(admission!=1||!all(isfinite(radiance))||any(radiance<0)||!PTSurfaceExtent(pixel,extent))return false;
    // Zero is a valid fully shadowed PT result, and MUST select replacement.
    // No native UAV reads: direct lighting cannot be counted a second time.
    PTSurfaceRadiance.Store4((pixel.y*extent.x+pixel.x)*16,asuint(float4(radiance,1)));return true;
}
#endif
