float PackSurfaceWord(uint bits) {
    return f16tof32(0x400u+(bits&0x3fffu) | ((bits&0x4000u)<<1));
}
uint UnpackSurfaceWord(float value) {
    uint h=f32tof16(value);
    return ((h&0x7fffu)-0x400u) | ((h>>1)&0x4000u);
}
float4 CharacterSurface(float depth,float opacity,uint2 instance,uint primitive) {
    uint z=asuint(depth); // Raster depth [0,1] occupies 30 bits.
    uint tag=(instance.x*0x9e3779b9u)^(instance.y*0x85ebca6bu)^(primitive*0xc2b2ae35u);
    tag^=tag>>16;
    return float4(PackSurfaceWord(z&32767),PackSurfaceWord(z>>15),PackSurfaceWord(tag&32767),opacity);
}
