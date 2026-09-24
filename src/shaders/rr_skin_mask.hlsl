Texture2D<float4> CharacterCoverage : register(t0);
RWTexture2D<float> ResponsivityMask : register(u0);

[numthreads(8,8,1)]
void GenerateSkinResponsivity(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    CharacterCoverage.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;
    float a = CharacterCoverage.Load(int3(id.xy,0)).a;
    ResponsivityMask[id.xy] = (isfinite(a) && a > 0.0005f) ? 1.0f : 0.0f;
}
