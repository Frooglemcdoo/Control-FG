// Keep the confirmed two-bounce output when the optional extension is invalid.
bool PTThirdFinite(float3 v) {
#ifdef __cplusplus
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
#else
    return !any((asuint(v)&0x7f800000u)==0x7f800000u);
#endif
}
float3 PTOptionalThirdOutput(float3 twoBounce,float3 candidate,bool valid) {
    return valid&&PTThirdFinite(candidate)?candidate:twoBounce;
}
