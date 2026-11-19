#ifndef GI_TEXTURE_BILINEAR
#define GI_TEXTURE_BILINEAR
// Diagnostic only: explicit integer mip, wrap addressing, four texel reads.
// No sampler access. Clamps the requested level to the exposed view's range.
float4 GILoadBilinear(Texture2D<float4> tex,float2 uv,uint mip){
 uint w,h,levels;tex.GetDimensions(0,w,h,levels);
 if(!w||!h||!levels||!all(isfinite(uv)))return asfloat(0x7fc00000u).xxxx;
 mip=min(mip,levels-1);tex.GetDimensions(mip,w,h,levels);
 if(!w||!h)return asfloat(0x7fc00000u).xxxx;
 float2 at=frac(uv)*float2(w,h)-.5;int2 p=int2(floor(at));float2 f=frac(at);
 int2 size=int2(w,h),lo=(p+size)%size,hi=(lo+1)%size;
 float4 a=tex.Load(int3(lo,mip)),b=tex.Load(int3(hi.x,lo.y,mip));
 float4 c=tex.Load(int3(lo.x,hi.y,mip)),d=tex.Load(int3(hi,mip));
 return lerp(lerp(a,b,f.x),lerp(c,d,f.x),f.y);
}
#endif
