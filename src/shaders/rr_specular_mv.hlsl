// GI27: specular motion vectors for DLSS-RR, ported from the Control RenoDX reference.
// Per-pixel motion of the reflected image, not the surface the reflection appears on.
cbuffer Params : register(b0)
{
    row_major float4x4 ClipToView;
    row_major float4x4 ViewToClip;
    row_major float4x4 ClipToPrevClip;
    uint Width;
    uint Height;
    float MvOutScaleX;
    float MvOutScaleY;
    uint Layers;
};

Texture2D<float4> GBuffer1 : register(t0);
Texture2DArray<uint> MatId : register(t1);
Texture2D<float> Depth : register(t2);
Texture2D<float2> GameMV : register(t3);
Texture2DArray<float4> HitPos : register(t4);

RWTexture2D<float2> SpecMV : register(u0);
RWTexture2D<float4> Debug : register(u1);

#define MATID_NO_RAY 0xFFFE
#define MATID_SKY    0xFFFF
#define NDC_CORRECTION_MAX 1.0

float3 ViewPos(int2 p)
{
    float z = Depth.Load(int3(p, 0));
    float2 uv = (float2(p) + 0.5) / float2(Width, Height);
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 v = mul(float4(ndc, z, 1.0), ClipToView);
    return v.xyz / v.w;
}

float MinHitDistance(int2 p, float3 surface_pos)
{
    float best = 3.402823466e38;
    bool any_hit = false;
    for (uint s = 0; s < Layers; ++s)
    {
        uint id = MatId.Load(int4(p, s, 0));
        if (id == MATID_NO_RAY)
            break;
        if (id == MATID_SKY)
            continue;
        float3 hit = HitPos.Load(int4(p, s, 0)).xyz;
        float d = length(hit - surface_pos);
        if (d > 0.0)
        {
            best = min(best, d);
            any_hit = true;
        }
    }
    return any_hit ? best : 0.0;
}

float2 NdcDelta(float3 view_pos)
{
    float4 clip_now = mul(float4(view_pos, 1.0), ViewToClip);
    float2 ndc_now = clip_now.xy / clip_now.w;
    float4 clip_prev = mul(clip_now, ClipToPrevClip);
    float2 ndc_prev = clip_prev.xy / clip_prev.w;
    return ndc_prev - ndc_now;
}

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= Width || tid.y >= Height)
        return;

    float2 game_mv = GameMV.Load(int3(tid.xy, 0));
    float gloss = GBuffer1.Load(int3(tid.xy, 0)).z;

    float3 surface_pos = ViewPos(int2(tid.xy));
    float hit_t = MinHitDistance(int2(tid.xy), surface_pos);

    float effective_t = hit_t * gloss * gloss;

    if (!(effective_t > 0.0))
    {
        SpecMV[tid.xy] = game_mv;
        Debug[tid.xy] = float4(hit_t, effective_t, 0.0, 0.0);
        return;
    }

    float3 view_dir = normalize(surface_pos);
    float3 virtual_pos = surface_pos + view_dir * effective_t;

    float2 correction = NdcDelta(virtual_pos) - NdcDelta(surface_pos);

    if (!all(abs(correction) < NDC_CORRECTION_MAX))
    {
        SpecMV[tid.xy] = game_mv;
        Debug[tid.xy] = float4(hit_t, effective_t, 0.0, -1.0);
        return;
    }

    float2 scaled = correction * float2(MvOutScaleX, MvOutScaleY);
    SpecMV[tid.xy] = game_mv + scaled;
    Debug[tid.xy] = float4(hit_t, effective_t, length(scaled), 1.0);
}
