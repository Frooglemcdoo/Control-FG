// Owned's current-frame evaluator uses the engine's GI6 bindings directly.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"

[numthreads(8,16,1)]
void main(uint3 id : SV_DispatchThreadID) {
    GIShadingRegisters receiver;
    int rays = asint(giCB4[0].z);
    if (rays <= 0 || !GIInitializeReceiver(id.xy, receiver)) {
        g_rwtDiffuseGIColorTarget[id.xy] = 0;
        g_rwtHitInfo[id.xy] = 65504.0f;
        return;
    }

    float3 throughput = asfloat(receiver.r3.xyz);
    float depth = asfloat(receiver.r1.w);
    float3 radiance = 0;
    [loop] for (int slice = 0; slice < rays; ++slice) {
        int4 address = int4(id.xy, slice, 0);
        GISecondaryInput hit;
        hit.material = g_tMaterialId.Load(address).x;
        hit.positionUV = asuint(g_tPosition_TexcoordY.Load(address));
        hit.normalUV = asuint(g_tNormal_TexcoordX.Load(address));
        // Includes the native volume contribution for a miss. This routine
        // also accumulates the distance convention expected by RR in r5.w.
        GIEvaluateSecondary(receiver, hit);
        radiance = mad(throughput, asfloat(receiver.r15.xyz), radiance);
    }

    float4 fade = asfloat(giCB1[17]);
    float nearEdge = fade.x - fade.y;
    float width = (fade.y + fade.x) - nearEdge;
    float t = saturate((depth - nearEdge) * rcp(width));
    float scale = mad(t*t*(3.0f-2.0f*t), fade.w-fade.z, fade.z);
    g_rwtDiffuseGIColorTarget[id.xy] = float4((scale * radiance) / float(rays), 1);
    g_rwtHitInfo[id.xy] = asfloat(receiver.r5.w) / float(rays);
}
