#ifndef CONTROL_PT_TERMINAL_GI_REQUEST
#define CONTROL_PT_TERMINAL_GI_REQUEST
// Temporary use of reconstruction's two existing 32-byte/pixel buffers.
// Reconstruction overwrites both after the GI compute pass completes.
RWByteAddressBuffer PTGIRequestGeometry : register(u4);
RWByteAddressBuffer PTGIRequestWeight : register(u5);
bool PTGIRequestExtent(uint2 pixel,uint2 extent){
    uint geometryBytes,weightBytes;
    PTGIRequestGeometry.GetDimensions(geometryBytes);PTGIRequestWeight.GetDimensions(weightBytes);
    return all(extent>0)&&all(extent<=16384)&&extent.x*extent.y<=16777216&&all(pixel<extent)&&
           geometryBytes==extent.x*extent.y*32&&weightBytes==geometryBytes;
}
bool PTClearGIRequest(uint2 pixel,uint2 extent){
    if(!PTGIRequestExtent(pixel,extent))return false;
    // Invalid requests never read the remaining bytes. Clear on every admitted
    // pixel before all depth/material rejection paths to prevent stale tails.
    PTGIRequestGeometry.Store((pixel.y*extent.x+pixel.x)*32+12,0);return true;
}
void PTWriteGIRequest(uint2 pixel,uint2 extent,float3 position,float3 normal,float3 weight,float3 fraction,uint kind,float3 fallback){
    uint at=(pixel.y*extent.x+pixel.x)*32;
    PTGIRequestGeometry.Store4(at,asuint(float4(position,(float)kind)));
    PTGIRequestGeometry.Store3(at+16,asuint(normal));
    PTGIRequestWeight.Store3(at,asuint(weight));
    PTGIRequestWeight.Store3(at+16,asuint(fraction));
    // Three formerly unused words carry the environment fallback on cache misses.
    PTGIRequestGeometry.Store(at+28,asuint(fallback.x));
    PTGIRequestWeight.Store(at+12,asuint(fallback.y));
    PTGIRequestWeight.Store(at+28,asuint(fallback.z));
}
#endif
