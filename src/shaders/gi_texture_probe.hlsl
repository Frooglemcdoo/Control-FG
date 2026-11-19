#ifndef GI_TEXTURE_PROBE
#define GI_TEXTURE_PROBE
struct GITextureProbeRecord {uint4 words[8];};
GITextureProbeRecord GIReadTextureProbe(Texture2D<float4> tex,SamplerState samp,float2 uv,uint material,uint index,uint ray,float weight){
 GITextureProbeRecord r=(GITextureProbeRecord)0;
 r.words[0]=uint4(material,index,ray,0);r.words[2]=asuint(float4(uv,weight,0));
 uint w,h,levels;tex.GetDimensions(0,w,h,levels);r.words[1]=uint4(w,h,levels,0);
 if(w==0||h==0||levels==0)return r;
 r.words[0].w|=1;
 if(!all(isfinite(uv)))return r;
 r.words[0].w|=2;
 r.words[3]=asuint(tex.SampleLevel(samp,uv,0));
 r.words[4]=asuint(tex.SampleLevel(samp,uv,1));
 uint2 p=min(uint2(frac(uv)*float2(w,h)),uint2(w-1,h-1));
 r.words[5]=asuint(tex.Load(int3(p,0)));
 if(levels>1){
  uint w1,h1,n1;tex.GetDimensions(1,w1,h1,n1);r.words[7]=uint4(w1,h1,n1,0);
  if(w1&&h1){r.words[0].w|=4;uint2 p1=min(uint2(frac(uv)*float2(w1,h1)),uint2(w1-1,h1-1));r.words[6]=asuint(tex.Load(int3(p1,1)));}
 }
 return r;
}
#endif
