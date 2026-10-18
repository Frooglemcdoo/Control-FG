#ifndef CONTROL_PT_GUIDE_NORMAL
#define CONTROL_PT_GUIDE_NORMAL
// Raw packet DWORD 11 stores the primary VIEW normal, not unused roughness.
// Bit 31 distinguishes packed normals from older/unpopulated packets.
uint PTPackGuideNormal(float3 n){
    uint3 v=(uint3)round(saturate(n*.5+.5)*1023);
    return 0x80000000u|v.x|(v.y<<10)|(v.z<<20);
}
float3 PTUnpackGuideNormal(uint n){
    return normalize(float3(n&1023,(n>>10)&1023,(n>>20)&1023)*(2.0/1023)-1);
}
#endif
