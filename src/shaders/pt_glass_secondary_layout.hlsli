#ifndef PT_GLASS_SECONDARY_LAYOUT_HLSLI
#define PT_GLASS_SECONDARY_LAYOUT_HLSLI
// Dedicated mod-owned storage. Never alias the native glass hit textures or
// either opaque reflection sample layer. All values are full float precision.
// One 64-byte record per pixel; writers must invalidate every pixel per dispatch.
struct PTGlassSecondaryHit {
    float4 positionView_texcoordY;
    float4 normalWorld_texcoordX;
    float4 incomingWorld_hitDistance;
    uint4 metadata; // material ID, valid flag, dispatch epoch, reserved
};
PTGlassSecondaryHit PTGlassInvalidHit(uint dispatchEpoch) {
    PTGlassSecondaryHit hit = (PTGlassSecondaryHit)0;
    hit.metadata = uint4(65534u, 0u, dispatchEpoch, 0u);
    return hit;
}
bool PTGlassValidHit(PTGlassSecondaryHit hit, uint dispatchEpoch, uint materialCount) {
    return hit.metadata.y == 1u && hit.metadata.z == dispatchEpoch &&
        hit.metadata.x < materialCount && hit.metadata.x < 65534u &&
        all(isfinite(hit.positionView_texcoordY)) &&
        all(isfinite(hit.normalWorld_texcoordX)) &&
        all(isfinite(hit.incomingWorld_hitDistance)) &&
        hit.incomingWorld_hitDistance.w > 0.0f &&
        dot(hit.normalWorld_texcoordX.xyz,hit.normalWorld_texcoordX.xyz) > 0.5f &&
        dot(hit.incomingWorld_hitDistance.xyz,hit.incomingWorld_hitDistance.xyz) > 0.5f;
}
#endif
