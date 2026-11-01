#include "../src/gi_reuse/true_geometry.h"
struct Inputs { GIPoint origin; float rayT; GIPoint direction; float pad; GIPoint a; float pad1; GIPoint b; float pad2; GIPoint c; float pad3; };
StructuredBuffer<Inputs> input : register(t0);
RWStructuredBuffer<GITrueHit> output : register(u0);
[numthreads(1,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    uint n,stride,m,outStride;input.GetDimensions(n,stride);output.GetDimensions(m,outStride);
    if(id.x>=n||id.x>=m)return;
    Inputs v=input[id.x];output[id.x]=GIRecoverTrueHit(v.origin,v.direction,v.rayT,v.a,v.b,v.c);
}
