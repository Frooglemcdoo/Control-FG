// R434 live fixed-scene guide, paired with tools/gi_guided_live.py.
#include "gi_guided_target.hlsl"
#include "gi_frozen_receiver.hlsl"
static const GTBound giGuide0 = {float3(-82.0759007f,25.003847f,110.55301f),0.160592319f};
static const GTBound giGuide1 = {float3(-82.0279381f,25.345521f,110.818513f),0.136421514f};
float GILiveGuideWeight(uint2 pixel) {
    GIFrozenReceiver receiver=GIFrozenDecodeReceiver(pixel,g_tGBuffer1.Load(int3(pixel,0)),
        g_tLinearDepth.Load(int3(pixel,0)).x,g_tClipDepth.Load(int3(pixel,0)).x,giCB0);
    if(!receiver.valid)return 1;
    GIProducerCoordinates random=GIProducerSample(pixel.x,pixel.y,giCB0[69].z,0,0,1);
    return GTSample(receiver.origin,receiver.normal,giGuide0,giGuide1,.1,true,random.u,random.v).radianceWeight;
}
