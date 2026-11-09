RaytracingAccelerationStructure scene : register(t0);
RWStructuredBuffer<uint4> results : register(u0);
[numthreads(8,1,1)]
void main(uint3 tid : SV_DispatchThreadID) {
 RayDesc ray;ray.Origin=float3(float(tid.x)*2.0,0,-2);ray.Direction=float3(0,0,1);ray.TMin=0.001;ray.TMax=100;
 RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
 q.TraceRayInline(scene,RAY_FLAG_NONE,255,ray);
 while(q.Proceed()){}
 uint4 hit=uint4(0xffffffff,0xffffffff,0xffffffff,0);
 if(q.CommittedStatus()==COMMITTED_TRIANGLE_HIT)
  hit=uint4(q.CommittedInstanceID(),q.CommittedInstanceIndex(),q.CommittedPrimitiveIndex(),asuint(q.CommittedRayT()));
 results[tid.x]=hit;
}
