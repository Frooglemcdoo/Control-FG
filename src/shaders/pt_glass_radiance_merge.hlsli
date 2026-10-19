#ifndef PT_GLASS_RADIANCE_MERGE
#define PT_GLASS_RADIANCE_MERGE
// The continuation is incomplete (notably baked/off-path lighting). Treat it
// as an enhancement of the native solution, not a complete replacement.
// Per-channel coverage retains colored native light. This intentionally cannot
// darken native probe leakage; it is a conservative hybrid, not a full estimator.
float3 PTGlassMergeRadiance(float3 nativeRadiance,float3 continuation) {
    return all(isfinite(continuation)) ? max(nativeRadiance,max(continuation,0.0f)) : nativeRadiance;
}
#endif
