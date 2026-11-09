// Capture-only geometry. Rows are the native row-vector projection layout.
bool captureProjectionApex(float4 rows[4], out float3 apex) {
    float3 a=float3(rows[0].x,rows[1].x,rows[2].x);
    float3 b=float3(rows[0].y,rows[1].y,rows[2].y);
    float3 c=float3(rows[0].w,rows[1].w,rows[2].w);
    float determinant=dot(a,cross(b,c));
    float scale=length(a)*length(b)*length(c);
    apex=0;
    if (!isfinite(scale) || scale<=0 || !isfinite(determinant) || abs(determinant)<=scale*1e-6) return false;
    apex=(-rows[3].x*cross(b,c)-rows[3].y*cross(c,a)-rows[3].w*cross(a,b))/determinant;
    return all(isfinite(apex));
}
float3 captureWorld(float3 value, float4 rows[4], bool isPoint) {
    return value.x*rows[0].xyz+value.y*rows[1].xyz+value.z*rows[2].xyz+(isPoint?rows[3].xyz:float3(0,0,0));
}
bool captureClippedSegment(float3 origin, float3 target, float3 cone, float3 axis, float nearDistance,
                           out float3 direction, out float maximum) {
    direction=0; maximum=0;
    float3 delta=target-origin;
    float distance=length(delta), norm=length(axis);
    if (!all(isfinite(origin)) || !all(isfinite(target)) || !all(isfinite(cone)) ||
        !isfinite(distance) || distance<=1e-6 || !isfinite(norm) || norm<=1e-6 ||
        !isfinite(nearDistance) || nearDistance<0) return false;
    direction=delta/distance;
    maximum=distance*(1.0-1.0/1048576.0);
    float3 normal=axis/norm;
    float start=dot(origin-cone,normal)-nearDistance;
    float end=start+dot(delta,normal);
    if (!isfinite(start) || !isfinite(end)) return false;
    if (start>0 && end<0) maximum*=start/(start-end);
    return isfinite(maximum) && maximum>0;
}
