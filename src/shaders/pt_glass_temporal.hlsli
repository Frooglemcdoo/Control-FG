// One record per reflected pixel, owned by the same fenced resource slot.
// No cross-pixel history reads: conservative reuse at the same pixel avoids
// treating the front glass depth/motion as reflected-surface motion.
struct PTGlassHistory {
    float4 illumination_count;
    float4 position_material;
    float4 normal_serial;
};
RWStructuredBuffer<PTGlassHistory> ptGlassHistory : register(u1);

void PTGlassAccumulate(uint2 pixel,float3 worldPosition,float3 normal,uint material,
                       float3 fallback,inout float3 illumination) {
    uint index=pixel.y*ptGlassExtent.x+pixel.x;
    float count=1.0f;
    float3 resolved=illumination;
    // Fresh allocations contain undefined bytes. Never read until a consumer
    // has been recorded and its slot has retired through the actual GPU fence.
    if(ptGlassHistorySerial!=0u) {
        PTGlassHistory previous=ptGlassHistory[index];
        float3 delta=previous.position_material.xyz-worldPosition;
        bool compatible=asuint(previous.normal_serial.w)==ptGlassHistorySerial&&
            asuint(previous.position_material.w)==material&&
            all(isfinite(previous.position_material.xyz))&&
            all(isfinite(previous.normal_serial.xyz))&&
            all(isfinite(previous.illumination_count))&&
            previous.illumination_count.w>=1.0f&&previous.illumination_count.w<=16.0f&&
            dot(delta,delta)<=0.0004f&&dot(previous.normal_serial.xyz,normal)>=0.995f;
        if(compatible) {
            count=min(previous.illumination_count.w+1.0f,16.0f);
            // Limit stale lighting after a large lighting change. The broad
            // bound retains ordinary sample variation and permits real light.
            float3 limit=max(max(fallback,resolved)*2.0f,1e-4f);
            float3 old=min(max(previous.illumination_count.xyz,0.0f),limit);
            resolved=lerp(old,resolved,1.0f/count);
        }
    }
    PTGlassHistory current;
    current.illumination_count=float4(resolved,count);
    current.position_material=float4(worldPosition,asfloat(material));
    current.normal_serial=float4(normal,asfloat(ptGlassEpoch));
    ptGlassHistory[index]=current;
    // New/disoccluded surfaces stay on native GI while evidence builds.
    illumination=lerp(fallback,resolved,saturate((count-1.0f)/7.0f));
}
