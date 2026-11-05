// Same global root and hit table as the native GI producer. Only the history
// buffer and its immutable capacity are local to this ray-generation record.
Texture2D<float4> gbuffer : register(t0);
Texture2D<float4> linearDepth : register(t1);
Texture2D<float4> clipDepth : register(t2);
#if GI_VARIANT
RaytracingAccelerationStructure scene : register(t5);
#else
RaytracingAccelerationStructure scene : register(t4);
#endif
RWTexture2DArray<uint> materials : register(u0);
RWTexture2DArray<float4> positions : register(u2);
RWByteAddressBuffer history : register(u0,space17);
cbuffer Limits : register(b0,space17) { uint capacity;  }
cbuffer System : register(b0) { float4 sys[71]; }
cbuffer GI : register(b1) { uint4 controls; uint passCount; }
struct Payload { uint slice; };

float4 transform(float4 v,uint i) {
    return v.x*sys[i]+v.y*sys[i+1]+v.z*sys[i+2]+v.w*sys[i+3];
}
float3 receiverNormal(float4 g) {
    uint4 bits=(uint4)floor(g*255+0.5f);
    uint2 uv=(bits.xy<<uint2(3,4))|uint2((bits.w>>1)&7,bits.w>>4);
    float2 n=float2(uv)*(2.6f/float2(2047,4095))-1.3f;
    float d=dot(n,n);
    return normalize(transform(float4(float3(2*n,d-1)/(d+1),0),6).xyz);
}
uint normalKey(float3 n) {
    return (uint(round(saturate(n.x*.5+.5)*1023))) |
        (uint(round(saturate(n.y*.5+.5)*1023))<<10) |
        (uint(round(saturate(n.z*.5+.5)*1023))<<20);
}
float3 unpackNormal(uint v) {
    return float3(v&1023,(v>>10)&1023,(v>>20)&1023)*(2.0f/1023)-1;
}
[shader("raygeneration")]
void TemporalGI() {
    uint2 p=DispatchRaysIndex().xy, extent=DispatchRaysDimensions().xy;
    uint rays=controls.z, frame=asuint(sys[69].z);
    if(!rays || rays>64 ||
       controls.w!=0 || passCount!=1 || !frame)return;
    uint step=2;
    while(((extent.x+step-1)/step)*((extent.y+step-1)/step)>capacity)step*=2;
    uint2 grid=(extent+step-1)/step;
    uint cell=(p.y/step)*grid.x+p.x/step;
    if(cell>=capacity)return;
    uint writeAddr=((frame&1)*capacity+cell)*40;
    // Each cell has exactly one writer. Rotate its representative over time.
    uint2 representative=min((p/step)*step+uint2(frame%step,(frame/step)%step),extent-1);
    bool writer=all(p==representative);
    if(writer)history.Store(writeAddr+12,0);
    if(materials[uint3(p,0)]==65534)return;
    float depth=linearDepth.Load(int3(p,0)).x;
    float2 ndc=(float2(p)+.5)*sys[1].zw*float2(2,-2)+float2(-1,1);
    float4 h=transform(float4(ndc,clipDepth.Load(int3(p,0)).x,1),22);
    float4 surface=gbuffer.Load(int3(p,0));
    float3 world=h.xyz/h.w, n=receiverNormal(surface);
    uint receiverKey=uint(round(saturate(surface.z)*255));
    if(depth<=0 || !all(isfinite(world)) || !all(isfinite(n)))return;
    uint slice=(frame+cell)%rays;
    if(writer){
        uint material=materials[uint3(p,slice)];
        float3 endpoint=transform(float4(positions[uint3(p,slice)].xyz,1),6).xyz;
        if(material<65534 && all(isfinite(endpoint))){
            history.Store4(writeAddr,uint4(asuint(world),frame));
            history.Store4(writeAddr+16,uint4(asuint(endpoint),normalKey(n)));
            history.Store2(writeAddr+32,uint2(extent.x|(extent.y<<16),material|(rays<<16)|(receiverKey<<24)));
        }
    }
    // Limit visibility to half the receivers per frame, alternating coverage.
    if(((p.x^p.y^frame)&1)!=0 || frame<2)return;
    float4 prev=transform(float4(world,1),38);
    if(prev.w<=0 || !all(isfinite(prev)))return;
    float2 uv=prev.xy/prev.w*float2(.5,-.5)+.5;
    if(any(uv<0)||any(uv>=1))return;
    uint2 prevCell=(uint2(uv*extent))/step;
    uint index=prevCell.y*grid.x+prevCell.x;
    if(index>=capacity)return;
    uint addr=(((frame-1)&1)*capacity+index)*40;
    uint4 r=history.Load4(addr);
    if(r.w!=frame-1)return;
    uint2 meta=history.Load2(addr+32);
    if(meta.x!=(extent.x|(extent.y<<16)) || (meta.y>>16)!=(rays|(receiverKey<<8)))return;
    uint4 hit=history.Load4(addr+16);
    float3 prior=asfloat(r.xyz), oldNormal=unpackNormal(hit.w);
    float tolerance=clamp(depth*.0015f,.004f,.035f);
    if(distance(prior,world)>tolerance || abs(dot(prior-world,n))>.0025f ||
       dot(oldNormal,n)<.995f)return;
    float3 origin=world-depth*.001f*normalize(world-sys[54].xyz)+depth*.002f*n;
    float3 delta=asfloat(hit.xyz)-origin;
    float lengthToHit=length(delta);
    if(!isfinite(lengthToHit)||lengthToHit<.03f || lengthToHit>asfloat(controls.x)*.8f ||
       lengthToHit>65.0f || dot(delta,n)<=0)return;
    // RayTCurrent must match the old endpoint to within 3 mm. A blocked or
    // missed ray leaves this frame's already-traced sample in place.
    Payload payload;
    uint priorSlice=(frame-1+index)%rays;
    // Preserve the current miss/volume contribution and reserved receiver sentinel.
    if(materials[uint3(p,priorSlice)]>=65534)return;
    uint mat=meta.y&65535;
    payload.slice=0xc0000000u|(((mat^(mat>>8))&255)<<22)|(uint(round(lengthToHit*1000))<<6)|priorSlice;
    RayDesc ray;ray.Origin=origin;ray.Direction=delta/lengthToHit;
    ray.TMin=0;ray.TMax=lengthToHit+.003f;
    TraceRay(scene,RAY_FLAG_NONE,1,0,2,0,ray,payload);
}
