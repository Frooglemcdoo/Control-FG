// OGI1 schema; diagnostic-only root UAV u0/space2 and eight constants b0/space2.
// Header: attempted record count, 0x4f474931, stride=256, frame token.
RWByteAddressBuffer ogiTrace : register(u0,space2);
cbuffer OGIControl : register(b0,space2) {
    uint4 ogiROI; // x,y,width,height in GI dispatch pixels
    uint ogiCapacity,ogiFrame,ogiEnabled,ogiReserved;
};
uint OGIReserve(uint2 pixel,uint count) {
    if(!ogiEnabled || any(pixel<ogiROI.xy) || any(pixel-ogiROI.xy>=ogiROI.zw)) return 0xffffffffu;
    uint index;ogiTrace.InterlockedAdd(0,count,index);
    if(count>ogiCapacity || index>ogiCapacity-count) return 0xffffffffu;
    return 16+index*256;
}
void OGIZero(uint a) {
    [unroll] for(uint offset=0;offset<256;offset+=16) ogiTrace.Store4(a+offset,0);
}
void OGIInvalid(uint2 pixel) {
    uint a=OGIReserve(pixel,1);if(a==0xffffffffu)return;
    OGIZero(a);ogiTrace.Store4(a,uint4(pixel,0,3));
    ogiTrace.Store(a+28,asuint(65504.0f));ogiTrace.Store(a+204,ogiFrame);
}
void OGIRay(uint a,uint2 pixel,uint ray,uint rays,GIShadingRegisters s,GISecondaryInput hit,
            float3 throughput,float depth,float3 origin,float3 normal,float3 running) {
    OGIZero(a);
    float3 secondary=asfloat(s.r15.xyz);
    ogiTrace.Store4(a,uint4(pixel,ray,1));
    ogiTrace.Store4(a+16,asuint(float4(secondary,asfloat(s.r5.w))));
    ogiTrace.Store4(a+32,asuint(float4(throughput,depth)));
    ogiTrace.Store4(a+48,asuint(float4(throughput*secondary,s.ogiDistance)));
    ogiTrace.Store4(a+64,uint4(asuint(s.ogiVolume),s.ogiMask));
    ogiTrace.Store4(a+80,uint4(asuint(s.ogiPoint),0));
    ogiTrace.Store4(a+96,uint4(asuint(s.ogiSpot),0));
    ogiTrace.Store4(a+112,uint4(asuint(secondary-(s.ogiVolume+s.ogiPoint+s.ogiSpot)),0));
    ogiTrace.Store4(a+128,uint4(asuint(s.ogiWorld),hit.material));
    ogiTrace.Store4(a+144,uint4(asuint(s.ogiNormal),rays));
    ogiTrace.Store4(a+160,hit.positionUV);
    ogiTrace.Store4(a+176,hit.normalUV);
    ogiTrace.Store4(a+192,uint4(asuint(origin),ogiFrame));
    ogiTrace.Store4(a+208,uint4(asuint(normal),0));
    ogiTrace.Store4(a+224,uint4(asuint(running),0));
}
void OGIFinal(uint a,uint2 pixel,uint rays,float3 output,float hitInfo,float3 throughput,float depth,
              float3 volume,float3 point,float3 spot,float3 miss,float scale,float3 sum,
              float3 origin,float3 normal) {
    OGIZero(a);
    ogiTrace.Store4(a,uint4(pixel,rays,2));
    ogiTrace.Store4(a+16,asuint(float4(output,hitInfo)));
    ogiTrace.Store4(a+32,asuint(float4(throughput,depth)));
    ogiTrace.Store4(a+48,asuint(float4(miss,scale)));
    ogiTrace.Store4(a+64,uint4(asuint(volume),0));
    ogiTrace.Store4(a+80,uint4(asuint(point),0));
    ogiTrace.Store4(a+96,uint4(asuint(spot),0));
    ogiTrace.Store4(a+112,uint4(asuint(output-(volume+point+spot)),0));
    ogiTrace.Store4(a+192,uint4(asuint(origin),ogiFrame));
    ogiTrace.Store4(a+208,uint4(asuint(normal),0));
    ogiTrace.Store4(a+224,uint4(asuint(sum),0));
}
