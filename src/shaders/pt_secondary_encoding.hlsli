// Secondary layer only. Octahedral normals free one half-float channel for U.
// normal = octX, octY, U, material; position = viewX, viewY, viewZ, V.
float2 PTSecondarySign(float2 p) { return float2(p.x>=0?1:-1,p.y>=0?1:-1); }
float2 PTSecondaryEncodeNormal(float3 n) {
    n/=max(abs(n.x)+abs(n.y)+abs(n.z),1.0e-12f);
    return n.z>=0?n.xy:(1.0f-abs(n.yx))*PTSecondarySign(n.xy);
}
float3 PTSecondaryDecodeNormal(float2 p) {
    float3 n=float3(p,1.0f-abs(p.x)-abs(p.y));
    if(n.z<0)n.xy=(1.0f-abs(n.yx))*PTSecondarySign(n.xy);
    return normalize(n);
}
