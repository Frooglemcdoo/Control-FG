#include "../rt_lights/layout.h"
// Both native passes have already run normally. Private base textures contain
// their non-spotlight replay. Commit only complete independent RT lighting;
// invalid receivers leave the game's original diffuse AND specular unchanged.
StructuredBuffer<Lighting> lighting : register(t0);
Texture2D<float4> baseDiffuse : register(t1);
Texture2D<float4> baseSpecular : register(t2);
RWTexture2D<float4> outputDiffuse : register(u0);
RWTexture2D<float4> outputSpecular : register(u1);
#ifdef RT_LIGHTS_RECONSTRUCT
#include "../rt_lights/tile_layout.h"
cbuffer Batch : register(b0) { ShadowTile tile; }
#else
cbuffer Batch : register(b0) {
    uint firstReceiver; uint receiverCount; uint width; uint height;
}
#endif
[numthreads(64,1,1)]
void main(uint3 dispatch : SV_DispatchThreadID) {
#ifdef RT_LIGHTS_RECONSTRUCT
    uint width=tile.frameWidth,height=tile.frameHeight,receiverCount=tile.receiverCount;
    if(tile.width==0) return;
    uint2 pixel=uint2(tile.originX,tile.originY)+uint2(dispatch.x%tile.width,dispatch.x/tile.width);
    if(pixel.x<tile.coreX || pixel.y<tile.coreY || pixel.x>=tile.coreX+tile.coreWidth || pixel.y>=tile.coreY+tile.coreHeight) return;
    uint firstReceiver=0;
#endif
    if(any(dispatch.yz) || dispatch.x>=receiverCount || width==0 || height==0 ||
       height>0xffffffff/width || firstReceiver>0xffffffff-dispatch.x) return;
#ifdef RT_LIGHTS_RECONSTRUCT
    uint receiver=pixel.y*width+pixel.x;
#else
    uint receiver=firstReceiver+dispatch.x;
#endif
    if(receiver>=width*height) return;
    uint count,stride,dw,dh,sw,sh,ow,oh,pw,ph;
    lighting.GetDimensions(count,stride);
    baseDiffuse.GetDimensions(dw,dh);baseSpecular.GetDimensions(sw,sh);
    outputDiffuse.GetDimensions(ow,oh);outputSpecular.GetDimensions(pw,ph);
    if(dispatch.x>=count || stride!=32 || dw!=width || sw!=width || ow!=width || pw!=width ||
       dh!=height || sh!=height || oh!=height || ph!=height) return;
    Lighting value=lighting[dispatch.x];
    if(value.valid!=1 || !all(isfinite(value.diffuse)) || !all(isfinite(value.specular))) return;
#ifndef RT_LIGHTS_RECONSTRUCT
    uint2 pixel=uint2(receiver%width,receiver/width);
#endif
    float4 diffuse=baseDiffuse.Load(int3(pixel,0)),specular=baseSpecular.Load(int3(pixel,0));
    diffuse.xyz+=value.diffuse;specular.xyz+=value.specular;
    // Validate the pair before either store. Half-float game targets must not
    // overflow when valid finite float32 inputs are added together.
    if(!all(isfinite(diffuse)) || !all(isfinite(specular)) ||
       any(abs(diffuse)>65504) || any(abs(specular)>65504)) return;
    outputDiffuse[pixel]=diffuse;outputSpecular[pixel]=specular;
}
