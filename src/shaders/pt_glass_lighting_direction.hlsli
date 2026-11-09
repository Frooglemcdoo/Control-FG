#ifndef PT_GLASS_LIGHTING_DIRECTION
#define PT_GLASS_LIGHTING_DIRECTION
// Incident travels from the previous path vertex to this hit. Direct-light BRDFs
// need the reverse direction, rotated into the native light records' view space.
// Positions and shadow lookups must continue using the actual hit position.
float3 PTGlassLightView(float3 incidentWorld,float3 worldToViewX,float3 worldToViewY,float3 worldToViewZ) {
    return -normalize(incidentWorld.x*worldToViewX+incidentWorld.y*worldToViewY+incidentWorld.z*worldToViewZ);
}
#endif
