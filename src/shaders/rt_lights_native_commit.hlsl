#include "../rt_lights/layout.h"
// Both native passes have already run normally. Private base textures contain
// their non-spotlight replay. Commit only complete independent RT lighting;
// invalid receivers leave the game's original diffuse AND specular unchanged.
StructuredBuffer<Lighting> lighting : register(t0);
Texture2D<float4> baseDiffuse : register(t1);
Texture2D<float4> baseSpecular : register(t2);
RWTexture2D<float4> outputDiffuse : register(u0);
RWTexture2D<float4> outputSpecular : register(u1);
cbuffer Batch : register(b0) {
    uint firstReceiver; uint receiverCount; uint width; uint height;
}
[numthreads(64,1,1)]
void main(uint3 dispatch : SV_DispatchThreadID) {
    if(any(dispatch.yz) || dispatch.x>=receiverCount || width==0 || height==0 ||
       height>0xffffffff/width || firstReceiver>0xffffffff-dispatch.x) return;
    uint receiver=firstReceiver+dispatch.x;
    if(receiver>=width*height) return;
    uint count,stride,dw,dh,sw,sh,ow,oh,pw,ph;
    lighting.GetDimensions(count,stride);
    baseDiffuse.GetDimensions(dw,dh);baseSpecular.GetDimensions(sw,sh);
    outputDiffuse.GetDimensions(ow,oh);outputSpecular.GetDimensions(pw,ph);
    if(dispatch.x>=count || stride!=32 || dw!=width || sw!=width || ow!=width || pw!=width ||
       dh!=height || sh!=height || oh!=height || ph!=height) return;
    Lighting value=lighting[dispatch.x];
    if(value.valid!=1 || !all(isfinite(value.diffuse)) || !all(isfinite(value.specular))) return;
    uint2 pixel=uint2(receiver%width,receiver/width);
    float4 diffuse=baseDiffuse.Load(int3(pixel,0)),specular=baseSpecular.Load(int3(pixel,0));
    diffuse.xyz+=value.diffuse;specular.xyz+=value.specular;
    // Validate the pair before either store. Half-float game targets must not
    // overflow when valid finite float32 inputs are added together.
    if(!all(isfinite(diffuse)) || !all(isfinite(specular)) ||
       any(abs(diffuse)>65504) || any(abs(specular)>65504)) return;
    outputDiffuse[pixel]=diffuse;outputSpecular[pixel]=specular;
}
