Texture2D<float4> FinalColor : register(t0);
Texture2D<float4> HudlessColor : register(t1);
RWTexture2D<float> UIAlpha : register(u0);

[numthreads(8,8,1)]
void GenerateUIAlpha(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    FinalColor.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;

    float3 finalColor = FinalColor.Load(int3(id.xy, 0)).rgb;
    float3 hudless = HudlessColor.Load(int3(id.xy, 0)).rgb;
    if (!all(isfinite(finalColor)) || !all(isfinite(hudless))) {
        UIAlpha[id.xy] = 0.0f;
        return;
    }

    float3 d = abs(finalColor - hudless);
    float delta = max(d.r, max(d.g, d.b));
    // Control HUD elements are predominantly opaque with antialiased edges.
    // Use the pre/post-UI delta only as an opacity hint: tiny post effects stay
    // at zero while visible HUD edges ramp rapidly toward one.
    float alpha = smoothstep(0.0025f, 0.035f, delta);
    UIAlpha[id.xy] = saturate(alpha);
}
