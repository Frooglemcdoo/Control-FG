// Research prototype only. No hard-coded scene objects or persistent radiance.
// Caller freezes valid world-space bounds BEFORE drawing a receiver ray.
// Return radianceWeight must multiply ALL evaluated radiance, never hit distance.
// Keep the original independent range random variable and configured ray count.
static const float GT_PI = 3.14159265358979323846;
struct GTBound { float3 center; float radius; };
struct GTCone { float3 axis; float oneMinusCos; };
struct GTDraw { float3 direction; float radianceWeight; float pdf; uint traceRay; };

float3 GTLocal(float3 axis,float z,float angle) {
    float signZ=axis.z>=0?1:-1;
    float a=-1/(signZ+axis.z),b=axis.x*axis.y*a;
    float3 t=float3(1+signZ*axis.x*axis.x*a,signZ*b,-signZ*axis.x);
    float3 bt=float3(b,signZ+axis.y*axis.y*a,-axis.y);
    float radius=sqrt(max(0,1-z*z));
    return axis*z+radius*(cos(angle)*t+sin(angle)*bt);
}
GTCone GTMakeCone(float3 origin,GTBound bound) {
    GTCone c;
    float3 delta=bound.center-origin;float d2=dot(delta,delta);
    c.axis=d2>1e-20?delta*rsqrt(d2):float3(0,0,1);
    if(d2<=bound.radius*bound.radius)c.oneMinusCos=2; // Full sphere.
    else {
        float sin2=min(1,bound.radius*bound.radius/d2);
        // Stable 1-sqrt(1-sin2). Widen minuscule cones consistently in both PDFs.
        c.oneMinusCos=max(1e-6,sin2/(1+sqrt(max(0,1-sin2))));
    }
    return c;
}
float GTConePdf(GTCone cone,float3 direction) {
    return dot(cone.axis,direction)>=1-cone.oneMinusCos?
        1/(2*GT_PI*cone.oneMinusCos):0;
}
bool GTValidBound(GTBound b) {
    return all(isfinite(b.center))&&isfinite(b.radius)&&b.radius>0;
}
GTDraw GTSample(float3 origin,float3 unitNormal,GTBound first,GTBound second,
                float guideFraction,bool frozenGuideValid,float u,float v) {
    // Caller supplies finite origin/unit normal and independent u,v in [0,1).
    float alpha=frozenGuideValid&&GTValidBound(first)&&GTValidBound(second)&&
        isfinite(guideFraction)?clamp(guideFraction,0,.1):0;
    GTCone a,b;
    if(alpha>0){a=GTMakeCone(origin,first);b=GTMakeCone(origin,second);}
    GTDraw r=(GTDraw)0;
    if(alpha>0&&u<alpha) {
        float select=u/alpha*2;
        GTCone c=a;if(select>=1)c=b;
        float remapped=frac(select);
        float z=1-v*c.oneMinusCos;
        r.direction=GTLocal(c.axis,z,2*GT_PI*remapped);
    } else {
        float remapped=(u-alpha)/(1-alpha);
        r.direction=GTLocal(unitNormal,sqrt(v),2*GT_PI*remapped);
    }
    float cosine=max(0,dot(unitNormal,r.direction));
    float base=cosine/GT_PI;
    float guide=alpha>0?.5*(GTConePdf(a,r.direction)+GTConePdf(b,r.direction)):0;
    r.pdf=(1-alpha)*base+alpha*guide;
    r.radianceWeight=r.pdf>0?base/r.pdf:(alpha==0?1:0);
    r.traceRay=cosine>0||(alpha==0&&v==0);
    return r;
}

#ifdef GT_COMPILE_TEST
// Compilation fixture only; this is NOT a replacement game shader.
StructuredBuffer<float4> inputs:register(t0);
RWStructuredBuffer<float4> outputs:register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint i=id.x*6;
    GTBound a,b;a.center=inputs[i+2].xyz;a.radius=inputs[i+2].w;
    b.center=inputs[i+3].xyz;b.radius=inputs[i+3].w;
    GTDraw r=GTSample(inputs[i].xyz,inputs[i+1].xyz,a,b,inputs[i+4].x,
                     inputs[i+4].y>0,inputs[i+5].x,inputs[i+5].y);
    outputs[id.x*2]=float4(r.direction,r.radianceWeight);
    outputs[id.x*2+1]=float4(r.pdf,r.traceRay,0,0);
}
#endif
