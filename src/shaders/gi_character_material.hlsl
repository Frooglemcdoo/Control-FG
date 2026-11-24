#include "gi_character_surface.hlsli"
Texture2D<float4> Mask : register(t0);
Texture2D<float> HardwareDepth : register(t1);
Texture2D<float4> Both : register(t2);
RWTexture2D<float4> NativeGI : register(u0);

#ifdef MEASURE_MATERIAL
RWByteAddressBuffer Counts : register(u1);
#endif
// All four native lighting remap destinations lie in the same 2x2 tile.
// A one-pixel apron also excludes the immediate bilinear footprint at an edge.
bool Owned(int2 p) {
    float4 m=Mask.Load(int3(p,0));
    if(m.a<1.0f)return false;
    uint recorded=UnpackSurfaceWord(m.r)|(UnpackSurfaceWord(m.g)<<15);
    float depth=HardwareDepth.Load(int3(p,0));
    return depth>0 && depth<1 && recorded==asuint(depth);
}
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID) {
    uint w,h;NativeGI.GetDimensions(w,h);
    int2 tile=int2(tid.xy)*2;
    if(any(tile<1)||any(tile+2>=int2(w,h)))return;
    // Skip empty tiles before evaluating the footprint.
    if(!Owned(tile))return;
    [unroll] for(int y=-1;y<=2;++y)
        [unroll] for(int x=-1;x<=2;++x)
            if((x||y)&&!Owned(tile+int2(x,y)))return;
#ifdef MEASURE_MATERIAL
    uint written=0,changed=0;
#endif
    [unroll] for(int j=0;j<2;++j)
        [unroll] for(int i=0;i<2;++i){
            int2 p=tile+int2(i,j);
            float4 both=Both.Load(int3(p,0));
            if(all(isfinite(both))&&all(both.rgb>=0)){
                // Keep all four lanes from the same native spatial result.
#ifdef MEASURE_MATERIAL
                float4 raw=NativeGI[p];
#endif
                NativeGI[p]=both;
#ifdef MEASURE_MATERIAL
                ++written;if(any(raw.rgb!=both.rgb))++changed;
#endif
            }
        }
#ifdef MEASURE_MATERIAL
    Counts.InterlockedAdd(0,written);Counts.InterlockedAdd(4,changed);
#endif
}
