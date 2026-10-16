#ifndef CONTROL_PT_SURFACE_COMMIT
#define CONTROL_PT_SURFACE_COMMIT
// Material-weighted radiance belongs AFTER diffuse*albedo+specular, BEFORE
// primary emission/fog/media. Alpha is coverage, never opacity or luminance.
// u7 is the existing private descriptor slot 48; never alias native u0/u1/u2.
RWByteAddressBuffer PTSurfaceRadiance : register(u7);
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
static const uint PTSurfaceStride=64;
#else
static const uint PTSurfaceStride=16;
#endif
bool PTSurfaceExtent(uint2 pixel,uint2 extent){
    uint bytes;PTSurfaceRadiance.GetDimensions(bytes);
    return all(extent>0)&&all(extent<=16384)&&extent.x*extent.y<=16777216&&bytes==extent.x*extent.y*PTSurfaceStride&&all(pixel<extent);
}
bool PTClearSurface(uint2 pixel,uint2 extent){
    if(!PTSurfaceExtent(pixel,extent))return false;
    uint at=(pixel.y*extent.x+pixel.x)*PTSurfaceStride;
    [unroll] for(uint offset=0;offset<PTSurfaceStride;offset+=16)PTSurfaceRadiance.Store4(at+offset,0);return true;
}
bool PTCommitSurface(uint2 pixel,uint2 extent,float3 radiance,uint admission){
    if(admission!=1||!all(isfinite(radiance))||any(radiance<0)||!PTSurfaceExtent(pixel,extent))return false;
    // Zero is a valid fully shadowed PT result, and MUST select replacement.
    // No native UAV reads: direct lighting cannot be counted a second time.
    PTSurfaceRadiance.Store4((pixel.y*extent.x+pixel.x)*PTSurfaceStride,asuint(float4(radiance,1)));return true;
}
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
bool PTCommitSignals(uint2 pixel,uint2 extent,PTPathResult value,uint admission){
    if(admission!=1||!PTSurfaceExtent(pixel,extent)||!all(isfinite(value.diffuse))||!all(isfinite(value.specular))||
       any(value.diffuse<0)||any(value.specular<0))return false;
    uint at=(pixel.y*extent.x+pixel.x)*64;
    PTSurfaceRadiance.Store4(at,asuint(float4(value.diffuse,1)));
    PTSurfaceRadiance.Store4(at+16,asuint(float4(value.specular,value.hitDistance)));
    PTSurfaceRadiance.Store4(at+32,asuint(float4(value.kd,value.roughness)));
    PTSurfaceRadiance.Store4(at+48,asuint(float4(max(value.f0,.02),0)));
    return true;
}
#endif
#endif
