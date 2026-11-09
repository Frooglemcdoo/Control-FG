// Stable shadow-space PCF for the added glass bounce only. Compare depths
// before interpolation; interpolating depth before comparison aliases edges.
float PTGlassShadowCompare(Texture2D<float4> atlas, float2 uv, float depth,
                           float2 size, int2 lo, int2 hi)
{
    float2 pixel=uv*size-0.5f;
    int2 p=int2(floor(pixel));
    float2 f=frac(pixel);
    float a=atlas.Load(int3(clamp(p,lo,hi),0)).x>=depth ? 1.0f:0.0f;
    float b=atlas.Load(int3(clamp(p+int2(1,0),lo,hi),0)).x>=depth ? 1.0f:0.0f;
    float c=atlas.Load(int3(clamp(p+int2(0,1),lo,hi),0)).x>=depth ? 1.0f:0.0f;
    float d=atlas.Load(int3(clamp(p+int2(1,1),lo,hi),0)).x>=depth ? 1.0f:0.0f;
    return lerp(lerp(a,b,f.x),lerp(c,d,f.x),f.y);
}

float PTGlassStableShadow(Texture2D<float4> atlas, float2 uv, float depth,
                          float radius, float4 tile)
{
    uint width,height;
    atlas.GetDimensions(width,height);
    float2 size=float2(width,height);
    int2 lo=clamp(int2(ceil(tile.xy*size-0.5f)),int2(0,0),int2(width,height)-1);
    int2 hi=clamp(int2(ceil((tile.xy+tile.zw)*size-0.5f))-1,lo,int2(width,height)-1);
    // Two equal-area rings, symmetric about the receiver. No frame/pixel seed.
    static const float2 disk[16]={
        float2(.5,0),float2(.35355339,.35355339),float2(0,.5),float2(-.35355339,.35355339),
        float2(-.5,0),float2(-.35355339,-.35355339),float2(0,-.5),float2(.35355339,-.35355339),
        float2(.80010315,.33141357),float2(.33141357,.80010315),float2(-.33141357,.80010315),float2(-.80010315,.33141357),
        float2(-.80010315,-.33141357),float2(-.33141357,-.80010315),float2(.33141357,-.80010315),float2(.80010315,-.33141357)
    };
    float visibility=0;
    [unroll] for(uint i=0;i<16;++i)
        visibility+=PTGlassShadowCompare(atlas,uv+disk[i]*radius*tile.zw,depth,size,lo,hi);
    return visibility*(1.0f/16.0f);
}
