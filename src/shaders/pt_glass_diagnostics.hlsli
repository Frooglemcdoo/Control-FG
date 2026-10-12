// Sparse GPU observations; no change to lighting. Three disjoint layers:
// reflected surface, glossy continuation, diffuse continuation.
struct PTGlassDiagnostic {
    uint4 identity;
    float4 levels,filter,history,pointStats,spot,output,reserved;
};
RWStructuredBuffer<PTGlassDiagnostic> ptGlassDiagnostics : register(u2);
float PTGlassLuma(float3 v) { return dot(v,float3(0.2126f,0.7152f,0.0722f)); }
bool PTGlassDiagnosticPixel(uint2 p) {
    return ptGlassCapture!=0u && all(p<ptGlassExtent) && all((p&31u)==0u);
}
uint PTGlassDiagnosticIndex(uint2 p,uint layer) {
    uint2 size=(ptGlassExtent+31u)/32u;
    return layer*size.x*size.y+(p.y/32u)*size.x+p.x/32u;
}
void PTGlassDiagnosticStore(uint2 p,uint layer,PTGlassDiagnostic d) {
    if(PTGlassDiagnosticPixel(p))ptGlassDiagnostics[PTGlassDiagnosticIndex(p,layer)]=d;
}
