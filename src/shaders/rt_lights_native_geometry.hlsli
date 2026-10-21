#ifndef RT_LIGHTS_NATIVE_GEOMETRY
#define RT_LIGHTS_NATIVE_GEOMETRY
// Offsets verified against R65 native RDEF and contact-shadow DXIL.
struct NativeSpotLight { float4 words[20]; };
cbuffer NativeSystem : register(b0) { float4 systemWords[71]; }
float4 NativeClipToView(float4 clip) {
    return clip.x*systemWords[14]+clip.y*systemWords[15]+clip.z*systemWords[16]+systemWords[17];
}
float3 NativeViewDirectionToWorld(float3 value) {
    return value.x*systemWords[6].xyz+value.y*systemWords[7].xyz+value.z*systemWords[8].xyz;
}
float3 NativeViewPositionToWorld(float3 value) {
    return NativeViewDirectionToWorld(value)+systemWords[9].xyz;
}
float3 NativeViewNormal(float4 gbuffer) {
    uint3 bytes=uint3(floor(gbuffer.xy*255+0.5), uint(gbuffer.w*255+0.5)&254);
    uint2 packed=uint2((bytes.x<<3)|((bytes.z>>1)&7), (bytes.y<<4)|((bytes.z>>4)&15));
    float2 plane=(float2(packed)*float2(0.00097703957,0.000488400517)-1)*1.3;
    float square=dot(plane,plane);
    return float3(2*plane,square-1)/(square+1);
}
// Same basis orientation as the native raygen; no screen/frame random seed.
void NativeEmitterBasis(float3 direction,out float3 u,out float3 v) {
    if(direction.z < -0.9999999) { u=float3(0,-1,0);v=float3(-1,0,0);return; }
    float a=1/(1+direction.z), b=-direction.x*direction.y*a;
    u=float3(1-direction.x*direction.x*a,b,-direction.x);
    v=float3(b,1-direction.y*direction.y*a,-direction.y);
}
#endif
