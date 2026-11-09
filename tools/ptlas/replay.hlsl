RaytracingAccelerationStructure scene : register(t0);
struct Ray { float3 origin; float tmin; float3 direction; float tmax; };
StructuredBuffer<Ray> rays : register(t1);
RWStructuredBuffer<uint4> results : register(u0);
[numthreads(8,1,1)]
void main(uint3 tid : SV_DispatchThreadID) {
 Ray r=rays[tid.x];RayDesc ray;ray.Origin=r.origin;ray.Direction=r.direction;ray.TMin=r.tmin;ray.TMax=r.tmax;
 RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
 q.TraceRayInline(scene,RAY_FLAG_NONE,255,ray);while(q.Proceed()){}
 uint4 hit=uint4(0xffffffff,0xffffffff,0xffffffff,0);
 if(q.CommittedStatus()==COMMITTED_TRIANGLE_HIT)
  hit=uint4(q.CommittedInstanceID(),q.CommittedInstanceIndex(),q.CommittedPrimitiveIndex(),asuint(q.CommittedRayT()));
 results[tid.x]=hit;
}
