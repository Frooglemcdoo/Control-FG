// Frozen R462 reference: independent arithmetic for regression comparison.
// Research prototype only. No hard-coded scene objects or persistent radiance.
// Caller freezes valid world-space bounds BEFORE drawing a receiver ray.
// Return radianceWeight must multiply ALL evaluated radiance, never hit distance.
// Keep the original independent range random variable and configured ray count.
static const float R462GT_PI = 3.14159265358979323846;
struct R462GTBound { float3 center; float radius; };
struct R462GTCone { float3 axis; float oneMinusCos; };
struct R462GTDraw { float3 direction; float radianceWeight; float pdf; uint traceRay; };

float3 R462GTLocal(float3 axis,float z,float angle) {
    float signZ=axis.z>=0?1:-1;
    float a=-1/(signZ+axis.z),b=axis.x*axis.y*a;
    float3 t=float3(1+signZ*axis.x*axis.x*a,signZ*b,-signZ*axis.x);
    float3 bt=float3(b,signZ+axis.y*axis.y*a,-axis.y);
    float radius=sqrt(max(0,1-z*z));
    return axis*z+radius*(cos(angle)*t+sin(angle)*bt);
}
R462GTCone R462GTMakeCone(float3 origin,R462GTBound bound) {
    R462GTCone c;
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
float R462GTConePdf(R462GTCone cone,float3 direction) {
    return dot(cone.axis,direction)>=1-cone.oneMinusCos?
        1/(2*R462GT_PI*cone.oneMinusCos):0;
}
bool R462GTValidBound(R462GTBound b) {
    return all(isfinite(b.center))&&isfinite(b.radius)&&b.radius>0;
}
R462GTDraw R462GTSampleMixture(float3 origin,float3 unitNormal,R462GTBound first,R462GTBound second,
                float guideFraction,bool frozenGuideValid,float u,float v) {
    // Caller supplies finite origin/unit normal and independent u,v in [0,1).
    float alpha=frozenGuideValid&&R462GTValidBound(first)&&R462GTValidBound(second)&&
        isfinite(guideFraction)?clamp(guideFraction,0,.5):0;
    R462GTCone a,b;
    if(alpha>0){a=R462GTMakeCone(origin,first);b=R462GTMakeCone(origin,second);}
    R462GTDraw r=(R462GTDraw)0;
    if(alpha>0&&u<alpha) {
        float select=u/alpha*2;
        R462GTCone c=a;if(select>=1)c=b;
        float remapped=frac(select);
        float z=1-v*c.oneMinusCos;
        r.direction=R462GTLocal(c.axis,z,2*R462GT_PI*remapped);
    } else {
        float remapped=(u-alpha)/(1-alpha);
        r.direction=R462GTLocal(unitNormal,sqrt(v),2*R462GT_PI*remapped);
    }
    float cosine=max(0,dot(unitNormal,r.direction));
    float base=cosine/R462GT_PI;
    float guide=alpha>0?.5*(R462GTConePdf(a,r.direction)+R462GTConePdf(b,r.direction)):0;
    r.pdf=(1-alpha)*base+alpha*guide;
    r.radianceWeight=r.pdf>0?base/r.pdf:(alpha==0?1:0);
    r.traceRay=cosine>0||(alpha==0&&v==0);
    return r;
}

// Preserve the original prototype contract for existing callers.
R462GTDraw R462GTSample(float3 origin,float3 unitNormal,R462GTBound first,R462GTBound second,
                float guideFraction,bool frozenGuideValid,float u,float v) {
 return R462GTSampleMixture(origin,unitNormal,first,second,guideFraction>.1?.1:guideFraction,frozenGuideValid,u,v);
}

#ifdef R462GT_COMPILE_TEST
// Compilation fixture only; this is NOT a replacement game shader.
StructuredBuffer<float4> inputs:register(t0);
RWStructuredBuffer<float4> outputs:register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint i=id.x*6;
    R462GTBound a,b;a.center=inputs[i+2].xyz;a.radius=inputs[i+2].w;
    b.center=inputs[i+3].xyz;b.radius=inputs[i+3].w;
    R462GTDraw r=R462GTSample(inputs[i].xyz,inputs[i+1].xyz,a,b,inputs[i+4].x,
                     inputs[i+4].y>0,inputs[i+5].x,inputs[i+5].y);
    outputs[id.x*2]=float4(r.direction,r.radianceWeight);
    outputs[id.x*2+1]=float4(r.pdf,r.traceRay,0,0);
}
#endif

#ifndef CONTROL_GI_AUTO_SAMPLE_R462_REFERENCE_H
#define CONTROL_GI_AUTO_SAMPLE_R462_REFERENCE_H



// Paired ten-ray baseline and four-ray candidate, both single-batch.
// The caller must freeze one target resource for producer AND evaluator.
float R462GIAutoAlpha(GIAutoTargets targets,GIAutoSelection selection) {
 return ((selection.rayCount==10&&selection.fraction==.4f)||(selection.rayCount==4&&selection.fraction==.5f))&&GIAutoUsable(targets,selection)?selection.fraction:0;
}
uint R462GIAutoSeed(uint2 pixel,uint frame,uint batch,uint ray) {
 return pixel.x*73856093u^pixel.y*19349663u^frame*83492791u^
        batch*2654435761u^ray*1597334677u;
}
// Exact R307 + owned strata permutation; four-ray progressive mode is excluded
// from this ten-ray fallback. Four rays use their own complete strata.
float2 R462GIAutoOwned10(uint2 pixel,uint frame,uint batch,uint ray,uint seed) {
 uint column=ray%2,row=ray/2;
 uint key=(pixel.x*73856093u^pixel.y*19349663u^batch*2654435761u)^((frame>>2)*2654435761u);
 uint hash=(key^(key>>16))*2146121005u;
 uint xbin=((hash>>12)+column*73u+row*67u)%5u;
 uint ybin=((hash>>23)^row^column)%2u;
 float u=(float(column)+(float(xbin)+GIProducerUnit(seed,0x68bc21ebu))/5.0f)/2.0f;
 float v=(float(row)+(float(ybin)+GIProducerUnit(seed,0x02e5be93u))/2.0f)/5.0f;
 return min(float2(u,v),.9999999403953552f);
}
struct R462GIAutoDraw {float3 direction;float weight,range;uint traceRay;};
R462GIAutoDraw R462GIAutoSample(uint2 pixel,uint frame,uint batch,uint ray,float3 origin,
                      float3 normal,GIAutoTargets targets,GIAutoSelection selection) {
 // Both stages use the invocation seed, never independently sourced native CB clocks.
 frame=selection.sampleFrame;
 R462GIAutoDraw result=(R462GIAutoDraw)0;
 uint seed=R462GIAutoSeed(pixel,frame,batch,ray);
 result.range=GIProducerUnit(seed,0x967a889bu);
 float alpha=R462GIAutoAlpha(targets,selection);
 if(alpha==0) {
  float2 uv;
  if(selection.rayCount==4){GIProducerCoordinates nativeDraw=GIProducerSample(pixel.x,pixel.y,frame,batch,ray,4);uv=float2(nativeDraw.u,nativeDraw.v);}
  else uv=R462GIAutoOwned10(pixel,frame,batch,ray,seed);
  result.direction=R462GTLocal(normal,sqrt(uv.y),2*R462GT_PI*uv.x);
  result.weight=1;result.traceRay=1;return result;
 }
 // Stratify the mixture selector over the selected ray count. At alpha=.4 this gives
 // two samples per target cone and six cosine samples; four rays use 1+1+2.
 // Each stratum has a fresh independent range draw and within-stratum jitter.
 float u=min((float(ray)+GIProducerUnit(seed,0x68bc21ebu))/float(selection.rayCount),.9999999403953552f);
 float v=GIProducerUnit(seed,0x02e5be93u);
 // Latin stratification within each fixed component preserves the component's
 // full radial marginal while avoiding six independent cosine elevations.
 // The shared random rotation is independent of per-ray angular/range jitter.
 if(alpha==.4f||alpha==.5f){
  uint perGuide=selection.rayCount==4?1:2;
  uint component=ray<perGuide?0:(ray<2*perGuide?1:2);
  uint size=component==2?selection.rayCount-2*perGuide:perGuide;
  uint local=component==2?ray-2*perGuide:ray-component*perGuide;
  uint rotation=GIProducerHash(pixel.x*0x8da6b343u^pixel.y*0xd8163841u^
                    frame*0xcb1ab31fu^batch*0x165667b1u^component*0x9e3779b9u)%size;
  v=(float((local+rotation)%size)+v)/float(size);
 }

 uint first=(targets.validMask&1)?0:1;
 uint second=targets.validMask==3?1:first;
 R462GTBound a,b;
 a.center=float3(targets.bounds[first].center.x,targets.bounds[first].center.y,targets.bounds[first].center.z);a.radius=targets.bounds[first].radius;
 b.center=float3(targets.bounds[second].center.x,targets.bounds[second].center.y,targets.bounds[second].center.z);b.radius=targets.bounds[second].radius;
 R462GTDraw draw=R462GTSampleMixture(origin,normal,a,b,alpha,true,u,v);
 result.direction=draw.direction;result.weight=draw.radianceWeight;result.traceRay=draw.traceRay;return result;
}
#endif
